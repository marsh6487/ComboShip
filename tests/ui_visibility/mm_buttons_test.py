#!/usr/bin/env python3
"""Execute MM's actual A/B Minimal-UI draw branches and contextual exceptions."""
from pathlib import Path
import os
import subprocess
import tempfile
from mm_draw_test import function

ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'mm/src/code/z_parameter.c').read_text()
player_source=(ROOT/'mm/src/overlays/actors/ovl_player_actor/z_player.c').read_text()
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
u16 gEquipMasks[4];
u8 gEquipShifts[4];
u8 gPlayerFormItemRestrictions[PLAYER_FORM_MAX][114];
static int flightItemRequests, flowerLandings, iconLoads;
static s32 Player_ActionHandler_13(Player* player, PlayState* play) {
    ++flightItemRequests;
    return true;
}
static void func_80837134(PlayState* play, Player* player) {
    ++flowerLandings;
    player->actionFunc = NULL;
    player->stateFlags3 = 0;
}
s16 CutsceneManager_GetCurrentCsId(void) { return CS_ID_NONE; }
s32 Player_GetEnvironmentalHazard(PlayState* play) { return PLAYER_ENV_HAZARD_NONE; }
void Interface_LoadItemIconImpl(PlayState* play, u8 button) { ++iconLoads; }
'''
# Count the actual shadow-texture draw boundary rather than mirroring its branch.
fixture=fixture.replace(function(fixture,'Gfx_DrawTexQuadIA8').strip(),
                        'static int shadowCalls;\nGfx* Gfx_DrawTexQuadIA8(Gfx* p, TexturePtr tex, s16 w, s16 h, u16 point) { ++shadowCalls; return p+1; }')
action=function(player_source,'Player_Action_94')
# Exercise the live landing and pending-action dispatch before airborne movement.
fixture+=action[:action.index('    if ((this->actor.velocity.y > 0.0f)')]+ '}\n'
fixture+=function(player_source,'Player_IsDekuFlowerFlying')
fixture+='\n'.join(function(source,name) for name in ['Interface_IsFullUi','Interface_DrawBButtonIcons','Interface_DrawAButton',
                                                       'Interface_UpdateDekuFlightBButton','Interface_UpdateButtonsPart2'])
fixture+=r'''
static void CheckDekuFlower(PlayState* play, Player* player) {
    player->actor.id = ACTOR_PLAYER;
    player->transformation = gSaveContext.save.playerForm = PLAYER_FORM_DEKU;
    player->actionFunc = Player_Action_94;
    player->stateFlags3 = PLAYER_STATE3_2000 | PLAYER_STATE3_1000000;
    play->sceneId = SCENE_00KEIKOKU;
    play->activeCamId = CAM_ID_MAIN;
    play->pauseCtx.state = PAUSE_STATE_OFF;
    play->interfaceCtx.bButtonPlayerDoAction = DO_ACTION_SHOOT;
    play->interfaceCtx.bButtonPlayerDoActionActive = true;
    gPlayerFormItemRestrictions[PLAYER_FORM_DEKU][ITEM_OCARINA_OF_TIME] = true;
    SET_CUR_FORM_BTN_ITEM(EQUIP_SLOT_C_LEFT, ITEM_OCARINA_OF_TIME);
    DPAD_SET_CUR_FORM_BTN_ITEM(EQUIP_SLOT_D_RIGHT, ITEM_OCARINA_OF_TIME);
    BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_FD;
    gSaveContext.save.saveInfo.inventory.items[SLOT_DEKU_NUT] = ITEM_DEKU_NUT;
    const int originalIcons = itemIcons;

    Player_Action_94(player, play);
    assert(flightItemRequests == 1 && flowerLandings == 0);
    Interface_UpdateButtonsPart2(play);
    Interface_DrawBButtonIcons(play);
    assert(itemIcons == originalIcons + 1 && iconLoads == 1);
    assert(BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) == ITEM_DEKU_NUT);
    assert(gSaveContext.buttonStatus[EQUIP_SLOT_B] == BTN_ENABLED);
    assert(gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT] == BTN_DISABLED);
    assert(gSaveContext.shipSaveContext.dpad.status[EQUIP_SLOT_D_RIGHT] == BTN_DISABLED);

    // Simulate ground contact before the player action has had an update.
    player->actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    Interface_UpdateButtonsPart2(play);
    play->interfaceCtx.bButtonPlayerDoActionActive = false;
    Interface_DrawBButtonIcons(play);
    assert(itemIcons == originalIcons + 1 && iconLoads == 2);
    assert(BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) == ITEM_FD);
    assert(gSaveContext.buttonStatus[EQUIP_SLOT_B] == BTN_DISABLED);
    assert(gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT] == BTN_ENABLED);
    assert(gSaveContext.shipSaveContext.dpad.status[EQUIP_SLOT_D_RIGHT] == BTN_ENABLED);
    assert(gSaveContext.hudVisibility == HUD_VISIBILITY_ALL);
    Player_Action_94(player, play);
    assert(flightItemRequests == 1 && flowerLandings == 1);

    // Magic unlocks ground bubbles, but the playground still controls C/D visibility.
    gSaveContext.save.saveInfo.playerData.isMagicAcquired = true;
    Interface_UpdateButtonsPart2(play);
    assert(BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) == ITEM_DEKU_NUT);
    assert(gSaveContext.buttonStatus[EQUIP_SLOT_B] == BTN_ENABLED);
    gSaveContext.eventInf[EVENTINF_34 >> 4] |= 1 << (EVENTINF_34 & 0xF);
    Interface_UpdateButtonsPart2(play);
    assert(gSaveContext.hudVisibility == HUD_VISIBILITY_A_B_MINIMAP);
    assert(gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT] == BTN_DISABLED);
    assert(gSaveContext.shipSaveContext.dpad.status[EQUIP_SLOT_D_RIGHT] == BTN_DISABLED);
}

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
 CheckDekuFlower(&play, &player);
 puts("PASS native MM Minimal A/B draws and Deku flight, landing, magic and playground buttons");
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
