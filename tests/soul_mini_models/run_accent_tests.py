#!/usr/bin/env python3
"""Real pinned ImGui probe for both soul-model checkboxes in both native menus."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/whats_new'))
from run_generate_tests import block

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--negative-control-without-accent',action='store_true',help='remove scopes in the generated probe; the assertions must fail')
    args=parser.parse_args()
    imgui=Path(os.environ.get('IMGUI_DIR',''))
    if not (imgui/'imgui.cpp').is_file():
        raise SystemExit('Set IMGUI_DIR to the project pinned Dear ImGui source directory')
    with tempfile.TemporaryDirectory(prefix='soul-menu-accent-') as temporary:
        directory=Path(temporary)
        pieces=[]
        for namespace,host,menu,widget in [
            ('OotWidgets','soh','soh/soh/SohGui/SohMenuRandomizer.cpp','soh/soh/SohGui/UIWidgets.cpp'),
            ('MmWidgets','mm','mm/2s2h/Rando/Menu.cpp','mm/2s2h/BenGui/UIWidgets.cpp')]:
            source=(ROOT/widget).read_text()
            options='''
enum class Colors { LightBlue };
enum class LabelPositions { Near, Far, Above, None };
using LabelPosition=LabelPositions;
enum class ComponentAlignments { Left, Right };
using ComponentAlignment=ComponentAlignments;
const std::map<Colors,ImVec4> ColorValues{{Colors::LightBlue,ImVec4(.3f,.6f,.8f,1)}};
struct WidgetOptions {
'''
            options+=('const char* tooltip=""; const char* disabledTooltip="";' if host=='mm' else
                      'std::string tooltip; std::string disabledTooltip;')
            options+='''
bool disabled=false;
};
struct CheckboxOptions:WidgetOptions {
    bool defaultValue=false;
    LabelPositions labelPosition=LabelPositions::Near;
    ComponentAlignments alignment=ComponentAlignments::Left;
    Colors color=Colors::LightBlue;
    ImVec2 padding=ImVec2(10,8);
    CheckboxOptions& Tooltip(const char* text){tooltip=text;return *this;}
};
std::string WrappedText(const char*,unsigned int=80);
std::string WrappedText(const std::string&,unsigned int=80);
void RenderText(ImVec2,const char*,const char*=nullptr,bool=true,float=0);
'''.replace('const char*=','const char* =')
            if host=='mm':
                options=options.replace('bool=true,float=0','bool=true')
            functions='\n'.join(block(source,sig) for sig in [
                'std::string WrappedText(const char*','std::string WrappedText(const std::string&',
                'void PushStyleCheckbox(const ImVec4&','void PushStyleCheckbox(Colors',
                'void PopStyleCheckbox()','void RenderText(','bool Checkbox('])
            native=block(source,'bool CVarCheckbox(').replace('bool CVarCheckbox(','bool NativeCVarCheckbox(',1)
            observer='''
bool CVarCheckbox(const char* label,const char* key,const CheckboxOptions& options){
    Row row{};row.label=label;row.key=key;row.original=ImGui::GetID(label);
    row.text=ImGui::GetStyle().Colors[ImGuiCol_Text];
    auto* list=ImGui::GetWindowDrawList();const int start=list->VtxBuffer.Size;
    row.changed=NativeCVarCheckbox(label,key,options);
    row.id=ImGui::GetItemID();row.min=ImGui::GetItemRectMin();row.max=ImGui::GetItemRectMax();
    const ImU32 gold=ImGui::ColorConvertFloat4ToU32(WhatsNew::Accent(WhatsNew::Kind::New));
    for(int i=start;i<list->VtxBuffer.Size;i++)row.goldVertices+=list->VtxBuffer[i].col==gold;
    rows.push_back(row);return row.changed;
}
'''
            registration=(ROOT/menu).read_text()
            if host=='soh':
                regs=[]
                for label in ['Simpler Enemy Soul Models','Simpler NPC Soul Models']:
                    m=re.search(r'AddWidget\(path, "'+label+r'", WIDGET_CVAR_CHECKBOX\).*?;',registration,re.S)
                    assert m,label
                    regs.append(m[0])
                renderer=(ROOT/'soh/soh/SohGui/Menu.cpp').read_text()
                scope=re.search(r'WhatsNew::Scope releaseHighlight\(widget\.cVar,.*?;',renderer,re.S).group(0)
                body='''
struct Widget {std::string name;const char* cVar=nullptr;CheckboxOptions storage;CheckboxOptions* options=&storage;
Widget& CVar(const char* key){cVar=key;return *this;}
Widget& RaceDisable(bool value){Check(!value,"soul model preferences must stay live");return *this;}
Widget& Options(const CheckboxOptions& value){storage=value;return *this;}};
static Widget widgets[2];static int nextWidget;
Widget& AddWidget(int,const char* name,int){Check(nextWidget<2,"unexpected registration");auto& w=widgets[nextWidget++];w.name=name;return w;}
void Register(){int path=0;constexpr int WIDGET_CVAR_CHECKBOX=0;
'''+ '\n'.join(regs)+'''
Check(nextWidget==2,"both OoT registrations required");}
void DrawControls(){for(auto& widget:widgets){
'''+scope+'''
CVarCheckbox(widget.name.c_str(),widget.cVar,*widget.options);
}}
'''
            else:
                start=registration.index('    {\n        WhatsNew::Scope releaseHighlight(ComboSoulModels::kSimplerEnemyCvar')
                end=registration.index('\n#endif',start)
                body='void Register(){}\nvoid DrawControls(){\n'+registration[start:end]+'\n}\n'
            if args.negative_control_without_accent:
                body=re.sub(r'WhatsNew::Scope releaseHighlight\(.*?\);','',body,flags=re.S)
            pieces.append('namespace '+namespace+' {\n'+options+functions+'\n'+native+observer+body+'\n}\n')
        (directory/'accent_controls.inc').write_text('\n'.join(pieces))
        bridge=directory/'libultraship/bridge/consolevariablebridge.h'
        bridge.parent.mkdir(parents=True);bridge.write_text('#pragma once\nint CVarGetInteger(const char*,int);\n')
        binary=directory/'accent'
        flags=['-std=c++20','-Wall','-Wextra','-Werror','-Wno-missing-field-initializers','-DIMGUI_DEFINE_MATH_OPERATORS=']
        if os.environ.get('SANITIZE'):
            flags+=['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
        command=[*shlex.split(os.environ.get('CXX','c++')),*flags,'-I'+str(directory),'-I'+str(ROOT/'combo'),'-I'+str(imgui),str(ROOT/'tests/soul_mini_models/accent_test.cpp')]
        command+=[str(imgui/name) for name in ('imgui.cpp','imgui_draw.cpp','imgui_tables.cpp','imgui_widgets.cpp')]
        subprocess.run([*command,'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
if __name__=='__main__':main()
