// Native MM port of the authored Rito controller. The donor controllers keep their
// state and tuning together; engine-specific glue belongs here.
#include "forms_internal.h"
extern "C" {
#include "mods/items/objects/object_tornado.h"
#include "mods/ext_buttons/ext_buttons.h"
#include "mods/extended_inventory.h"
#include "overlays/actors/ovl_En_Light/z_en_light.h"
#include "overlays/actors/ovl_Obj_Switch/z_obj_switch.h"
#include "overlays/actors/ovl_En_Arrow/z_en_arrow.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "z64quake.h"
PlayerAnimationHeader* ResourceMgr_LoadPlayerAnimAsHeaderInPlaceResampled(const char*, uint8_t, int16_t);
int ResourceMgr_OTRSigCheck(char*);
u8 Sm64Mario_ShouldHideLink(void);
u8 WolfLinkForm_IsSelected(void);
u8 KiteSurf_IsActive(void);
}

static u8 MmRitoFlightUpdate(Player*, PlayState*);
static u8 MmRitoBowUpdate(Player*, PlayState*);
static void MmRitoRelease(Player*, u8);
static void MmRitoBowEnd(Player*);
extern "C" void MmRitoRestoreLanding(void);
extern "C" u8 MmRitoBowIsOut(void);
static u8 sRitoNativeLandFallback;
static u8 sRitoNativeBowFallback;

static u8 MmRitoInputBlocked(void) {
    return gPlayState == NULL || gPlayState->pauseCtx.state != PAUSE_STATE_OFF ||
           gPlayState->msgCtx.msgMode != MSGMODE_NONE || GET_PLAYER(gPlayState)->transformation != PLAYER_FORM_HUMAN;
}

#include "rito_flight.inc.c"
#include "rito_bow.inc.c"

static PlayState* sRitoPlay;
static Player* sRitoPlayer;
static u32 sRitoLastFrame;

void RitoForm_ForgetContext(void) {
    // The old arena owns its actors/effects. Never kill or dereference its saved
    // addresses after a scene or file transition, even when the action is OFF.
    MmRitoRestoreLanding();
    MmRitoWindClear();
    MmRitoBowReset();
    MmRitoClearFlightState();
    sRitoDamage = {};
    sRitoPlay = NULL;
    sRitoPlayer = NULL;
    sRitoLastFrame = 0;
}

static void RitoForm_SyncContext(Player* player, PlayState* play) {
    if (play != sRitoPlay || player != sRitoPlayer || (play != NULL && play->gameplayFrames < sRitoLastFrame))
        RitoForm_ForgetContext();
    sRitoPlay = play;
    sRitoPlayer = player;
    sRitoLastFrame = play != NULL ? play->gameplayFrames : 0;
}

void RitoForm_Reset(Player* player, PlayState* play) {
    RitoForm_SyncContext(player, play);
    if (player != NULL) {
        MmRitoRelease(player, 0);
        MmRitoBowEnd(player);
    }
    MmRitoRestoreLanding();
    if (play != NULL)
        MmRitoWindTrailsOff(play);
    MmRitoWindClear();
    MmRitoBowReset();
    MmRitoClearFlightState();
    sRitoDamage = {};
}

void RitoForm_FilterInput(Player* player, Input* input) {
    RitoForm_SyncContext(player, gPlayState);
    if (!Forms_CanAct(player) || MmRitoInputBlocked())
        return;
    // Native Link must never begin a roll, sword swing or shield transition on
    // the same press that the Rito controller claims after the common update.
    input->cur.button &= ~(BTN_A | BTN_B);
    input->press.button &= ~(BTN_A | BTN_B);
    input->rel.button &= ~(BTN_A | BTN_B);
    if (MmRitoBowIsOut() || CHECK_BTN_ALL(Forms_RawInput()->press.button, BTN_B)) {
        input->cur.button &= ~BTN_R;
        input->press.button &= ~BTN_R;
    }
}

void RitoForm_Update(Player* player, PlayState* play) {
    RitoForm_SyncContext(player, play);
    if (!Forms_CanAct(player) || MmRitoInputBlocked()) {
        if (sRito.state != RITO_FLY_OFF || sRitoBow.state != RITO_BOW_OFF)
            RitoForm_Reset(player, play);
        return;
    }
    const Input* raw = Forms_RawInput();
    if (sRito.state == RITO_FLY_OFF && sRitoBow.state == RITO_BOW_OFF && Forms_OnGround(player) &&
        CHECK_BTN_ALL(raw->press.button, BTN_A) &&
        (Forms_StickMagnitude(raw) > 10.0f || CHECK_BTN_ALL(raw->cur.button, BTN_Z))) {
        s32 dir =
            CHECK_BTN_ALL(raw->cur.button, BTN_Z) ? player->controlStickDirections[player->controlStickDataIndex] : -1;
        if (dir < 0)
            dir = -1;
        MmRitoTryHop(player, play, dir);
    }
    if (!MmRitoBowUpdate(player, play))
        MmRitoFlightUpdate(player, play);
    if (RitoForm_OwnsAction())
        player->actor.speed = player->speedXZ;
}

u8 RitoForm_OwnsAction(void) {
    return sRito.state != RITO_FLY_OFF || sRitoBow.state != RITO_BOW_OFF;
}

