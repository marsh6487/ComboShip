#!/usr/bin/env python3
"""Run real native mask-use routing before any vanilla action conversion."""
from pathlib import Path
import subprocess, tempfile, sys
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_mm_scene_randomization_tests import function
src=(ROOT/'mm/mods/forms/custom_forms.cpp').read_text()
code=r'''#include <cassert>
#include <cstdint>
using u8=uint8_t; using s32=int; using ItemId=int;
struct Player{};
#define ITEM_OOT_MASK_PLACEHOLDER 0xB0
#define EXT_ITEM_RITO_MASK 0x225
#define CUSTOM_FORM_GERUDO 2
#define CUSTOM_FORM_RITO 4
#define OOT_MASK_WHEEL_GERUDO 6
#define NA_SE_PL_CHANGE_ARMS 1
struct FormDef{const char* cvarName;int cvarDefault;};
FormDef sForms[5]={{"",1},{"",1},{"",1},{"",1},{"",1}};
int toggled=-1,owned=1;
int OotMask_CursorIndex(){return -1;}
int CVarGetInteger(const char*,int def){return def;}
u8 RitoMask_IsOwned(){return owned;}
void CustomForms_Toggle(int f){toggled=f;}
void func_8082E1F0(Player*,int){}
'''+function(src,'CustomForms_UseItem')+r'''
int main(){ Player p; assert(CustomForms_UseItem(&p,0x225)==1); assert(toggled==4);
owned=0;toggled=-1;assert(CustomForms_UseItem(&p,0x225)==1);assert(toggled==-1);
assert(CustomForms_UseItem(&p,0xA1)==0); }
'''
with tempfile.TemporaryDirectory(prefix='rito-mm-form-') as d:
 p=Path(d);(p/'test.cpp').write_text(code)
 subprocess.run(['c++','-std=c++20',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS MM full-width owned mask toggles Rito and unowned mask cannot activate')
