#include "global.h"
#include "variables.h"

#include <assert.h>
#include <stdio.h>

static PlayState sPlay;
static Player sPlayer;
static CollisionPoly sFloor;
static bool sAdultRig;
static bool sMinish;
static bool sPikachu;
static bool sIronBoots;
static bool sEnteredSwim;
static bool sStartedStanding;
static u8 sAudioFilter;
static PlayerAnimationHeader gPlayerAnim_link_swimer_swim;
static PlayerAnimationHeader gPlayerAnim_link_swimer_swim_down;
SaveContext gSaveContext;

s32 AdultLink_UsesAdultItemPresentation(const Player* player) {
    return player == &sPlayer && player->transformation == PLAYER_FORM_HUMAN && sAdultRig;
}
s32 MinishTiny_IsActive(void) { return sMinish; }
s32 PikachuForm_IsSelected(void) { return sPikachu; }
bool VanillaTB_IsIronBoots(void) { return sIronBoots; }
s32 Nei_IsZoraSwim(Player* player) { return false; }
u8 ExtEquip_SpiritHasMoney(void) { return false; }
void Audio_SetBaseFilter(u8 filter) { sAudioFilter = filter; }
void Audio_PlaySfx(u16 id) {}
void Player_PlaySfx(Player* player, u16 id) {}
void func_80123140(PlayState* play, Player* player) {}
void func_80169EFC(PlayState* play) {}
void func_808345C8(void) {}
void func_8083B8D0(PlayState* play, Player* player) {}
void func_80834140(PlayState* play, Player* player, PlayerAnimationHeader* anim) {}
s32 func_808373F8(PlayState* play, Player* player, u16 sound) { return false; }
s32 func_80837730(PlayState* play, Player* player, f32 speed, s32 scale) { return false; }
void Player_Anim_PlayLoopSlowMorph(PlayState* play, Player* player, PlayerAnimationHeader* anim) {}
s32 WaterBox_GetSurface1(PlayState* play, CollisionContext* colCtx, f32 x, f32 z, f32* height, WaterBox** box) {
    *height = 0.0f;
    return true;
}
f32 BgCheck_EntityRaycastFloor5(CollisionContext* colCtx, CollisionPoly** outPoly, s32* outBgId, Actor* actor,
                               Vec3f* pos) {
    *outPoly = actor->floorPoly;
    return actor->floorHeight;
}

#define ACTION(number) static void Player_Action_##number(Player* player, PlayState* play) {}
ACTION(1)
ACTION(28)
ACTION(33)
ACTION(43)
ACTION(49)
ACTION(54)
ACTION(55)
ACTION(56)
ACTION(57)
ACTION(58)
ACTION(59)
ACTION(60)
ACTION(61)
ACTION(62)
#undef ACTION

s32 Player_SetAction(PlayState* play, Player* player, PlayerActionFunc action, s32 arg3) {
    player->actionFunc = action;
    return true;
}
void func_80835324(PlayState* play, Player* player, f32 speed, s16 yaw) {
    sStartedStanding = true;
}
void Player_SetupTurnInPlace(PlayState* play, Player* player, s16 yaw) {
    sStartedStanding = true;
}
void func_8083B930(PlayState* play, Player* player) {
    sEnteredSwim = true;
    player->stateFlags1 |= PLAYER_STATE1_8000000;
    player->actionFunc = Player_Action_54;
}

#include "production.inc"

static void PlacePlayer(s32 form, f32 footDepth, f32 floorDepth, bool isSwimming) {
    sPlayer = (Player){0};
    sPlayer.transformation = form;
    sPlayer.ageProperties = &sPlayerAgeProperties[form];
    sPlayer.currentBoots = (form == PLAYER_FORM_ZORA) ? PLAYER_BOOTS_ZORA_LAND : PLAYER_BOOTS_HYLIAN;
    sPlayer.subCamId = CAM_ID_NONE;
    sPlayer.actor.world.pos.y = -footDepth;
    sPlayer.actor.depthInWater = footDepth;
    sPlayer.actor.floorHeight = -floorDepth;
    sPlayer.actor.floorPoly = &sFloor;
    sPlayer.actor.bgCheckFlags = BGCHECKFLAG_WATER | ((footDepth == floorDepth) ? BGCHECKFLAG_GROUND : 0);
    if (isSwimming) {
        sPlayer.stateFlags1 = PLAYER_STATE1_8000000;
        sPlayer.stateFlags2 = PLAYER_STATE2_400;
        sPlayer.actionFunc = Player_Action_54;
    }
    sEnteredSwim = sStartedStanding = false;
}

int main(void) {
    sAdultRig = true;
    PlacePlayer(PLAYER_FORM_HUMAN, 40.0f, 40.0f, false);
    func_8083BB4C(&sPlay, &sPlayer);
    assert(!sEnteredSwim && sAudioFilter == 0 && "adult should wade on the swamp floor");
    assert(func_8083A878(&sPlay, &sPlayer, 180.0f) && sStartedStanding);
    PlacePlayer(PLAYER_FORM_HUMAN, 40.0f, 120.0f, false);
    assert(!func_8083A878(&sPlay, &sPlayer, 180.0f) && sPlayer.actionFunc == Player_Action_55);

    PlacePlayer(PLAYER_FORM_HUMAN, 56.0f, 56.0f, false);
    func_8083BB4C(&sPlay, &sPlayer);
    assert(!sEnteredSwim);
    PlacePlayer(PLAYER_FORM_HUMAN, 60.0f, 120.0f, false);
    func_8083BB4C(&sPlay, &sPlayer);
    assert(sEnteredSwim);
    PlacePlayer(PLAYER_FORM_HUMAN, 45.0f, 120.0f, true);
    func_808475B4(&sPlayer);
    assert(sPlayer.actor.velocity.y < 0.0f); // Approaches the adult float depth.
    func_8083BB4C(&sPlay, &sPlayer);
    assert(!sStartedStanding && (sPlayer.stateFlags1 & PLAYER_STATE1_8000000));
    PlacePlayer(PLAYER_FORM_HUMAN, 39.0f, 40.0f, true);
    func_8083BB4C(&sPlay, &sPlayer);
    assert(sStartedStanding && !(sPlayer.stateFlags1 & PLAYER_STATE1_8000000));
    assert(!(sPlayer.stateFlags2 & PLAYER_STATE2_400));

    sIronBoots = true;
    PlacePlayer(PLAYER_FORM_HUMAN, 40.0f, 40.0f, true);
    func_8083BB4C(&sPlay, &sPlayer);
    assert(!sStartedStanding && sPlayer.currentBoots == PLAYER_BOOTS_ZORA_UNDERWATER);
    sIronBoots = false;

    for (int mode = 0; mode < 3; mode++) {
        sAdultRig = mode != 0;
        sMinish = mode == 1;
        sPikachu = mode == 2;
        PlacePlayer(PLAYER_FORM_HUMAN, 40.0f, 40.0f, false);
        func_8083BB4C(&sPlay, &sPlayer);
        assert(sEnteredSwim && "child-sized modes keep their native swim threshold");
    }
    sMinish = sPikachu = false;
    PlacePlayer(PLAYER_FORM_ZORA, 40.0f, 40.0f, true);
    func_8083BB4C(&sPlay, &sPlayer);
    assert(!sStartedStanding && (sPlayer.stateFlags1 & PLAYER_STATE1_8000000));
    assert(sPlayerAgeProperties[PLAYER_FORM_HUMAN].unk_2C == 32.0f);

    puts("PASS adult swamp wading, shallow spawn, deep swimming, shore exit, and native form and boot controls");
    return 0;
}
