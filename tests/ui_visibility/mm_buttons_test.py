#!/usr/bin/env python3
"""Execute MM's actual A/B Minimal-UI draw branches and contextual exceptions."""
from pathlib import Path
import os
import subprocess
import tempfile
from mm_draw_test import function

ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'mm/src/code/z_parameter.c').read_text()
fixture=(ROOT/'tests/ui_visibility/mm_interface_fixture.c').read_text().split('/* PRODUCTION */')[0]
for name in ['Interface_DrawAButton','Interface_DrawBButtonIcons']:
    fixture=fixture.replace(function(fixture,name).strip(),'')
fixture += r'''
static int itemIcons, ammo, actionLabels;
static s16 sAButtonDoActionTexScales[LANGUAGE_MAX] = {100,100,100,100,100};
static f32 sBButtonDoActionTextureScales[LANGUAGE_MAX] = {100,100,100,100,100};
static s16 sBButtonDoActionTextureScale;
static s16 sBButtonDoActionXPositions[LANGUAGE_MAX], sBButtonDoActionYPositions[LANGUAGE_MAX];
u8 MasterCycle_IsRiding(void) { return 0; }
#define PLAYER_ON_REAL_HORSE(player) (((player)->stateFlags1 & PLAYER_STATE1_800000) && !MasterCycle_IsRiding())
void Interface_DrawItemIconTexture(PlayState* play, TexturePtr tex, s16 button) { ++itemIcons; }
void Interface_DrawAmmoCount(PlayState* play, s16 button, s16 alpha) { ++ammo; }
void Interface_SetPerspectiveView(PlayState* play, s16 y1,s16 y2,s16 x1,s16 x2) {}
void Matrix_RotateXFApply(f32 x) {}
Gfx* Gfx_DrawTexQuad4b(Gfx* p, TexturePtr tex, s32 fmt,s16 w,s16 h,u16 point) { ++actionLabels; return p+1; }
'''
# Count the actual shadow-texture draw boundary rather than mirroring its branch.
fixture=fixture.replace('Gfx* Gfx_DrawTexQuadIA8(Gfx* p, TexturePtr tex, s16 w, s16 h, u16 point) { return p+1; }',
                        'static int shadowCalls;\nGfx* Gfx_DrawTexQuadIA8(Gfx* p, TexturePtr tex, s16 w, s16 h, u16 point) { ++shadowCalls; return p+1; }')
fixture+='\n'.join(function(source,name) for name in ['Interface_IsFullUi','Interface_DrawBButtonIcons','Interface_DrawAButton'])
fixture+=r'''
int main(void) {
 GraphicsContext gfx={0}; PlayState play={0}; Player player={0};
 ActionLabel labels[2]={0}; char* icons[4]={0};
 play.state.gfxCtx=&gfx; play.actorCtx.actorLists[ACTORCAT_PLAYER].first=&player.actor;
 play.interfaceCtx.actionVtx=vertices; play.interfaceCtx.doActionSegment=labels; play.interfaceCtx.iconItemSegment=icons;
 player.transformation=PLAYER_FORM_HUMAN; gSaveContext.save.playerForm=PLAYER_FORM_HUMAN;
 BUTTON_ITEM_EQUIP(CUR_FORM,EQUIP_SLOT_B)=ITEM_SWORD_KOKIRI;
 gfx.work.p=work; gfx.polyOpa.p=buffer; gfx.polyXlu.p=xlu; gfx.overlay.p=overlay; gfx.debug.p=debug;
 minimal=1; R_MINIMAP_DISABLED=1;
 Interface_DrawAButton(&play); assert(shadowCalls==0 && actionLabels==1);
 Interface_DrawBButtonIcons(&play); assert(itemIcons==0);
 player.stateFlags1|=PLAYER_STATE1_800000;
 Interface_DrawBButtonIcons(&play); assert(itemIcons==1 && ammo==1);
 player.stateFlags1=0;
 play.bButtonAmmoPlusOne=3;
 Interface_DrawBButtonIcons(&play); assert(itemIcons==2);
 play.bButtonAmmoPlusOne=0;
 enabled=1;
 Interface_DrawAButton(&play); assert(shadowCalls==1 && actionLabels==2);
 Interface_DrawBButtonIcons(&play); assert(itemIcons==3);
 enabled=0; play.pauseCtx.state=PAUSE_STATE_MAIN;
 Interface_DrawAButton(&play); assert(shadowCalls==2 && actionLabels==3);
 puts("PASS native MM Minimal A/B draws: action labels, backgrounds, horse/minigame, priority and pause");
}
'''
with tempfile.TemporaryDirectory(prefix='mm-ui-buttons-') as directory:
    path=Path(directory)/'test.c';path.write_text(fixture);binary=Path(directory)/'test'
    command=['cc','-std=gnu11','-g','-w','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t',
             '-DLOG_LEVEL_GAME_PRINTS=0','-Wno-int-conversion','-Wno-incompatible-pointer-types',
             '-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',
             *['-I'+str(ROOT/p) for p in ['mm/include','mm/include/PR','mm/src','mm/assets','mm',
                                        'mm/2s2h','libultraship/include','combo']],str(path),'-lm','-o',str(binary)]
    result=subprocess.run(command,capture_output=True,text=True)
    if result.returncode:
        print('\n'.join(line for line in result.stderr.splitlines() if 'error:' in line or 'undefined reference' in line))
        result.check_returncode()
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
