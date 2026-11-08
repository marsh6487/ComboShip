#include "global.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

SaveContext gSaveContext;
PlayState* gPlayState;
static int hide, drawCalls, specialDraws;
static Gfx commands[256];
static Gfx* opa;
static const char* D_801C0B20[28];

int32_t CVarGetInteger(const char* name, int32_t fallback) {
    assert(strcmp(name, "gEnhancements.HideNonTransformationMasks") == 0 && fallback == 0);
    return hide;
}

#undef OPEN_DISPS
#undef CLOSE_DISPS
#undef POLY_OPA_DISP
#undef gSPSegment
#undef gSPDisplayList
#undef MATRIX_FINALIZE_AND_LOAD
#define OPEN_DISPS(ctx) ((void)(ctx))
#define CLOSE_DISPS(ctx) ((void)(ctx))
#define POLY_OPA_DISP opa
#define gSPSegment(packet, segment, data) ((void)(packet), (void)(segment), (void)(data))
#define gSPDisplayList(packet, data) ((void)(packet), (void)(data), ++drawCalls)
#define MATRIX_FINALIZE_AND_LOAD(packet, ctx) ((void)(packet), (void)(ctx))
#define gPlayerAnim_cl_setmask "setmask"
#define gPlayerAnim_cl_setmaskend "setmaskend"

void Player_DrawCouplesMask(PlayState* play, Player* player) { ++specialDraws; }
void Player_DrawCircusLeadersMask(PlayState* play, Player* player) { ++specialDraws; }
void Player_DrawBlastMask(PlayState* play, Player* player) { ++specialDraws; }
void Player_DrawBunnyHood(PlayState* play) { ++specialDraws; }
void Player_DrawGreatFairysMask(PlayState* play, Player* player) { ++specialDraws; }
void Matrix_Push(void) {}
void Matrix_Pop(void) {}
void Matrix_Translate(f32 x, f32 y, f32 z, MatrixMode mode) {}
void Matrix_Scale(f32 x, f32 y, f32 z, MatrixMode mode) {}
void SkinMatrix_Vec3fMtxFMultXYZ(MtxF* mtx, Vec3f* src, Vec3f* dest) { dest->z = 0; }

/* NATIVE_SEGMENT_LOADER */

static void DrawNativeWorn(PlayState* play, Player* player, Gfx** dList1) {
    /* NATIVE_WORN_DRAW */
}

static void DrawPersistentWorn(Player* player) {
    /* PERSISTENT_WORN_PREFIX */
    ++drawCalls;
}

int main(void) {
    Player player = {0};
    PlayState play = {0};
    Gfx* head = commands;
    gPlayState = &play;
    opa = commands;
    player.transformation = PLAYER_FORM_HUMAN;
    player.skelAnime.animation = "idle";
    for (hide = 0; hide <= 1; ++hide) {
        for (int mask = PLAYER_MASK_NONE; mask < PLAYER_MASK_MAX; ++mask) {
            player.currentMask = player.maskId = mask;
            player.skelAnime.curFrame = 0;
            int before = drawCalls;
            Player saved = player;
            DrawNativeWorn(&play, &player, &head);
            int expected = mask != PLAYER_MASK_NONE && (!hide || mask >= PLAYER_MASK_FIERCE_DEITY);
            assert(drawCalls == before + expected);
            assert(memcmp(&saved, &player, sizeof(player)) == 0);
            // Held masks share the segment loader, independent of worn visibility.
            assert(func_80127438(&play, &player, mask));
        }
    }

    hide = 1;
    player.currentMask = player.maskId = PLAYER_MASK_COUPLE;
    specialDraws = 0;
    DrawNativeWorn(&play, &player, &head);
    assert(specialDraws == 0);
    hide = 0;
    DrawNativeWorn(&play, &player, &head);
    assert(specialDraws == 1);
    player.maskObjectLoadState = 1;
    int before = drawCalls;
    DrawNativeWorn(&play, &player, &head);
    assert(drawCalls == before);
    player.maskObjectLoadState = 0;
    player.skelAnime.animation = "setmask";
    player.skelAnime.curFrame = 11;
    DrawNativeWorn(&play, &player, &head);
    assert(drawCalls == before);
    player.skelAnime.curFrame = 12;
    DrawNativeWorn(&play, &player, &head);
    assert(drawCalls == before + 1);

    for (int form = PLAYER_FORM_FIERCE_DEITY; form < PLAYER_FORM_HUMAN; ++form) {
        player.transformation = form;
        player.currentMask = player.maskId = PLAYER_MASK_DEKU - (form == PLAYER_FORM_DEKU);
        player.skelAnime.curFrame = 10;
        hide = 1;
        before = drawCalls;
        DrawNativeWorn(&play, &player, &head);
        assert(drawCalls == before + 1);
    }

    for (hide = 0; hide <= 1; ++hide) {
        for (int active = 0; active <= 1; ++active) {
            gSaveContext.save.shipSaveInfo.persistentBunnyHood = active;
            before = drawCalls;
            DrawPersistentWorn(&player);
            assert(drawCalls == before + (active && !hide));
            assert(gSaveContext.save.shipSaveInfo.persistentBunnyHood == active);
        }
    }
    puts("PASS all native masks, transformation/held models, native equip timing and persistent bunny effects");
}
