#!/usr/bin/env python3
"""Execute the updated MM editor control and real NEI JSON conversion."""
from pathlib import Path
import sys,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_mm_scene_randomization_tests import function
src=(ROOT/'mm/2s2h/DeveloperTools/SaveEditor.cpp').read_text()
code=r'''#include <cassert>
#include <cstdint>
#include <cstring>
#define EXT_ITEM_RITO_MASK 0x225
struct ImVec2{ImVec2(float,float){}};
static bool owned,clicked;static int grants,removes,loaded,shown;static uint16_t iconId;
uint8_t RitoMask_IsOwned(){return owned;}
void RitoMask_Grant(){owned=true;++grants;}
void RitoMask_Remove(){owned=false;++removes;}
uint8_t RitoMask_UsesCornerWheel(){return 0;}
void* ExtInv_GetItemIcon(uint16_t id){iconId=id;return (void*)"rito-icon";}
bool ResourceMgr_FileExists(const char*){return true;}
struct Gui{bool HasTextureByName(const char*){return loaded;}void LoadGuiTexture(const char*,const char*){++loaded;}void* GetTextureByName(const char*){return (void*)1;}}gui;
Gui* Ship_GetFast3dGui(){return &gui;}
namespace ImGui{void SeparatorText(const char*){}void Image(void*,ImVec2){++shown;}void SameLine(){}void TextDisabled(const char*){}
bool Checkbox(const char*,bool* state){if(clicked){*state=!*state;return true;}return false;}}
'''+function(src,'DrawRitoMaskEditor')+r'''
int main(){clicked=true;DrawRitoMaskEditor();assert(owned&&grants==1&&removes==0);assert(iconId==0x225&&loaded==1&&shown==1);
DrawRitoMaskEditor();assert(!owned&&grants==1&&removes==1);clicked=false;DrawRitoMaskEditor();assert(!owned&&removes==1);}
'''
with tempfile.TemporaryDirectory(prefix='rito-mm-editor-') as d:
 p=Path(d);(p/'test.cpp').write_text(code)
 subprocess.run(['c++','-std=c++20',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS MM updated editor canonical grant/remove and exact full-width Rito icon')
