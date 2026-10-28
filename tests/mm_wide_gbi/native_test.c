/* Production-backed checks: no matrix/vertex writer is reproduced here. */
#include "global.h"
#include "2s2h/BenPort.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(test) do { checks++; if (!(test)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #test); abort(); \
} } while (0)

/* Native-only fixture: resource resolution and trace recording are external to
 * these writers. Keep real GBI wrappers, native trigonometry and MM types. */
int ResourceMgr_OTRSigCheck(char* data) { return 0; }
Gfx* ResourceMgr_LoadGfxByName(const char* name) { abort(); }
Vtx* ResourceMgr_LoadVtxByName(char* name) { abort(); }
char* ResourceMgr_LoadIfDListByName(const char* name) { abort(); }
void FrameInterpolation_RecordOpenChild(const void* id, int value) {}
void FrameInterpolation_RecordCloseChild(void) {}
void FrameInterpolation_RecordMatrixMtxFToMtx(MtxF* src, Mtx* dest) {}
void FrameInterpolation_RecordMatrixToMtx(Mtx* dest, char* file, s32 line) {}
void FrameInterpolation_RecordSkinMatrixMtxFToMtx(MtxF* src, Mtx* dest) {}
void Sleep_Msec(u32 ms) {}

static void* allocations[4];
static size_t allocationSizes[4];
static unsigned allocationCount;
static unsigned freeCount;
void* SystemArena_Malloc(size_t size) {
    CHECK(allocationCount < 4);
    void* ptr = malloc(size);
    allocations[allocationCount] = ptr;
    allocationSizes[allocationCount++] = size;
    return ptr;
}
void SystemArena_Free(void* ptr) {
    unsigned found = 0;
    for (unsigned i = 0; i < allocationCount; i++) {
        if (allocations[i] == ptr) {
            allocations[i] = NULL;
            found++;
        }
    }
    CHECK(found == 1);
    freeCount++;
    free(ptr);
}

static const MtxF identity = {{
    {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}
}};

static void CheckMatrix(Mtx* mtx, const MtxF* expected, f32 tolerance) {
    MtxF result;
    guMtxL2F(result.mf, mtx);
    for (unsigned row = 0; row < 4; row++) {
        for (unsigned col = 0; col < 4; col++) {
            CHECK(fabsf(result.mf[row][col] - expected->mf[row][col]) <= tolerance);
        }
    }
}

