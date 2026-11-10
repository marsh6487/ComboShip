#!/usr/bin/env python3
"""Source-only seven-placement audit; actual native C mode/collision proof, never a geometry/ROM proof."""
from pathlib import Path
import argparse, hashlib, os, re, subprocess, tempfile, xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[3]
PIN='0c0f8a8e1412f3bc979e77a2563293770d58a439'
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--upstream-root',type=Path,default=ROOT.parent/'firstdrops-tooling'/('OoTMM-'+PIN))
p.add_argument('--sanitize',action='store_true')
args=p.parse_args()
up=args.upstream_root
xml=up/'data/checks/mm/dungeons/snowhead_temple.xml'
world=up/'data/world/mm/dungeons/snowhead_temple.yml'
macro=up/'data/macros/macros_mm.yml'
actor=ROOT/'mm/src/overlays/actors/ovl_Bg_Icicle/z_bg_icicle.c'
header=ROOT/'mm/src/overlays/actors/ovl_Bg_Icicle/z_bg_icicle.h'
collision=ROOT/'mm/src/code/z_collision_check.c'
checks=ROOT/'mm/2s2h/Rando/MMEnvironmentIcicles.inc'
regions=ROOT/'mm/2s2h/Rando/Logic/Regions/SnowheadTemple.cpp'
rows=[]
for node in ET.parse(xml).iter('xflag'):
 if node.attrib.get('location','').startswith('Snowhead Temple Icicle Entrance'):
  assert node.attrib['type']=='icicle'
  assert not ({'params','position','x','y','z'} & node.attrib.keys()), 'New placement metadata needs audit'
  rows.append((node.attrib['location'],int(node.attrib['room'],0),int(node.attrib['actor'],0)))
expected=[('Snowhead Temple Icicle Entrance '+str(i+1),0,index) for i,index in enumerate((0,1,4,5,6))]+[
 ('Snowhead Temple Icicle Entrance After Block '+str(i+1),0,index) for i,index in enumerate((2,3))]
assert rows==expected, rows
native_rows=re.findall(r'MM_ICICLE\((RC_\w+), SCENE_HAKUGIN, (\d+), (\d+), (RR_\w+)\)',checks.read_text())
assert len(native_rows)==7
for (name,room,index),(id,nroom,nindex,region) in zip(expected,native_rows):
 assert ' '.join(id.removeprefix('RC_').split('_')).title()==name
 assert (int(nroom),int(nindex))==(room,index)
 assert region=='RR_SNOWHEAD_TEMPLE_ENTRANCE_'+('AFTER' if 'After Block' in name else 'BEFORE')+'_BLOCK'
 assert id in regions.read_text()
print('PASS exact seven pinned XML identities; source XML contains no authored actor params or coordinates')
assert '"break_icicle": "has_nuts || has_arrows || can_use_slingshot || can_use_din || can_fight || has_mask_deku || has_explosives"' in macro.read_text()
assert 'has_mask_goron || has_mask_zora || (break_icicle && can_lift_silver)' in world.read_text()
assert 'this->collider.elem.atDmgInfo.dmgFlags = 0x00000080; // DMG_HOOKSHOT' in (ROOT/'mm/src/overlays/actors/ovl_Arms_Hook/z_arms_hook.c').read_text()
assert '{ DMG_DEKU_STICK, 0, 0, 2, 4 }' in (ROOT/'mm/src/overlays/actors/ovl_player_actor/z_player.c').read_text()
# Extract just the production collision damage-mask predicate, compile as C unchanged.
def function(source,name):
 match=re.search(r'^s32 '+name+r'\([^;]+?\)\s*\{',source,re.M)
 assert match,name
 start=match.start();begin=source.index('{',match.start());level=1;end=begin+1
 while level:
  level+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[start:end]