// This POC calibrates the actual adult/default renderer only. Unknown rigs,
// scales and special poses retain native leg IK without any contact rewrite.
extern "C" u8 RitoForm_CanAdjustFeet(PlayState* play, Player* player, SkelAnime* skelAnime) {
    if (play == NULL || player == NULL || play != gPlayState || player != GET_PLAYER(play) ||
        skelAnime != &player->skelAnime || CustomForms_ActiveForm() != CUSTOM_FORM_RITO ||
        player->transformation != PLAYER_FORM_HUMAN || !Forms_IsAdultAge() || CustomForms_RootScale() != 0.7036f ||
        !Forms_OnGround(player) || !Forms_CanAct(player) || MmRitoInputBlocked() ||
        play->csCtx.state != CS_STATE_IDLE || Sm64Mario_ShouldHideLink() || WolfLinkForm_IsSelected() ||
        KiteSurf_IsActive() || sRito.state != RITO_FLY_OFF || sRitoBow.state != RITO_BOW_OFF ||
        (player->stateFlags3 & (PLAYER_STATE3_1 | PLAYER_STATE3_PAUSE_ACTION_FUNC))) {
        return 0;
    }
    if (CVarGetFloat("gAdultLink.Scale", 0.01f) != 0.01f || CVarGetFloat("gAdultLink.YOffset", 0.0f) != 0.0f ||
        player->actor.scale.x != 0.01f || player->actor.scale.y != 0.01f || player->actor.scale.z != 0.01f ||
        player->actor.shape.yOffset != RITO_DRAW_Y_BASE || player->ageProperties == NULL ||
        player->ageProperties->unk_08 != 1.0f || player->unk_AB8 != 0.0f || player->unk_ABC != 0.0f) {
        return 0;
    }
    const char* animation = (const char*)player->skelAnime.animation;
    if (!ResourceMgr_OTRSigCheck((char*)animation)) {
        return 0;
    }
    return BEN_ANIM_EQUAL(animation, gPlayerAnim_link_normal_walk_free) ||
           BEN_ANIM_EQUAL(animation, gPlayerAnim_link_normal_walk) ||
           BEN_ANIM_EQUAL(animation, gPlayerAnim_link_fighter_walk_long) ||
           BEN_ANIM_EQUAL(animation, gPlayerAnim_link_normal_run_free) ||
           BEN_ANIM_EQUAL(animation, gPlayerAnim_link_normal_run) ||
           BEN_ANIM_EQUAL(animation, gPlayerAnim_link_fighter_run) ||
           BEN_ANIM_EQUAL(animation, gPlayerAnim_link_fighter_run_long);
}

f32 RitoForm_DrawOffset(Player* player) {
    return MmRitoDrawYOffset(player);
}

void RitoForm_DrawWorld(PlayState* play, Player* player) {
    MmRitoWindDraw(play);
}

static void RitoForm_DrawPiece(PlayState* play, const char* path, f32 x, f32 y, f32 z, u8 xlu) {
    Gfx* dl = ResourceMgr_FileExists(path + 7) ? ResourceMgr_LoadGfxByName(path) : (Gfx*)OotAssets_LoadGfxDirect(path);
    if (dl == NULL)
        return;
    OPEN_DISPS(play->state.gfxCtx);
    Matrix_Push();
    Matrix_Translate(x, y, z, MTXMODE_APPLY);
    if (xlu) {
        MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
        gSPDisplayList(POLY_XLU_DISP++, dl);
    } else {
        MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
        gSPDisplayList(POLY_OPA_DISP++, dl);
    }
    Matrix_Pop();
    CLOSE_DISPS(play->state.gfxCtx);
}

void RitoForm_PostLimb(PlayState* play, Player* player, s32 limbIndex) {
    const u8 nativeBow = sRitoNativeBowFallback || !RitoMask_UsesCornerWheel();
    const s32 bowHand = nativeBow ? PLAYER_LIMB_RIGHT_HAND : PLAYER_LIMB_LEFT_HAND;
    if (MmRitoBowIsOut() && limbIndex == bowHand) {
        // The authored bow-only extraction is absent from the input archive. The
        // native adult held-bow DL and separate string are established resources.
        if (nativeBow) {
            RitoForm_DrawPiece(play, "__OTR__objects/object_link_child/gLinkHumanBowDL", 0, 0, 0, 0);
            RitoForm_DrawPiece(play, "__OTR__objects/object_link_child/object_link_child_DL_017818", -35, -395, 0, 1);
        } else {
            RitoForm_DrawPiece(play, "__OTR__objects/object_link_boy/gLinkAdultRightHandHoldingBowNearDL", 0, 0, 0, 0);
            RitoForm_DrawPiece(play, "__OTR__objects/object_link_boy/gLinkAdultBowStringDL", 0, -360.4f, 0, 1);
        }
    } else if (!MmRitoBowIsOut()) {
        const u8 shielding = (player->stateFlags1 & PLAYER_STATE1_SHIELDING) != 0;
        if (limbIndex == (shielding ? PLAYER_LIMB_RIGHT_HAND : PLAYER_LIMB_SHEATH)) {
            RitoForm_DrawPiece(play, "__OTR__objects/object_nei_rito_shield/gRitoShieldDL", shielding ? -20.0f : 608.0f,
                               shielding ? 115.5f : 10.0f, -145.5f, 1);
        }
    }
}
