/* Native structures and packet macros are the production game's actual ABI.
 * Only graphics/service calls are fixture boundaries. The room and collision
 * lookup functions are appended verbatim by run_tests.py. */
#include "global.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include <fast/ucodehandlers.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#undef osSyncPrintf
#define osSyncPrintf(...) ((void)0)
/* These fixture lists are native memory, so use the actual native packet
 * builder without the resource-name dispatch provided by the game bridge. */
#define gSPDisplayList(packet, list) __gSPDisplayList(packet, list)
#define gSPSegment(packet, segment, base) __gSPSegment(packet, segment, (uintptr_t)(base))

_Static_assert(CAM_SET_DOOR0 == 0x1c, "Crash setting is DOOR0");
_Static_assert(CAM_SET_PREREND_FIXED == 0x19, "Fixed prerender setting");
_Static_assert(sizeof(Vec3s) * 2 + offsetof(Vec3s, y) == 0xe,
               "Faulting camera metadata access is exactly pointer + 0xe");
_Static_assert(sizeof(Mtx) == sizeof(float) * 16, "Use the combined game's matrix ABI");

static struct {
    PlayState play;
    Camera camera;
    Player player;
    GraphicsContext graphics;
    GameInfo gameInfo;
    CollisionHeader collision;
    CamData cameraData[4];
    Vec3s cameraVectors[3];
    BgImage backgrounds[2];
    PolygonDlist polygons;
    MeshHeader mesh;
    Room room;
    Gfx opaqueBuffer[64];
    Gfx translucentBuffer[64];
    Gfx opaqueList[1];
    Gfx translucentList[1];
    unsigned char backgroundPixels[2];
} fixture;

GameInfo* gGameInfo;
uintptr_t gSegments[NUM_SEGMENTS];
Mtx gMtxClear;
static int allowImageLookup;
static int hookCameraSetting;
static int imageHookCalls;
static int opaqueSetupCalls;
static int translucentSetupCalls;
static int backgroundCalls;
static int backgroundPolicyCalls;
static int allowBackgroundDraw;
static void* drawnBackground;
static u16 drawnWidth, drawnHeight;
static f32 drawnOffsetX, drawnOffsetY;

bool GameInteractor_Should(GIVanillaBehavior flag, uint32_t result, ...) {
    if (flag == VB_SHOULD_LOAD_BG_IMAGE) {
        ++imageHookCalls;
        /* Existing entrance hooks restore camera state. Exercise that boundary
         * with a real Camera mutation, without reproducing lookup logic. */
        if (hookCameraSetting >= 0) {
            fixture.camera.setting = hookCameraSetting;
        }
        return allowImageLookup;
    }
    if (flag == VB_DRAW_2D_BACKGROUND) {
        ++backgroundPolicyCalls;
        return allowBackgroundDraw;
    }
    return result;
}

void Graph_OpenDisps(Gfx** refs, GraphicsContext* gfx, const char* file, s32 line) {}
void Graph_CloseDisps(Gfx** refs, GraphicsContext* gfx, const char* file, s32 line) {}
void FrameInterpolation_RecordOpenChild(const void* key, int line) {}
void FrameInterpolation_RecordCloseChild(void) {}
void Gfx_SetupDL_25Opa(GraphicsContext* gfx) { ++opaqueSetupCalls; }
void Gfx_SetupDL_25Xlu(GraphicsContext* gfx) { ++translucentSetupCalls; }
uint32_t SysUcode_GetUCode(void) { return ucode_f3dex2; }
void LogUtils_HungupThread(const char* file, s32 line) {
    fprintf(stderr, "Unexpected missing background ID in %s:%d\n", file, line);
    abort();
}
Vec3f* Camera_GetSkyboxOffset(Vec3f* dst, Camera* camera) {
    *dst = (Vec3f){ 10.0f, 20.0f, 30.0f };
    return dst;
}

/* Rendering service boundary: preserve its documented veto behavior and record
 * the actual selected image and arguments received from the production caller.
 * This function does not choose camera IDs, find images, or choose geometry. */
void Room_DrawBackground2D(Gfx** gfx, void* source, void* tlut, u16 width, u16 height,
                           u8 fmt, u8 siz, u16 mode, u16 tlutCount, f32 x, f32 y) {
    if (!GameInteractor_Should(VB_DRAW_2D_BACKGROUND, true)) {
        return;
    }
    ++backgroundCalls;
    drawnBackground = source;
    drawnWidth = width;
    drawnHeight = height;
    drawnOffsetX = x;
    drawnOffsetY = y;
}
