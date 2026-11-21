#include "expansions/ssbb/ssbb_spawn.h"
#include "expansions/ssbb/ssbb_anim.h"
#include "expansions/ssbb/characters/pikachu_ssbb_all_anims.h"
#include "expansions/ssbb/characters/pikachu_ssbb_register.h"
#include "expansions/ssbb/characters/pikachu_ssbb_tex.h"

#define PIKACHU_MODEL_SCALE 0.014f

static SSBBCharacterInstance sPikachu;
static s32 sPikachuDefinition = -1;

s32 PikachuForm_IsSelected(void) {
    return CVarGetInteger("gBrokenItems.Enabled", 0) && CVarGetInteger("gPikachuMode", 0) &&
           !CVarGetInteger("gSm64Mario", 0) && Nei_Save()->pokeballOwned;
}

void PikachuForm_Destroy(void) {
    if (sPikachu.initialized) {
        SSBBSkin_Destroy(sPikachu.def->skinMesh);
        if (sPikachu.jointTable != NULL) {
            ZELDA_ARENA_FREE_DEBUG(sPikachu.jointTable);
        }
        memset(&sPikachu, 0, sizeof(sPikachu));
    }
}

static s32 PikachuForm_Init(PlayState* play) {
    if (!sPikachu.initialized) {
        if (sPikachuDefinition < 0) {
            sPikachuDefinition = pikachu_ssbb_Register();
        }
        SSBBChar_Init(&sPikachu, sPikachuDefinition, play);
        if (!sPikachu.initialized) {
            return false;
        }
        sPikachu.def->scale = PIKACHU_MODEL_SCALE;
        sPikachu.def->skinMesh->drawBindPose = true;
        sPikachu.def->skinMesh->useDefinitionScale = true;
        sPikachu.def->skinMesh->materialDL = pikachu_ssbb_mat_main;
        sPikachu.ssbbAnim = PikachuForm_GetAnimation(PIKA_ANIM_WAIT1);
    }
    return true;
}

static s32 PikachuForm_CanReplacePlayer(PlayState* play, Player* player) {
    return player != NULL && player == GET_PLAYER(play) && player->transformation == PLAYER_FORM_HUMAN &&
           PikachuForm_IsSelected() && gSaveContext.gameMode == GAMEMODE_NORMAL &&
           !(player->stateFlags1 & (PLAYER_STATE1_DEAD | PLAYER_STATE1_FIRST_PERSON)) && !Player_InCsMode(play);
}

void PikachuForm_Update(PlayState* play, Player* player) {
    // Keep the buffers until player teardown for queued draw commands.
    if (!PikachuForm_CanReplacePlayer(play, player) || !PikachuForm_Init(play)) {
        return;
    }

    PikachuAnimId animation = PIKA_ANIM_WAIT1;
    if (player->stateFlags1 & PLAYER_STATE1_8000000) {
        animation = PIKA_ANIM_SWIMF;
    } else if (!(player->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
        animation = player->actor.velocity.y > 0.0f ? PIKA_ANIM_JUMPF : PIKA_ANIM_FALL;
    } else if (fabsf(player->actor.speed) > 4.0f) {
        animation = PIKA_ANIM_RUN;
    } else if (fabsf(player->actor.speed) > 0.1f) {
        animation = PIKA_ANIM_WALKMIDDLE;
    }

    const struct SSBBAnim* clip = PikachuForm_GetAnimation(animation);
    if (clip == NULL) {
        clip = PikachuForm_GetAnimation(PIKA_ANIM_WAIT1);
    }
    if (clip == NULL) {
        return;
    }
    if (sPikachu.ssbbAnim != clip) {
        sPikachu.ssbbAnim = clip;
        sPikachu.curFrame = 0.0f;
    }
    sPikachu.animLength = (f32)clip->numFrames;
    sPikachu.playSpeed = clip->frameRate / 20.0f;
    SSBBChar_Update(&sPikachu);
}

static s32 PikachuForm_DrawMesh(PlayState* play, Vec3f* pos, s16 yaw) {
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    // The SSBB mesh is in Brawl's coordinate space with its Z axis vertical.
    Vec3s rot = { 0x4000, yaw, 0 };
    static Gfx* eyeMaterials[] = {
        pikachu_ssbb_mat_eyes_00,
        pikachu_ssbb_mat_eyes_01,
        pikachu_ssbb_mat_eyes_02,
        pikachu_ssbb_mat_eyes_03,
    };
    s32 blink = play->gameplayFrames % 50;
    s32 eyes = blink < 41 ? 0 : 1 + (blink - 41) / 3;

    // Allow the skin pass plus the caller's OPA/XLU continuation, including
    // the sublist terminators for the equipment framebuffer.
    const SSBBSkinDrawReserve reserve = { 3 * sizeof(Gfx), 3 * sizeof(Gfx), 0 };
    if (!SSBBSkin_ArenaHasSpace(&gfxCtx->polyOpa, 16 * sizeof(Gfx) + ALIGN16(sizeof(Mtx))) ||
        !SSBBSkin_ArenaHasSpace(&gfxCtx->polyXlu, 7 * sizeof(Gfx)) ||
        !SSBBSkin_ArenaHasSpace(&gfxCtx->overlay, 4 * sizeof(Gfx))) {
        return false;
    }
    s32 drawn;
    OPEN_DISPS(gfxCtx);
    Matrix_Push();
    gSPSegment(POLY_OPA_DISP++, 0x09, (uintptr_t)eyeMaterials[eyes]);
    drawn = SSBBSkin_DrawWithReserve(&sPikachu, play, pos, &rot, &reserve);
    Matrix_Pop();
    CLOSE_DISPS(gfxCtx);
    return drawn;
}

s32 PikachuForm_Draw(PlayState* play, Player* player) {
    if (!PikachuForm_CanReplacePlayer(play, player) || !sPikachu.initialized) {
        return false;
    }
    Vec3f pos = player->actor.world.pos;
    if (sPikachu.ssbbAnim == NULL) {
        pos.y += 2.7f; // center the bundled bind-pose feet above the ground
    }
    return PikachuForm_DrawMesh(play, &pos, player->actor.shape.rot.y);
}

s32 PikachuForm_DrawPause(PlayState* play) {
    Player* player = GET_PLAYER(play);
    if (player == NULL || player->transformation != PLAYER_FORM_HUMAN || !PikachuForm_IsSelected() ||
        !PikachuForm_Init(play)) {
        return false;
    }
    // Preview inside the same framebuffer and projection as the native Link doll.
    Vec3f pos = { 0.0f, -22.0f, 0.0f };
    const SSBBCharacterInstance gameplayAnimation = sPikachu;
    const f32 gameplayScale = sPikachu.def->scale;
    sPikachu.def->scale = 0.034f;
    sPikachu.ssbbAnim = PikachuForm_GetAnimation(PIKA_ANIM_WAIT1);
    sPikachu.curFrame = 0.0f;
    s32 drawn = PikachuForm_DrawMesh(play, &pos, 32300);
    sPikachu.def->scale = gameplayScale;
    sPikachu = gameplayAnimation;
    return drawn;
}
