#!/usr/bin/env python3
"""Execute actual Run setting lookup, indexed toggle/Flush, and menu registrations."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]

def block(source,marker):
    start=source.index(marker);opening=source.index('{',start);end=opening+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]

def run(sanitize):
    source=(ROOT/'libultraship/src/fast/interpreter.cpp').read_text()
    header=(ROOT/'libultraship/include/fast/interpreter.h').read_text()
    start=source.index('void Interpreter::Run(Gfx* commands,')
    run_prefix=source[source.index('{',start)+1:source.index('const bool sampled = ',start)]
    setter=block(header,'    void SetIndexedTrianglesEnabled(')
    flush=block(source,'void Interpreter::Flush()')
    fixture=r'''
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
namespace Ship {
struct ConsoleVariables {
 std::unordered_map<std::string,int> saved;
 int GetInteger(const char* name,int fallback) {return saved.count(name)?saved.at(name):fallback;}
 void SetInteger(const char* name,int value){saved[name]=value;}
};
struct Context {
 ConsoleVariables vars;
 static Context* GetRawInstance(){static Context context;return &context;}
 ConsoleVariables* GetConsoleVariables(){return &vars;}
 Context* GetWindow(){return this;}Context* GetGui(){return this;}
 void SaveConsoleVariablesNextFrame(){}
};
}
/* CVAR_BRIDGE */
namespace ShipInit {void Init(const char*){}}
namespace Fast {
enum class RenderCostOperation {DriverDraw};
struct Probe {struct {size_t submittedTriangles=0;} report;bool Active(){return false;}auto& MutableReport(){return report;}};
struct RenderOperationScope {RenderOperationScope(Probe*,RenderCostOperation){}};
constexpr float N64_PRIM_DEPTH_MAX=32767.0f;
struct Backend {
 bool supported=true;int draws=0,indexedDraws=0;
 bool SupportsIndexedTriangles(){return supported;}
 void SetCurrentPrimDepth(float){}
 void SetToonLighting(float*,float*,float*){}
 void DrawTriangles(float*,size_t,size_t){++draws;}
 void DrawIndexedTriangles(float*,size_t,const uint16_t*,size_t,size_t){++indexedDraws;}
};
struct Interpreter {
 Backend backend;Backend* mRapi=&backend;Probe mRenderCost;
 struct {uint64_t packedVertexMask=0;} mTriangleState;
 struct {int prim_depth=0;} rdp;decltype(rdp)* mRdp=&rdp;
 struct {float toon_light_dir[3]{},toon_light_color[3]{},toon_ambient[3]{};} rsp;decltype(rsp)* mRsp=&rsp;
 bool mIndexedTrianglesEnabled=false;
 uint64_t mIndexedVertexMask=0;
 float mBufVbo[32]{};uint16_t mBufIndices[8]{};
 size_t mBufVboLen=0,mBufVboNumTris=0,mBufIndexedVertices=0,mBufIndexLen=0,mBufVertexStride=0;
 bool mBufUsesIndices=false,mBufIndexable=false;
 bool mPreludeMaterialBindingReuseEnabled=false;
 const void* mPreludeMaterialBoundResources[2]{};
 void Flush();
 /* SETTER */
 void BeginRun(){/* RUN */}
 void Pending(bool indexed) {
  mBufVboLen=12;mBufVboNumTris=2;mBufIndexedVertices=4;mBufIndexLen=6;mBufVertexStride=3;
  mBufUsesIndices=indexed;mBufIndexable=indexed;mIndexedVertexMask=3;mTriangleState.packedVertexMask=3;
 }
};
/* FLUSH */
}
struct CheckboxOptions {
 bool defaultValue=false;
 CheckboxOptions& DefaultValue(bool value){defaultValue=value;return *this;}
 CheckboxOptions& Tooltip(const char*){return *this;}
};
static bool clicked=false,nextValue=false,observedValue=false;
namespace ImGui {
bool Checkbox(const char*,bool* value){observedValue=*value;if(!clicked)return false;*value=nextValue;clicked=false;return true;}
}
/* NATIVE_CHECKBOXES */
struct ImVec4 {};
void ComboMenu_PushCheckbox(ImVec4){}void ComboMenu_PopCheckbox(){}
constexpr int CW_CHECKBOX=1;
struct SharedWidget {const char* cvar;bool bDefault;int kind=CW_CHECKBOX;};
bool SharedCheckbox(const SharedWidget& w) {
 const char* label="Indexed Triangle Rendering (Experimental)";ImVec4 themeColor;bool changed=false;
 switch(w.kind){/* SHARED_CHECKBOX */default:break;}
 return changed;
}
struct Path {std::string sidebarName="Graphics";int column=2;};
constexpr int WIDGET_CVAR_CHECKBOX=1;
struct Widget {
 std::string name,key,sidebar;int kind=0,column=0;bool defaultValue=false;
 Widget& CVar(const char* key_){key=key_;return *this;}
 Widget& RaceDisable(bool){return *this;}
 Widget& Options(const CheckboxOptions& options){defaultValue=options.defaultValue;return *this;}
};
std::vector<Widget> widgets;
Widget& AddWidget(Path path,const char* name,int kind){widgets.push_back({name,"",path.sidebarName,kind,path.column,false});return widgets.back();}
void OotMenu(){Path path;/* OOT */}
void MmMenu(){Path path;/* MM */}
int main(int argc,char** argv) {
 if(argc>1 && std::string(argv[1])=="menu") {
  OotMenu();MmMenu();assert(widgets.size()==2 && "indexed setting must be registered in both native Graphics menus");
  for(const auto& w:widgets) {
   assert(w.key=="gEnhancements.IndexedTriangles" && w.kind==WIDGET_CVAR_CHECKBOX);
   assert(w.sidebar=="Graphics" && w.column==2 && !w.defaultValue);
   assert(w.name.find("Experimental")!=std::string::npos);
  }
  auto& vars=Ship::Context::GetRawInstance()->vars;vars.saved.clear();
  Fast::Interpreter oot,mm;
  SharedWidget shared{widgets[0].key.c_str(),widgets[0].defaultValue};
  assert(!SharedCheckbox(shared) && !observedValue && vars.saved.empty());
  for(size_t game=0;game<widgets.size();game++) {
   clicked=true;nextValue=true;CheckboxOptions options;options.defaultValue=widgets[game].defaultValue;
   if(game==0)assert(OotUI::CVarCheckbox(widgets[game].name.c_str(),widgets[game].key.c_str(),options));
   else assert(MmUI::CVarCheckbox(widgets[game].name.c_str(),widgets[game].key.c_str(),options));
   oot.BeginRun();mm.BeginRun();assert(oot.mIndexedTrianglesEnabled && mm.mIndexedTrianglesEnabled);
   clicked=true;nextValue=false;assert(SharedCheckbox(shared));
   oot.BeginRun();mm.BeginRun();assert(!oot.mIndexedTrianglesEnabled && !mm.mIndexedTrianglesEnabled);
  }
  puts("PASS indexed menu: OoT/MM Graphics Advanced checkboxes use one key and default OFF; Combo inherits OoT Graphics");return 0;
 }
 auto& vars=Ship::Context::GetRawInstance()->vars;
 Fast::Interpreter gfx;gfx.BeginRun();
 assert(!gfx.mIndexedTrianglesEnabled && "missing IndexedTriangles key silently enables indexed rendering");
 vars.saved["unrelated.existing.setting"]=1;gfx.BeginRun();assert(!gfx.mIndexedTrianglesEnabled);
 gfx.Pending(false);vars.saved["gEnhancements.IndexedTriangles"]=1;gfx.BeginRun();
 assert(gfx.mIndexedTrianglesEnabled && gfx.backend.draws==1 && gfx.mBufVboLen==0);
 gfx.Pending(true);gfx.BeginRun();assert(gfx.mBufVboLen==12 && gfx.backend.indexedDraws==0);
 vars.saved["gEnhancements.IndexedTriangles"]=0;gfx.BeginRun();
 assert(!gfx.mIndexedTrianglesEnabled && gfx.backend.indexedDraws==1 && gfx.mBufVboLen==0);
 assert(gfx.mIndexedVertexMask==0 && gfx.mTriangleState.packedVertexMask==0);
 vars.saved["gEnhancements.IndexedTriangles"]=1;gfx.BeginRun();assert(gfx.mIndexedTrianglesEnabled);
 assert(vars.saved.at("gEnhancements.IndexedTriangles")==1 && "explicit opt-in preference was overwritten");
 vars.saved.erase("gEnhancements.IndexedTriangles");gfx.Pending(true);gfx.BeginRun();
 assert(!gfx.mIndexedTrianglesEnabled && gfx.backend.indexedDraws==2);
 Fast::Interpreter unsupported;unsupported.backend.supported=false;unsupported.Pending(false);
 vars.saved["gEnhancements.IndexedTriangles"]=1;unsupported.BeginRun();
 assert(!unsupported.mIndexedTrianglesEnabled && unsupported.backend.indexedDraws==0 && unsupported.mBufVboLen==12);
 unsupported.Flush();assert(unsupported.backend.draws==1 && unsupported.backend.indexedDraws==0);
 puts("PASS indexed native Run: absent/existing config OFF, explicit opt-in, toggle batch flush, unsupported backend fallback");
}
'''
    menus=[]
    for path in ('soh/soh/SohGui/SohMenuSettings.cpp','mm/2s2h/BenGui/BenMenu.cpp'):
        menu=(ROOT/path).read_text();needle='.CVar("gEnhancements.IndexedTriangles")'
        if needle not in menu:
            menus.append('')
            continue
        at=menu.index(needle);start=menu.rfind('    AddWidget(',0,at);end=menu.index(';',at)+1
        # Confirm the actual registration lives in Graphics column 2, rather
        # than letting the fixture invent its placement.
        context=menu[:start]
        assert re.findall(r'path.sidebarName\s*=\s*"([^"]+)"',context)[-1]=='Graphics'
        assert re.findall(r'path.column\s*=\s*(SECTION_COLUMN_\d)',context)[-1]=='SECTION_COLUMN_2'
        menus.append(menu[start:end])
    combo=(ROOT/'combo/gui/ComboMenu.cpp').read_text()
    assert 'AppendSectionEntries(e, "Settings", HubEntry::ENGINE, model.Oot(), "Settings"' in combo
    render=(ROOT/'combo/gui/ComboWidgetRender.cpp').read_text()
    assert 'game.drawWidget && DelegatesToGame(w.kind)' in render
    assert 'case CW_CHECKBOX:' in block(render,'static bool DelegatesToGame(')
    shared=block(render,'        case CW_CHECKBOX: {')+'\nbreak;\n'
    bridge=(ROOT/'libultraship/src/libultraship/bridge/consolevariablebridge.cpp').read_text()
    cvars=block(bridge,'int32_t CVarGetInteger(')+'\n'+block(bridge,'void CVarSetInteger(')
    native=[]
    for namespace,path in (('OotUI','soh/soh/SohGui/UIWidgets.cpp'),('MmUI','mm/2s2h/BenGui/UIWidgets.cpp')):
        text=(ROOT/path).read_text()
        native.append('namespace '+namespace+' {\nbool Checkbox(const char* label,bool* value,const CheckboxOptions&){return ImGui::Checkbox(label,value);}\n'+block(text,'bool CVarCheckbox(')+'\n}\n')
    fixture=fixture.replace('/* SETTER */',setter).replace('/* RUN */',run_prefix).replace('/* FLUSH */',flush)
    fixture=fixture.replace('/* OOT */',menus[0]).replace('/* MM */',menus[1])
    fixture=fixture.replace('/* CVAR_BRIDGE */',cvars).replace('/* NATIVE_CHECKBOXES */','\n'.join(native)).replace('/* SHARED_CHECKBOX */',shared)
    with tempfile.TemporaryDirectory(prefix='indexed-setting-') as td:
        cpp=Path(td)/'test.cpp';binary=Path(td)/'test';cpp.write_text(fixture)
        flags=['-std=c++20','-O1','-g','-Wall','-Wextra','-Werror']
        if sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
        subprocess.run([os.environ.get('CXX','c++'),*flags,str(cpp),'-o',str(binary)],check=True)
        failed=False
        for mode in ('run','menu'):
            result=subprocess.run([str(binary),mode],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0','UBSAN_OPTIONS':'halt_on_error=1'})
            failed|=result.returncode!=0
        return int(failed)

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--sanitize',action='store_true')
    raise SystemExit(run(parser.parse_args().sanitize))
