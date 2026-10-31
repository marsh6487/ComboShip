from pathlib import Path
import re,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[2]
class Accents(unittest.TestCase):
    def test_both_menus_have_scoped_release_accents(self):
        groups=[]
        for game in ('soh','mm'):
            text=(ROOT/game/'mods/partners/Partners.cpp').read_text()
            groups.append(set(re.findall(r'\.CVar\("(gEnhancements\.(?:Partners|CustomEnemies)\.[^\"]+)"\)',text)))
        self.assertEqual(groups[0],groups[1]);self.assertEqual(len(groups[0]),9)
        with tempfile.TemporaryDirectory() as t:
            p=Path(t)
            (p/'imgui.h').write_text('''#pragma once
struct ImVec4{float x,y,z,w;ImVec4(float a,float b,float c,float d):x(a),y(b),z(c),w(d){}};
constexpr int ImGuiCol_Text=0,ImGuiHoveredFlags_AllowWhenDisabled=1;
namespace ImGui{inline int colors=0;inline bool BeginTabItem(const char*){return true;}inline void PushStyleColor(int,ImVec4){++colors;}inline void PopStyleColor(){--colors;}inline bool IsItemHovered(int){return false;}inline void SetTooltip(const char*,...){}}
''')
            code='#include "combo/gui/WhatsNew.h"\n#include <cassert>\nint main(){\n'
            for key in sorted(groups[0]):
                code+='assert(WhatsNew::Find("'+key+'")->kind==WhatsNew::Kind::New);\n'
                code+='{WhatsNew::Scope scope("'+key+'",nullptr,true);assert(ImGui::colors==1);}assert(ImGui::colors==0);\n'
                code+='{WhatsNew::Scope scope("'+key+'",nullptr,false);assert(ImGui::colors==0);}\n'
                code+='{WhatsNew::Scope scope("'+key+'",nullptr,true,true);assert(ImGui::colors==0);}\n'
            code+='assert(!WhatsNew::Find("gEnhancements.Partners.Unrelated"));}\n'
            (p/'t.cpp').write_text(code)
            subprocess.run(['c++','-std=c++17','-I'+str(p),'-I'+str(ROOT),str(p/'t.cpp'),'-o',str(p/'t')],check=True)
            subprocess.run([str(p/'t')],check=True)
if __name__=='__main__':unittest.main()
