// Production grass draw bodies with only GPU/matrix/light/hook boundaries replaced.
#include "global.h"
#include "2s2h/BenPort.h"
#include "2s2h/ShipUtils.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/Enhancements/Audio/MMWeather.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "overlays/actors/ovl_En_Kusa/z_en_kusa.h"
#include "overlays/actors/ovl_Obj_Grass/z_obj_grass.h"
#include "objects/object_kusa/object_kusa.h"
#include "objects/gameplay_field_keep/gameplay_field_keep.h"
#include "overlays/ovl_Obj_Grass/ovl_Obj_Grass.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#undef OPEN_DISPS_PORT_HELPERS
#undef CLOSE_DISPS_PORT_HELPERS
#define OPEN_DISPS_PORT_HELPERS(gfxCtx)
#define CLOSE_DISPS_PORT_HELPERS(gfxCtx)

static PlayState play;
static GraphicsContext gfx;
static Gfx opaque[128], translucent[128];
static int season = SEASON_AUTUMN, eligible = 1, allowDraw = 1, sway;
static Mtx matrix;
static MtxF D_80936AD8[8];

int MMWeather_SeasonForPlay(const PlayState* current) {
    assert(current == &play);
    return eligible ? season : -1;
}
void Gfx_SetupDL25_Opa(GraphicsContext* current) {
    assert(current == &gfx);
}
void Gfx_SetupDL25_Xlu(GraphicsContext* current) {
    assert(current == &gfx);
}
void gSPDisplayList(Gfx* command, Gfx* list) {
    __gSPDisplayList(command, list);
}
void Gfx_DrawDListOpa(PlayState* current, Gfx* list) {
    gSPDisplayList(current->state.gfxCtx->polyOpa.p++, list);
}
Mtx* Matrix_Finalize(GraphicsContext* current) {
    assert(current == &gfx);
    return &matrix;
}
void Matrix_SetTranslateRotateYXZ(f32 x, f32 y, f32 z, Vec3s* rotation) {
    (void)x;
    (void)y;
    (void)z;
    (void)rotation;
}
void Matrix_Scale(f32 x, f32 y, f32 z, MatrixMode mode) {
    (void)x;
    (void)y;
    (void)z;
    (void)mode;
}
void FrameInterpolation_RecordOpenChild(const void* child, int epoch) {
    (void)child;
    (void)epoch;
}
void FrameInterpolation_RecordCloseChild(void) {
}
void Ship_ExtendedCullingActorAdjustProjectedZ(Actor* actor) {
    (void)actor;
}
void Ship_ExtendedCullingActorRestoreProjectedPos(PlayState* current, Actor* actor) {
    (void)current;
    (void)actor;
}
bool GameInteractor_Should(GIVanillaBehavior flag, uint32_t result, ...) {
    assert(flag == VB_OBJGRASS_OPA_DRAW_BE_OVERRIDDEN || flag == VB_OBJGRASS_XLU_DRAW_BE_OVERRIDDEN);
    assert(result);
    return allowDraw;
}
Lights* LightContext_NewLights(LightContext* context, GraphicsContext* current) {
    static Lights lights;
    (void)context;
    assert(current == &gfx);
    return &lights;
}
void Lights_BindAll(Lights* lights, LightNode* node, Vec3f* pos, PlayState* current) {
    (void)lights;
    (void)node;
    (void)pos;
    assert(current == &play);
}
void Lights_Draw(Lights* lights, GraphicsContext* current) {
    (void)lights;
    assert(current == &gfx);
}
void EnKusa_ApplySway(MtxF* current) {
    (void)current;
    ++sway;
}
void EnKusa_WaitForInteract(EnKusa* actor, PlayState* current) {
    (void)actor;
    (void)current;
}
void ObjGrass_OverrideMatrixCurrent(MtxF* current) {
    (void)current;
}

#include "autumn_grass_production.inc"

