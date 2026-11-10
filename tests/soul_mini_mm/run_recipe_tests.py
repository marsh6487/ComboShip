#!/usr/bin/env python3
"""Compile actual MM recipe functions in a native-header-free fixture; optional JSON dump."""
import json,pathlib,re,subprocess,tempfile,sys
root=pathlib.Path(__file__).resolve().parents[2]
s=(root/'combo/menu/ComboItemDrawMM.h').read_text()
enemy=re.findall(r'\bRI_SOUL_ENEMY_\w+\b',(root/'mm/2s2h/Rando/Types.h').read_text());enemy=list(dict.fromkeys(enemy))
rows=json.loads((root/'tests/soul_mini_mm/expected_npcs.json').read_text())
ids=enemy+['RI_SOUL_BOSS_GOHT','RI_SOUL_BOSS_GYORG','RI_SOUL_BOSS_MAJORA','RI_SOUL_BOSS_ODOLWA','RI_SOUL_BOSS_TWINMOLD']+['RI_SOUL_NPC_'+r['key'] for r in rows]
prefix='#include <cassert>\n#include <iostream>\n#include <cstring>\n#include "ComboItemDrawABI.h"\n'
prefix+='enum RandoItemId {'+','.join(ids)+', RI_UNKNOWN};\n'
prefix+='namespace ComboSoulModels {bool enemy=false,npc=false; bool SimplerEnemy(){return enemy;} bool SimplerNpc(){return npc;}}\n'
includes=re.findall(r'#include "((?:assets/)?objects/[^\"]+)"',s)
for inc in dict.fromkeys(includes+['objects/object_bigslime/object_bigslime.h']):prefix+='#include "'+inc+'"\n'
# Extract whole production blocks, retaining the real header and every special switch arm.
start=s.index('struct MmSoulModel {');end=s.index('// Native MM boss models whose draw routines fit')
prefix+=s[start:end]
start=s.index('static int32_t MM_FillEnemySoulAnim(');end=s.index('// DrawMinifrog:',start)
prefix+=s[start:end]
prefix+='\nstruct Expected {RandoItemId id;const char* name;const char* skel;const char* anim;int limbs;};\nExpected expected[]={\n'
for r in rows:prefix+='{RI_SOUL_NPC_'+r['key']+','+json.dumps(r['key'])+','+json.dumps(r['skel'])+','+(json.dumps(r['anim']) if r['anim'] else 'nullptr')+','+str(r['limbCount'])+'},\n'
prefix+='};\nconst char* enemyNames[]={'+','.join(json.dumps(x) for x in enemy)+'};\n'
def function(text, signature):
 start=text.index(signature); brace=text.index('{',start); depth=1;end=brace+1
 while depth:
  if text[end]=='{':depth+=1
  if text[end]=='}':depth-=1
  end+=1
 return text[start:end]
draw=(root/'mm/2s2h/Rando/DrawItem.cpp').read_text()
prefix+="\n#include <map>\n#include <functional>\n#define COMBO_BUILD\n"
prefix+="static void* gPlayState=(void*)1; static int pushes=0,pops=0,rendered=0,tokens=0,legacy=0; static bool failRenderer=false;\n"
prefix+="static void Matrix_Push(){++pushes;}static void Matrix_Pop(){++pops;}\nstatic int ComboForeignAnim_Draw(const CwItemAnimDrawInfo*,const char*,void*){++rendered;return !failRenderer;}\n"
prefix+="extern \"C\" int MM_GetSoulMiniAnimDrawInfoById(int32_t id,CwItemAnimDrawInfo* out){*out={};return MM_FillNpcSoulAnim((RandoItemId)id,out)||MM_FillEnemySoulAnim((RandoItemId)id,out);}\n"
prefix+=function(draw,'static int32_t ComboDrawNativeMmSoul(')
prefix+='\nstatic void DrawShuffledSoul(){++tokens;}\nstatic std::map<RandoItemId,std::function<void()>> soulDrawMap;\n'
prefix+=function(draw,'void DrawSoul(RandoItemId randoItemId)')
prefix+=(root/'tests/soul_mini_mm/recipe_fixture.cpp').read_text()
with tempfile.TemporaryDirectory(prefix='mm-soul-recipes-') as d:
 p=pathlib.Path(d);(p/'test.cpp').write_text(prefix)
 subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror','-I'+str(root/'combo/menu'),'-I'+str(root/'mm'),'-I'+str(root/'mm/assets'),'-I'+str(root/'mm/include'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')]+sys.argv[1:],check=True)
