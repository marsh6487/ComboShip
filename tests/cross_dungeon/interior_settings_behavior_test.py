#!/usr/bin/env python3
"""Exercise the production entrance menu section, including disabled preference retention."""
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
menu=(ROOT/'combo/gui/ComboMenu.cpp').read_text()
start=menu.index('    bool crossDungeonEntrances =')
end=menu.index('    ImGui::Separator();',start)
section=menu[start:end]
source=r'''
#include <cassert>
#include <map>
#include <string>
#include <vector>
static std::map<std::string,int> settings;
static std::map<std::string,bool> disabled;
static std::string click;
int CVarGetInteger(const char* key,int fallback) {auto it=settings.find(key);return it==settings.end()?fallback:it->second;}
void CVarSetInteger(const char* key,int value) {settings[key]=value;}
namespace ComboRando {void ComboMenu_PushCheckbox(int){} void ComboMenu_PopCheckbox(){}}
namespace ImGui {
static std::vector<bool> stack;
void BeginDisabled(bool value){stack.push_back(value||(!stack.empty()&&stack.back()));}
void EndDisabled(){stack.pop_back();}
bool Checkbox(const char* label,bool* value){
    const bool off=!stack.empty()&&stack.back();disabled[label]=off;
    if(!off&&click==label){*value=!*value;return true;}return false;
}
void SetItemTooltip(const char*,...){}
}
void Draw(){int goalTheme=0;
'''+section+r'''
}
int main(){
    Draw();
    const char* shops="Cross-Game Shops";
    const char* doors="Cross-Game Door Interiors";
    assert(disabled.count(shops)&&disabled.count(doors)&&"both narrow controls must be visible");
    assert(!disabled.at(shops)&&!disabled.at(doors));
    click=shops;Draw();assert(settings["gCombo.Rando.CrossShopEntrances"]==1);
    assert(settings["gCombo.Rando.CrossDoorInteriorEntrances"]==0);
    click=doors;Draw();assert(settings["gCombo.Rando.CrossDoorInteriorEntrances"]==1);
    click="Cross-Game Interiors";Draw();assert(settings["gCombo.Rando.CrossInteriorEntrances"]==1);
    assert(!disabled.at(shops)&&disabled.at(doors));
    click=shops;Draw();assert(settings["gCombo.Rando.CrossShopEntrances"]==0);
    click=doors;Draw();assert(settings["gCombo.Rando.CrossDoorInteriorEntrances"]==1);
    click="Cross-Game Interiors";Draw();assert(!disabled.at(shops)&&!disabled.at(doors));
    assert(!settings["gCombo.Rando.CrossShopEntrances"]&&settings["gCombo.Rando.CrossDoorInteriorEntrances"]);
}
'''
with tempfile.TemporaryDirectory(prefix='interior-menu-') as tmp:
    cpp=Path(tmp)/'test.cpp';binary=Path(tmp)/'test';cpp.write_text(source)
    subprocess.run(['g++','-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('entrance menu: independent shops, broad nonretail precedence and stored door preference PASS')