driver=r'''
#include "global.h"
#include "src/overlays/actors/ovl_Bg_Icicle/z_bg_icicle.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
void BgIcicle_Init(Actor*,PlayState*);
void BgIcicle_Wait(BgIcicle*,PlayState*);
void BgIcicle_DoNothing(BgIcicle*,PlayState*);
void BgIcicle_Shiver(BgIcicle*,PlayState*);
void BgIcicle_Fall(BgIcicle*,PlayState*);
void BgIcicle_Regrow(BgIcicle*,PlayState*);
void BgIcicle_UpdateAttacked(BgIcicle*,PlayState*);
static int breaks,kills,drops;
void Actor_ProcessInitChain(Actor* a,InitChainEntry* c){}
void DynaPolyActor_Init(DynaPolyActor* a,s32 f){}
void DynaPolyActor_LoadMesh(PlayState* p,DynaPolyActor* a,CollisionHeader* h){}
s32 Collider_InitAndSetCylinder(PlayState* p,ColliderCylinder* c,Actor* a,ColliderCylinderInit* s){
 c->elem.acDmgInfo.dmgFlags=s->elem.acDmgInfo.dmgFlags;return 1;
}
void Collider_UpdateCylinder(Actor* a,ColliderCylinder* c){}
void Actor_PlaySfx(Actor* a,u16 sound){}
s32 CollisionCheck_SetAT(PlayState* p,CollisionCheckContext* c,Collider* a){return 0;}
void DynaPoly_DisableCollision(PlayState* p,DynaCollisionContext* c,s32 id){}
void DynaPoly_EnableCollision(PlayState* p,DynaCollisionContext* c,s32 id){}
void Actor_Kill(Actor* a){++kills;}
void DynaPoly_DeleteBgActor(PlayState* p,DynaCollisionContext* c,s32 id){}
s32 Collider_DestroyCylinder(PlayState* p,ColliderCylinder* c){return 0;}
s32 CollisionCheck_SetAC(PlayState* p,CollisionCheckContext* c,Collider* a){return 0;}
void Gfx_DrawDListOpa(PlayState* p,Gfx* d){}
void Actor_MoveWithGravity(Actor* a){}
void Actor_UpdateBgCheckInfo(PlayState* p,Actor* a,f32 h,f32 r,f32 y,u32 flags){}
s32 Math_StepToF(f32* v,f32 target,f32 step){*v=target;return 1;}
void SoundSource_PlaySfxAtFixedWorldPos(PlayState* p,Vec3f* v,u32 d,u16 s){}
f32 Rand_CenteredFloat(f32 f){return 0;}
f32 Rand_ZeroOne(void){return 0;}
void EffectSsEnIce_Spawn(PlayState* p,Vec3f* pos,f32 scale,Vec3f* vel,Vec3f* accel,Color_RGBA8* prim,Color_RGBA8* env,s32 life){}
void Item_DropCollectibleRandom(PlayState* p,Actor* a,Vec3f* v,s16 params){++drops;}
Actor* Item_DropCollectible(PlayState* p,Vec3f* v,u32 params){++drops;return NULL;}
s32 func_800A8150(s32 id){return 0;}
bool GameInteractor_Should(GIVanillaBehavior flag,uint32_t result,...){
 if(flag==VB_ICICLE_DROP_COLLECTIBLE)++breaks;
 else assert(flag==VB_CHECK_BUMPER_COLLISION);
 return result;
}
'''+function(collision.read_text(),'CollisionCheck_NoSharedFlags')+r'''
int main(void){
 static PlayState play;
 for(int type=0;type<4;++type){
  BgIcicle ice={0};ice.dyna.actor.params=(0xAB<<8)|(0x12<<2)|type;
  BgIcicle_Init(&ice.dyna.actor,&play);
  assert(ice.dyna.actor.params==type);
  assert(ice.unk_161==0xAB);
  assert(ice.collider.elem.acDmgInfo.dmgFlags==0xF7CFFFFF);
  if(type==ICICLE_STALAGMITE_RANDOM_DROP){assert(ice.unk_160==0xAB);assert(ice.actionFunc==BgIcicle_DoNothing);}
  else if(type==ICICLE_STALAGMITE_FIXED_DROP){assert(ice.unk_160==0x12);assert(ice.actionFunc==BgIcicle_DoNothing);}
  else{
   assert(ice.actionFunc==BgIcicle_Wait);
   ice.dyna.actor.xzDistToPlayer=60;BgIcicle_Wait(&ice,&play);assert(ice.actionFunc==BgIcicle_Wait);
   ice.dyna.actor.xzDistToPlayer=59;BgIcicle_Wait(&ice,&play);assert(ice.actionFunc==BgIcicle_Shiver);
   assert(ice.shiverTimer==10);
   for(int frame=0;frame<10;++frame)BgIcicle_Shiver(&ice,&play);
   assert(ice.actionFunc==BgIcicle_Fall);
   int previous=breaks,previousKills=kills;ice.dyna.actor.bgCheckFlags=BGCHECKFLAG_GROUND;
   BgIcicle_Fall(&ice,&play);assert(breaks==previous+1);
   assert(kills==previousKills+(type!=ICICLE_STALACTITE_REGROW));
  }
  const u32 accepted[]={DMG_HOOKSHOT,DMG_DEKU_STICK,DMG_DEKU_NUT,DMG_NORMAL_ARROW,DMG_SWORD,DMG_DEKU_SPIN,DMG_ZORA_PUNCH,DMG_ZORA_BOOMERANG,DMG_GORON_PUNCH,DMG_EXPLOSIVES};
  for(int i=0;i<(int)(sizeof(accepted)/sizeof(*accepted));++i){
   ColliderElement attack={0};attack.atDmgInfo.dmgFlags=accepted[i];
   assert(!CollisionCheck_NoSharedFlags(&attack,&ice.collider.elem));
   int previous=breaks;ice.collider.base.acFlags|=AC_HIT;BgIcicle_UpdateAttacked(&ice,&play);assert(breaks==previous+1);
  }
  ColliderElement shield={0};shield.atDmgInfo.dmgFlags=DMG_NORMAL_SHIELD;assert(CollisionCheck_NoSharedFlags(&shield,&ice.collider.elem));
 }
 puts("PASS actual native C init modes, proximity Wait/Shiver/Fall and collision-mask Hookshot/Stick/form weapon acceptance; physical contact is a service boundary");
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='icicle-access-audit-') as temp:
 temp=Path(temp);src=temp/'audit.c';src.write_text(driver)
 flags=['-std=c11','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-DCONTROLLERBUTTONS_T=uint32_t',
 '-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I'+str(ROOT)]
 for path in ('mm/include','mm/include/PR','mm/2s2h','mm','mm/src','mm/assets','libultraship/include','combo','combo/menu'):
  flags+=['-isystem',str(ROOT/path)]
 if args.sanitize:flags+=['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
 for combo in (False,True):
  out=temp/('combo' if combo else 'original')
  subprocess.run([os.environ.get('CC','cc'),*flags,*(['-DCOMBO_BUILD'] if combo else []),str(src),str(actor),'-o',str(out)],check=True)
  subprocess.run([str(out)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
for path in (xml,world,macro,actor,header,collision,checks,regions):
 print(hashlib.sha256(path.read_bytes()).hexdigest(),str(path.relative_to(ROOT) if path.is_relative_to(ROOT) else path.relative_to(up)))
print('AUDIT LIMIT: no per-placement params or geometry present in inspected source; no seven-location physical reachability or full graph claim')