static void TestConversions(void) {
    Mtx mtx;
    MtxF source = {{
        {1.25f, -2.5f, 3.125f, 0}, {0.5f, 1, -0.25f, 0},
        {5, -6, 7, 0}, {123.75f, -234.125f, 345.5f, 1}
    }};
#ifdef GBI_FLOAT_MTX
    source.xw = 100000.25f;
    source.yw = -200000.125f;
#endif
    guMtxF2L(source.mf, &mtx);
    CheckMatrix(&mtx, &source, 0);
    MtxF roundtrip;
    MtxConv_L2F(&roundtrip, &mtx);
    CHECK(memcmp(&roundtrip, &source, sizeof(source)) == 0);
    CHECK(Matrix_MtxFToMtx(&source, &mtx) == &mtx);
    Matrix_MtxToMtxF(&mtx, &roundtrip);
    CHECK(memcmp(&roundtrip, &source, sizeof(source)) == 0);
    SkinMatrix_MtxFToMtx(&source, &mtx);
    CheckMatrix(&mtx, &source, 0);
#ifdef GBI_FLOAT_MTX
    CHECK(memcmp(&mtx, &source, sizeof(source)) == 0);
#else
    /* Literal legacy packed-word contract; distinct from a host halfword view. */
    CHECK((u32)mtx.m[0][0] == 0x0001FFFDu);
    CHECK((u32)mtx.m[2][0] == 0x40008000u);
#endif
    MtxConv_F2L(&mtx, &source);
#ifdef GBI_FLOAT_MTX
    MtxF result;
    MtxConv_L2F(&result, &mtx);
    CHECK(memcmp(&result, &source, sizeof(source)) == 0);
    guMtxIdent(&mtx);
    CheckMatrix(&mtx, &identity, 0);
#else
    /* Preserve the existing native halfword writer in legacy mode. */
    CHECK(mtx.intPart[0][0] == 1);
    CHECK(mtx.fracPart[0][0] == 0x4000);
    CHECK(mtx.intPart[0][1] == 0xFFFD);
    CHECK(mtx.fracPart[0][1] == 0x8000);
#endif
    CheckMatrix(&gIdentityMtx, &identity, 0);

    const MtxF translated = {{
        {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {12.5f, -23.25f, 34.75f, 1}
    }};
    guTranslate(&mtx, 12.5f, -23.25f, 34.75f);
    CheckMatrix(&mtx, &translated, 0);
    const MtxF scaled = {{
        {2, 0, 0, 0}, {0, -3, 0, 0}, {0, 0, 4.5f, 0}, {0, 0, 0, 1}
    }};
    guScale(&mtx, 2, -3, 4.5f);
    CheckMatrix(&mtx, &scaled, 0);
}

static void TestNativeTransforms(void) {
    Mtx mtx;
    Mtx_SetTranslateScaleMtx(&mtx, 2, -3, 4.5f, 12.5f, -23.25f, 34.75f);
    const MtxF scaledTranslated = {{
        {2, 0, 0, 0}, {0, -3, 0, 0}, {0, 0, 4.5f, 0}, {12.5f, -23.25f, 34.75f, 1}
    }};
    CheckMatrix(&mtx, &scaledTranslated, 0);
#ifdef GBI_FLOAT_MTX
    /* Quarter turn, oblique unit axis. The original axisY signs are deliberate
     * preservation requirements, including the documented vanilla bug. */
    const MtxF rotated = {{
        {0.36f, 0.48f, 0.8f, 0}, {0.48f, 0.64f, 0.6f, 0},
        {-0.8f, -0.6f, 0, 0}, {0, 0, 0, 1}
    }};
    Mtx_SetRotationMtx(&mtx, 0x4000, 0.6f, 0.8f, 0);
    CheckMatrix(&mtx, &rotated, 0.000001f);
    const MtxF srt = {{
        {0.72f, 0.96f, 1.6f, 0}, {-1.44f, -1.92f, -1.8f, 0},
        {-3.6f, -2.7f, 0, 0}, {100000.25f, -200000.125f, 300000.5f, 1}
    }};
    Mtx_SetTranslationRotationScaleMtx(&mtx, 2, -3, 4.5f, 0x4000, 0.6f, 0.8f, 0,
                                     100000.25f, -200000.125f, 300000.5f);
    CheckMatrix(&mtx, &srt, 0.000001f);
#else
    /* Existing legacy word/halfword layout is pinned without interpreting its
     * endian-sensitive output as a different renderer contract. */
    Mtx_SetRotationMtx(&mtx, 0x4000, 0.6f, 0.8f, 0);
    CHECK(mtx.intPart[0][0] == 0);
    CHECK(mtx.fracPart[0][0] == 23592);
    CHECK(mtx.intPart[2][0] == 0xFFFF);
    CHECK(mtx.fracPart[2][0] == 13108);
    Mtx_SetTranslationRotationScaleMtx(&mtx, 2, -3, 4.5f, 0x4000, 0.6f, 0.8f, 0,
                                     12.5f, -23.25f, 34.75f);
    CHECK(mtx.intPart[3][1] == 0xFFE8);
    CHECK(mtx.fracPart[3][1] == 0xC000);
    CHECK(mtx.intPart[3][3] == 1);
#endif
}

static void TestTileFrames(void) {
    TransitionTile tile;
    CHECK(TransitionTile_Init(&tile, 3, 2) == &tile);
    CHECK(allocationCount == 4);
    CHECK(allocationSizes[0] == 12 * sizeof(TransitionTileVtxData));
    CHECK(allocationSizes[1] == 12 * sizeof(Vtx));
    CHECK(allocationSizes[2] == 12 * sizeof(Vtx));
    const size_t commandsPerFrame = (3 * 9 + 1) * 2 + 2;
#ifdef GBI_S32_VTX
    CHECK(sizeof(Vtx) == 24);
    CHECK(allocationSizes[3] == 2 * commandsPerFrame * sizeof(Gfx));
    CHECK(tile.gfx[0].words.w1 == (uintptr_t)tile.vtxFrame1);
    CHECK(tile.gfx[28].words.w1 == (uintptr_t)&tile.vtxFrame1[4]);
    CHECK(tile.gfx[commandsPerFrame].words.w1 == (uintptr_t)tile.vtxFrame2);
    CHECK(tile.gfx[commandsPerFrame + 28].words.w1 == (uintptr_t)&tile.vtxFrame2[4]);
#else
    CHECK(sizeof(Vtx) == 16);
    CHECK(allocationSizes[3] == commandsPerFrame * sizeof(Gfx));
    CHECK(tile.gfx[0].words.w1 == 0x0A000000);
    CHECK(tile.gfx[28].words.w1 == 0x0A000040);
#endif
    CHECK(tile.vtxFrame1[11].n.ob[0] == 96);
    CHECK(tile.vtxFrame2[11].n.ob[1] == 64);
    CHECK(tile.vtxFrame1[11].n.tc[0] == 6144);
    CHECK(tile.vtxFrame2[11].n.a == 255);
    tile.vtxData[11].x = 111;
    tile.vtxData[11].y = 222;
    Gfx output[20] = {0};
    Gfx* next = output;
    TransitionTile_Draw(&tile, &next);
    CHECK(tile.frame == 1);
    CHECK(tile.vtxFrame1[11].n.ob[0] == 111);
    CHECK(tile.vtxFrame2[11].n.ob[0] == 96);
    CHECK(output[6].words.w1 == (uintptr_t)tile.gfx);
    tile.vtxData[11].x = 333;
    tile.vtxData[11].y = 444;
    next = output;
    TransitionTile_Draw(&tile, &next);
    CHECK(tile.frame == 0);
    CHECK(tile.vtxFrame1[11].n.ob[0] == 111);
    CHECK(tile.vtxFrame2[11].n.ob[0] == 333);
#ifdef GBI_S32_VTX
    CHECK(output[6].words.w1 == (uintptr_t)(tile.gfx + commandsPerFrame));
#else
    CHECK(output[6].words.w1 == (uintptr_t)tile.gfx);
#endif
    TransitionTile_Destroy(&tile);
    CHECK(freeCount == 4);
    CHECK(tile.gfx == NULL && tile.vtxFrame1 == NULL && tile.vtxFrame2 == NULL && tile.vtxData == NULL);
}

int main(void) {
    TestConversions();
    TestNativeTransforms();
    TestTileFrames();
#ifdef GBI_FLOAT_MTX
    printf("MM float/s32 native writers: %u checks passed\n", checks);
#else
    printf("MM legacy native writers: %u checks passed\n", checks);
#endif
    return 0;
}
