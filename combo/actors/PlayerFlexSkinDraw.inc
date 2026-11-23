// Native C glue included once by each host's FlexLOD implementation.
// PLAYER_FLEX_SKIN_STORE uses that host's native Matrix_ToMtx signature.
#include "PlayerFlexSkin.h"
#include "SeasonalIdle.h"
static const void* sPlayerFlexSkinPlan;
typedef struct {
    Gfx* commands[32];
    Gfx* originals[32];
    s32 count;
    SeasonalIdleDraw idle;
} PlayerFlexSkinFrame;
static PlayerFlexSkinFrame* sPlayerFlexSkinFrame;

static void PlayerFlexSkin_AddIdleArmRotation(s32 limbIndex, Vec3s* rotation) {
    const s32 arm = limbIndex - SEASONAL_IDLE_FIRST_ARM;
    if (sPlayerFlexSkinFrame != NULL && sPlayerFlexSkinFrame->idle.active &&
        arm >= 0 && arm < SEASONAL_IDLE_ARM_COUNT) {
        const Vec3s* delta = &sPlayerFlexSkinFrame->idle.rotations[arm];
        rotation->x = (s16)((u16)rotation->x + (u16)delta->x);
        rotation->y = (s16)((u16)rotation->y + (u16)delta->y);
        rotation->z = (s16)((u16)rotation->z + (u16)delta->z);
    }
}

static Gfx* PlayerFlexSkin_SelectDraw(s32 limbIndex, Gfx* selected, Gfx* original, Gfx** rigid) {
    *rigid = selected;
    const s32 hand = limbIndex == 16 ? 0 : limbIndex == 19 ? 1 : -1;
    if (hand >= 0 && selected != NULL && original != NULL && sPlayerFlexSkinFrame != NULL) {
        const SeasonalIdleDraw* idle = &sPlayerFlexSkinFrame->idle;
        if (idle->active && idle->rigid[hand] != NULL &&
            (selected == original || selected == idle->ordinary[hand][0] ||
             selected == idle->ordinary[hand][1] || selected == idle->ordinary[hand][2])) {
            *rigid = idle->rigid[hand];
            return sPlayerFlexSkinPlan != NULL && idle->weighted[hand] != NULL ? idle->weighted[hand] : *rigid;
        }
    }
    return PlayerFlexSkin_SelectLimb(sPlayerFlexSkinPlan, limbIndex, selected, original);
}

static void PlayerFlexSkin_RecordDraw(Gfx* command, Gfx* selected, Gfx* rigid) {
    if (sPlayerFlexSkinPlan != NULL && sPlayerFlexSkinFrame != NULL && selected != rigid &&
        selected != NULL && rigid != NULL && sPlayerFlexSkinFrame->count < 32) {
        s32 index = sPlayerFlexSkinFrame->count++;
        sPlayerFlexSkinFrame->commands[index] = command;
        sPlayerFlexSkinFrame->originals[index] = rigid;
    }
}

static void PlayerFlexSkin_RestoreRigidDraws(void) {
    s32 i;
    if (sPlayerFlexSkinFrame == NULL)
        return;
    for (i = 0; i < sPlayerFlexSkinFrame->count; ++i)
        gSPDisplayList(sPlayerFlexSkinFrame->commands[i], sPlayerFlexSkinFrame->originals[i]);
}
static s32 PlayerFlexSkin_MatrixFits(const MtxF* matrix) {
    const float* values = &matrix->xx;
    s32 i;
    for (i = 0; i < 16; ++i) {
        u32 bits;
        memcpy(&bits, &values[i], sizeof(bits));
        if ((bits & 0x7f800000u) == 0x7f800000u || values[i] < -32768.f || values[i] > 32767.f)
            return false;
    }
    return true;
}

static const void* PlayerFlexSkin_CheckRoom(PlayState* play, const void* plan, s32 nativeCount) {
    if (plan != NULL) {
        const uintptr_t head = (uintptr_t)play->state.gfxCtx->polyOpa.p;
        const uintptr_t tail = (uintptr_t)play->state.gfxCtx->polyOpa.d & ~(uintptr_t)15;
        const size_t bytes = (nativeCount + PlayerFlexSkin_MatrixCount(plan)) * sizeof(Mtx) + 32 * sizeof(Gfx);
        if (!head || tail < head || bytes > tail - head)
            return NULL; // Rigid companions remain valid without any extra allocation.
    }
    return plan;
}
static void PlayerFlexSkin_InitMatrices(Mtx* matrices, s32 count) {
    MtxF identity;
    s32 i;
    memset(&identity, 0, sizeof(identity));
    identity.xx = identity.yy = identity.zz = identity.ww = 1.f;
    Matrix_Push();
    Matrix_Put(&identity);
    for (i = 0; i < count; ++i)
        PLAYER_FLEX_SKIN_STORE(&matrices[i]);
    Matrix_Pop();
}
static void PlayerFlexSkin_ApplyMatrices(const void* plan, Mtx* matrices, s32 nativeCount) {
    MtxF native[32];
    MtxF blended;
    s32 i;
    if (plan == NULL)
        return;
    for (i = 0; i < nativeCount; ++i)
        Matrix_MtxToMtxF(&matrices[i], &native[i]);
    Matrix_Push();
    for (i = 0; i < PlayerFlexSkin_MatrixCount(plan); ++i) {
        PlayerFlexSkin_BlendMatrix(plan, i, native, &blended);
        if (!PlayerFlexSkin_MatrixFits(&blended)) {
            // GPU commands have not run yet. Keep this frame's ordinary rigid
            // companions instead of referencing an unrepresentable palette.
            PlayerFlexSkin_RestoreRigidDraws();
            break;
        }
        Matrix_Put(&blended);
        PLAYER_FLEX_SKIN_STORE(&matrices[nativeCount + i]);
    }
    Matrix_Pop();
}
