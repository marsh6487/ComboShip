#!/usr/bin/env python3
"""Compile actual Slate teardown/input latch and native stow classifiers.

Native Player/PlayState layouts are real. Unrelated item cleanup and rune/UI
services are declared boundaries; persistent rune actors are never simulated as
being destroyed by stow. --ref executes the original classifier as a red control.
"""
from pathlib import Path
import subprocess, tempfile, os, argparse
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--ref');args=p.parse_args()
def read(path):
 return subprocess.check_output(['git','show',args.ref+':'+path],cwd=ROOT,text=True) if args.ref else (ROOT/path).read_text()
def block(s,marker):
 start=s.index(marker);i=s.index('{',start)+1;depth=1
 while depth:depth+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[start:i]
for game in ('soh','mm'):
 slate=read(game+'/mods/items/logic/item_sheikah_slate.c')
 stow=read(game+'/mods/items/custom_items_stow.c')
 before= 'void Slate_PutAway(' not in slate
 code='''#include "global.h"
#include "mods/items/custom_items.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
CustomItemState gCustomItemState;
static u8 sSlateDrawn,sSlatePrevRightHand;
static s16 sSlateHoldTimer,sSlateCastTimer;
static u16 sSlateSuppressedButtons, equipped=0x0002;
static u32 sSlateStowFrame=(u32)-1;
static u8 fireRodActive,fireRodFirstPerson,iceRodActive,iceRodFirstPerson,lightRodActive,lightRodFirstPerson;
static int sfx,closed,wheel,cryonis,sensor,highlight,placedBomb,placedPillar,frozenActor,bike;
static u8 sSensorPhase;
#define SENSOR_PHASE_IDLE 0
#define SENSOR_PHASE_PROMPT 1
static void Slate_OnWheelConfirm(s32 index) {}
static void OtherWheel(s32 index) {}
static void (*sBoxMOnConfirm)(s32);
static int BoxMenu_IsOpen(void) { return wheel; }
static void BoxMenu_Close(PlayState* play,u8 confirm) { assert(!confirm);wheel=0;sBoxMOnConfirm=NULL;++closed; }
static u16 Slate_EquippedButtonMask(void) { return equipped; }
static void Cryonis_ModeExit(u8 cancelled) { assert(!cancelled);cryonis=0; }
static u8 Cryonis_ModeActive(void) { return cryonis; }
static void Message_CloseTextbox(PlayState* play) { ++sensor; }
static void Sensor_Release(Player* player) { sSensorPhase=SENSOR_PHASE_IDLE; }
static void Stasis_UpdateOffer(PlayState* play,u8 enabled) { highlight=enabled; }
static void ItemEquip_PlayUnequipSFX(PlayState* play,Player* player) { ++sfx; }
static u8 Seasons_IsDrawn(void) { return 0; }
static u8 CustomItems_CanStowWhip(void) { return 0; }
static s32 Player_IsEnteringMittsClimb(const Player* player) { return 0; }
'''
 # global.h already declares Message_CloseTextbox with external linkage
 code=code.replace('static void Message_CloseTextbox','void Message_CloseTextbox')
 code+=block(slate,'u8 Slate_IsDrawn(')+'\n'
 code+=block(stow,'s32 CustomItems_HasStowableHeldItem(')+'\n'
 if before:
  code+='''int main(void) { static Player p; sSlateDrawn=1; assert(CustomItems_HasStowableHeldItem(&p) && "Slate missing from native put-away classifier"); }'''
 else:
  code+=block(slate,'static void Slate_Stow(')+'\n'+block(slate,'void Slate_PutAway(')+'\n'+block(slate,'static u8 Slate_InputSuppressed(')+'\n'
  code+='void Player_UseItem(PlayState* play,Player* player,s32 item) { assert(item==ITEM_NONE); Slate_PutAway(play,player); player->heldItemAction=PLAYER_IA_NONE; }\n'
  code+=block(read(game+'/src/overlays/actors/ovl_player_actor/z_player.c'),'s32 Player_PutAwayHeldItem(PlayState* play, Player* this) {')+'\n'
  code+='''int main(void) {
   static Player player; static PlayState play;
   play.gameplayFrames=7;player.rightHandType=7;sSlatePrevRightHand=2;
   sSlateDrawn=1;sSlateCastTimer=10;sSlateHoldTimer=8;wheel=1;sBoxMOnConfirm=Slate_OnWheelConfirm;
   cryonis=1;sSensorPhase=SENSOR_PHASE_PROMPT;highlight=1;
   placedBomb=placedPillar=frozenActor=bike=1;
   assert(CustomItems_HasStowableHeldItem(&player));
   play.state.input[0].cur.button=play.state.input[0].press.button=equipped;
   player.heldItemAction=PLAYER_IA_NONE;
   assert(Player_PutAwayHeldItem(&play,&player));
   assert(!Slate_IsDrawn() && !sSlateCastTimer && !sSlateHoldTimer);
   assert(!wheel && closed==1 && !cryonis && sSensorPhase==SENSOR_PHASE_IDLE && sensor==1 && !highlight);
   assert(placedBomb && placedPillar && frozenActor && bike);
   assert(sfx==1 && !CustomItems_HasStowableHeldItem(&player));
'''
  if game=='mm': code+='assert(player.rightHandType==sSlatePrevRightHand);\n'
  code+='''assert(Slate_InputSuppressed(&play));
   ++play.gameplayFrames;assert(Slate_InputSuppressed(&play));
   play.state.input[0].cur.button=0;play.state.input[0].press.button=0;
   ++play.gameplayFrames;assert(!Slate_InputSuppressed(&play));
   // An unrelated rune/item wheel belongs to its own controller.
   sSlateDrawn=1;wheel=1;sBoxMOnConfirm=OtherWheel;closed=0;
   Slate_PutAway(&play,&player);assert(wheel && !closed && sBoxMOnConfirm==OtherWheel);
   // Idempotent stow emits no extra sound or changes an unrelated modal.
   int oldSfx=sfx;Slate_PutAway(&play,&player);assert(sfx==oldSfx && wheel);
   puts("PASS actual Slate draw/grip/cast teardown, own modal targeting, ext input release, persistent rune effects");
  }'''
 for combo in (False,True):
  with tempfile.TemporaryDirectory(prefix=game+'-slate-stow-') as t:
   source=Path(t)/'test.c';source.write_text(code);binary=Path(t)/'test'
   includes=['.',game,game+'/include',game+'/include/PR',game+'/src',game+'/assets',game+('/soh' if game=='soh' else '/2s2h'),'libultraship/include','combo']
   cmd=[os.environ.get('CC','cc'),'-std=gnu17','-w','-Wno-error=incompatible-pointer-types','-Wno-error=int-conversion','-DF3DEX_GBI_2','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-no-pie',*(['-DCOMBO_BUILD'] if combo else []),*['-I'+str(ROOT/x) for x in includes],str(source),'-lm','-o',str(binary)]
   subprocess.run(cmd,check=True);subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