static void Begin(void) {
    gfx.polyOpa.p = opaque;
    gfx.polyXlu.p = translucent;
}
static void Check(const Gfx* first, const Gfx* end, const void* leafList, const Gfx* native, size_t count) {
    int tinted = 0, drawn = 0, colors = 0;
    size_t sourceIndex = 0;
    for (const Gfx* command = first; command < end; ++command) {
        const unsigned op = command->words.w0 >> 24;
        if (op == G_SETGRAYSCALE) {
            tinted = command->words.w1 != 0;
            continue;
        }
        if (op == G_SETINTENSITY) {
            assert(command->words.w1 == 0xD99C45FF);
            ++colors;
            continue;
        }
        if (op == G_DL && command->words.w1 == (uintptr_t)leafList) {
            assert(tinted);
            ++drawn;
        }
        assert(sourceIndex < count && !memcmp(command, &native[sourceIndex++], sizeof(Gfx)));
    }
    assert(!tinted && drawn == 1 && colors == 1 && sourceIndex == count);
}

static void Kusa(void) {
    EnKusa grass = { 0 };
    grass.actor.projectedPos.z = 500;
    for (int bush = 0; bush <= 1; ++bush)
        for (int fade = 0; fade <= bush; ++fade) {
            grass.actor.projectedPos.z = fade ? 1250 : 500;
            void (*draw)(Actor*, PlayState*) = bush ? EnKusa_DrawBush : EnKusa_DrawGrass;
            season = SEASON_OFF;
            Begin();
            draw(&grass.actor, &play);
            Gfx native[128];
            const Gfx* start = fade ? translucent : opaque;
            const size_t count = (fade ? gfx.polyXlu.p : gfx.polyOpa.p) - start;
            memcpy(native, start, count * sizeof(Gfx));
            season = SEASON_AUTUMN;
            Begin();
            draw(&grass.actor, &play);
            Check(fade ? translucent : opaque, fade ? gfx.polyXlu.p : gfx.polyOpa.p,
                  !bush  ? gKusaSproutDL
                  : fade ? gKusaBushType2DL
                         : gKusaBushType1DL,
                  native, count);
            eligible = 0;
            Begin();
            draw(&grass.actor, &play);
            assert((size_t)((fade ? gfx.polyXlu.p : gfx.polyOpa.p) - (fade ? translucent : opaque)) == count);
            assert(!memcmp(native, fade ? translucent : opaque, count * sizeof(Gfx)));
            eligible = 1;
        }
    grass.isCut = 1;
    Begin();
    EnKusa_DrawGrass(&grass.actor, &play);
    assert(gfx.polyOpa.p - opaque == 1 && opaque[0].words.w1 == (uintptr_t)gKusaStumpDL);
    puts("PASS cuttable grass/bush OPA and faded XLU keep original alpha/materials; stumps and ineligible play "
         "unchanged");
}

static void GroupedGrass(void) {
    static ObjGrass grass;
    grass.activeGrassGroups = 1;
    grass.grassGroups[0].count = 1;
    grass.grassGroups[0].flags = OBJ_GRASS_GROUP_DRAW;
    ObjGrassElement* element = &grass.grassGroups[0].elements[0];
    element->flags = OBJ_GRASS_ELEM_DRAW;
    for (int fade = 0; fade <= 1; ++fade) {
        element->alpha = fade ? 96 : 255;
        void (*draw)(Actor*, PlayState*) = fade ? ObjGrass_DrawXlu : ObjGrass_DrawOpa;
        season = SEASON_OFF;
        Begin();
        draw(&grass.actor, &play);
        Gfx native[128];
        const Gfx* start = fade ? translucent : opaque;
        const size_t count = (fade ? gfx.polyXlu.p : gfx.polyOpa.p) - start;
        memcpy(native, start, count * sizeof(Gfx));
        season = SEASON_AUTUMN;
        Begin();
        draw(&grass.actor, &play);
        Check(fade ? translucent : opaque, fade ? gfx.polyXlu.p : gfx.polyOpa.p, gObjGrass_D_809AAAE0, native, count);
        assert(element->alpha == (fade ? 96 : 255));
        allowDraw = 0;
        Begin();
        draw(&grass.actor, &play);
        for (const Gfx* command = fade ? translucent : opaque; command < (fade ? gfx.polyXlu.p : gfx.polyOpa.p);
             ++command)
            assert(command->words.w1 != (uintptr_t)gObjGrass_D_809AAAE0);
        allowDraw = 1;
    }
    puts("PASS grouped grass OPA/XLU scoped color, unchanged fade alpha and existing replacement hook routing");
}

int main(void) {
    play.state.gfxCtx = &gfx;
    Kusa();
    GroupedGrass();
}
