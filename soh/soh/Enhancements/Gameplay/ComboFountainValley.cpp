#ifdef COMBO_BUILD
#include "ComboFountainValley.h"
#include "global.h"
#include "ComboGameMode.h"
#include "entrances/FountainValley.h"
#include "entrances/LostWoodsEntrance.h"
#include "render/RegionalBridgeMaterials.h"
#include "soh/ActorDB.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/ShipInit.hpp"
#include "soh/frame_interpolation.h"
#include "libultraship/bridge/resourcebridge.h"
#include "fast/resource/type/DisplayList.h"
#include "fast/resource/type/Vertex.h"
#include <algorithm>
#include <cstring>
#include <memory>
namespace {
// OPEN/CLOSE_DISPS redeclare these at block scope; retain their C linkage.
extern "C" void FrameInterpolation_RecordOpenChild(const void*, int);
extern "C" void FrameInterpolation_RecordCloseChild(void);
using namespace ComboFountainValley;
constexpr const char* kBackdrop = "scenes/shared/spot08_scene/spot08_room_0DL_002538";
std::shared_ptr<Fast::DisplayList> sPatched;
std::array<Gfx, 4> sOriginal{};
bool sDetailedRender = false;
void RestoreBackdrop() {
    if (sPatched && sPatched->Instructions.size() == 45)
        for (size_t i = 0; i < kBackdropSlots.size(); ++i)
            sPatched->Instructions[kBackdropSlots[i]] = sOriginal[i];
    sPatched.reset();
}
bool PrepareBackdrop() {
    auto list = std::dynamic_pointer_cast<Fast::DisplayList>(ResourceLoad(kBackdrop));
    if (!list) {
        RestoreBackdrop();
        return false;
    }
    if (list == sPatched)
        return true;
    RestoreBackdrop();
    sDetailedRender = false;
    if (NativeBackdropLayout(list->Instructions)) {
        // Verify the actual referenced vertex resource as well as command words.
        auto vertices =
            std::dynamic_pointer_cast<Fast::Vertex>(ResourceLoad("scenes/shared/spot08_scene/spot08_room_0Vtx_0023F8"));
        constexpr Point expected[] = { { 2008, 145, 50 },   { 2049, 250, 448 },  { 2052, 718, 488 },
                                       { 2008, 614, 50 },   { 1969, 716, -382 }, { 1967, 250, -348 },
                                       { 1931, 961, -408 }, { 1928, 565, -370 }, { 1968, 462, 50 },
                                       { 1968, 862, 50 },   { 2012, 566, 488 },  { 2013, 966, 497 } };
        if (!vertices || vertices->VertexList.size() < 12)
            return false;
        for (size_t i = 0; i < 12; ++i) {
            const auto& v = vertices->VertexList[i].v;
            if (v.ob[0] != expected[i].x || v.ob[1] != expected[i].y || v.ob[2] != expected[i].z)
                return false;
        }
        sPatched = list;
        for (size_t i = 0; i < kBackdropSlots.size(); ++i) {
            sOriginal[i] = list->Instructions[kBackdropSlots[i]];
            list->Instructions[kBackdropSlots[i]] = gsSPNoOp();
        }
        return true;
    }
    // The complete preverified XML/Djipi layouts have a high, distant canopy.
    // Recognize their parent marker and separate high vertex resource below;
    // this does not prove leaf linkage for arbitrary partial/restructured packs.
    constexpr const char* leaf = "scenes/shared/spot08_scene/spot08_room_0DL_002538_tri_0";
    bool callsLeaf = false;
    for (const auto& c : list->Instructions)
        if ((c.words.w0 >> 24) == 0x27 && c.words.w1 &&
            std::strcmp(reinterpret_cast<const char*>(c.words.w1), leaf) == 0)
            callsLeaf = true;
    if (!callsLeaf)
        return false;
    auto vertices = std::dynamic_pointer_cast<Fast::Vertex>(
        ResourceLoad("scenes/shared/spot08_scene/spot08_room_0DL_002538_vtx_0"));
    if (!vertices || vertices->VertexList.empty())
        return false;
    for (const auto& v : vertices->VertexList)
        if (v.v.ob[1] < 1000)
            return false;
    sDetailedRender = true;
    return true;
}
struct ValleyActor {
    DynaPolyActor dyna;
};
// Process-owned, fixed-size buffers outlive native DynaPoly teardown. Only one
// scene instance is registered; no archive collision/resource buffer is edited.
std::array<Vec3s, 43 * 3> sVertices{};
std::array<CollisionPoly, 43> sPolys{};
std::array<Vtx, 43 * 3> sDrawVertices{};
SurfaceType sSurface[1]{};
Vec3s sCameraPos[1]{};
CamData sCamera[1] = { { CAM_SET_NONE, 0, sCameraPos } };
CollisionHeader sCollision{};
ValleyActor* sActor = nullptr;
PlayState* sOwner = nullptr;
CollisionHeader* sSceneCollision = nullptr;
int sActorId = -1;
bool Fountain(const PlayState* play) {
    return play && Combo_IsCombinedGame() && gSaveContext.gameMode == GAMEMODE_NORMAL &&
           ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) && play->sceneNum == SCENE_ZORAS_FOUNTAIN &&
           play->roomCtx.curRoom.num == 0;
}
bool SamePoint(const Vec3s& v, const Point& p) {
    return v.x == p.x && v.y == p.y && v.z == p.z;
}
bool SceneContains(const CollisionHeader* h, const Triangle& t) {
    if (!h || !h->vtxList || !h->polyList)
        return false;
    const Point points[] = { t.a, t.b, t.c };
    for (size_t i = 0; i < h->numPolygons; ++i) {
        const auto& p = h->polyList[i];
        const unsigned indices[] = { unsigned(p.flags_vIA & 0x1fff), unsigned(p.flags_vIB & 0x1fff),
                                     unsigned(p.vIC & 0x1fff) };
        if (indices[0] >= h->numVertices || indices[1] >= h->numVertices || indices[2] >= h->numVertices)
            continue;
        bool match = true;
        for (const auto& point : points) {
            bool found = false;
            for (auto index : indices)
                found |= SamePoint(h->vtxList[index], point);
            match &= found;
        }
        if (match)
            return true;
    }
    return false;
}
void Build(CollisionHeader* scene) {
    sCollision = {};
    size_t count = 0;
    for (size_t i = 0; i < kTriangles.size(); ++i) {
        const auto& t = kTriangles[i];
        const Point points[] = { t.a, t.b, t.c };
        for (size_t j = 0; j < 3; ++j) {
            auto& d = sDrawVertices[i * 3 + j].v;
            d.ob[0] = s16(points[j].x);
            d.ob[1] = s16(points[j].y);
            d.ob[2] = s16(points[j].z);
            // One shared origin keeps every S in signed16 range without changing
            // density or seams. The 16384-unit shift is 16 whole 32-pixel tiles.
            d.tc[0] = s16((points[j].x - 2048) * 8);
            d.tc[1] = s16((t.floor ? points[j].z : points[j].y) * 8);
            d.cn[0] = d.cn[1] = d.cn[2] = d.cn[3] = 255;
        }
        if (SceneContains(scene, t))
            continue;
        for (size_t j = 0; j < 3; ++j)
            sVertices[count * 3 + j] = { s16(points[j].x), s16(points[j].y), s16(points[j].z) };
        float ux = t.b.x - t.a.x, uy = t.b.y - t.a.y, uz = t.b.z - t.a.z;
        float vx = t.c.x - t.a.x, vy = t.c.y - t.a.y, vz = t.c.z - t.a.z;
        float nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
        float len = std::sqrt(nx * nx + ny * ny + nz * nz);
        nx /= len;
        ny /= len;
        nz /= len;
        auto& poly = sPolys[count];
        // Deliberate remap: donor types0 and37 become a local ordinary
        // non-exit/non-camera/non-conveyor surface, never an unowned type index.
        poly.type = 0;
        poly.flags_vIA = count * 3;
        poly.flags_vIB = count * 3 + 1;
        poly.vIC = count * 3 + 2;
        poly.normal = { s16(std::lround(nx * 32767)), s16(std::lround(ny * 32767)), s16(std::lround(nz * 32767)) };
        poly.dist = s16(std::lround(-(nx * t.a.x + ny * t.a.y + nz * t.a.z)));
        ++count;
    }
    if (count) {
        sCollision.minBounds = sCollision.maxBounds = sVertices[0];
        for (size_t i = 0; i < count * 3; ++i) {
            const auto& v = sVertices[i];
            auto& lo = sCollision.minBounds;
            auto& hi = sCollision.maxBounds;
            lo.x = std::min(lo.x, v.x);
            lo.y = std::min(lo.y, v.y);
            lo.z = std::min(lo.z, v.z);
            hi.x = std::max(hi.x, v.x);
            hi.y = std::max(hi.y, v.y);
            hi.z = std::max(hi.z, v.z);
        }
    }
    sCollision.numVertices = count * 3;
    sCollision.vtxList = sVertices.data();
    sCollision.numPolygons = count;
    sCollision.polyList = sPolys.data();
    sCollision.surfaceTypeList = sSurface;
    sCollision.cameraDataList = sCamera;
    sCollision.cameraDataListLen = 1;
}
void Destroy(Actor* actor, PlayState* play) {
    auto* self = reinterpret_cast<ValleyActor*>(actor);
    if (self->dyna.bgId >= 0 && self->dyna.bgId < BG_ACTOR_MAX)
        DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, self->dyna.bgId);
    if (sActor == self) {
        RestoreBackdrop();
        sActor = nullptr;
        sOwner = nullptr;
        sSceneCollision = nullptr;
    }
}
void Init(Actor* actor, PlayState* play) {
    auto* self = reinterpret_cast<ValleyActor*>(actor);
    DynaPolyActor_Init(&self->dyna, 0);
    self->dyna.bgId = BG_ACTOR_MAX;
    Actor_SetScale(actor, 1);
    actor->room = 0;
    Build(play->colCtx.colHeader);
    if (sCollision.numPolygons) {
        self->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, actor, &sCollision);
        if (self->dyna.bgId < 0 || self->dyna.bgId >= BG_ACTOR_MAX) {
            Actor_Kill(actor);
            return;
        }
    }
    sActor = self;
    sOwner = play;
    sSceneCollision = play->colCtx.colHeader;
}
void Update(Actor* actor, PlayState* play) {
    if (!Fountain(play))
        Actor_Kill(actor);
}
void Draw(Actor*, PlayState* play) {
    if (!ComboFountainValley_Ready(play))
        return;
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    Matrix_Translate(0, 0, 0, MTXMODE_NEW);
    gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPClearGeometryMode(POLY_OPA_DISP++, G_LIGHTING | G_CULL_BACK);
    gSPTexture(POLY_OPA_DISP++, 0xffff, 0xffff, 0, G_TX_RENDERTILE, G_ON);
    gDPSetTextureLUT(POLY_OPA_DISP++, G_TT_NONE);
    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATERGBA, G_CC_MODULATERGBA);
    for (bool floor : { false, true }) {
        if (!floor && sDetailedRender)
            continue; // Keep the existing detailed walls and their UVs.
        auto material =
            floor ? RegionalBridgeMaterials::Material::ValleySnow : RegionalBridgeMaterials::Material::SnowheadStone;
        const char* texture =
            RegionalBridgeMaterials::Resolve(material, [](const char* p) { return ResourceMgr_FileExists(p); });
        gDPPipeSync(POLY_OPA_DISP++);
        if (floor && sDetailedRender) {
            // Exact coplanar decal: no raised duplicate ground or z-fighting.
            gDPSetRenderMode(POLY_OPA_DISP++, G_RM_AA_ZB_OPA_DECAL, G_RM_AA_ZB_OPA_DECAL2);
        } else {
            gDPSetRenderMode(POLY_OPA_DISP++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
        }
        gDPLoadTextureBlock(POLY_OPA_DISP++, texture, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0, G_TX_WRAP, G_TX_WRAP, 5,
                            5, G_TX_NOLOD, G_TX_NOLOD);
        for (size_t i = 0; i < kTriangles.size(); ++i)
            if (kTriangles[i].floor == floor) {
                gSPVertex(POLY_OPA_DISP++, reinterpret_cast<uintptr_t>(&sDrawVertices[i * 3]), 3, 0);
                gSP1Triangle(POLY_OPA_DISP++, 0, 1, 2, 0);
            }
    }
    gSPSetGeometryMode(POLY_OPA_DISP++, G_LIGHTING | G_CULL_BACK);
    CLOSE_DISPS(play->state.gfxCtx);
}
void Ensure(PlayState* play) {
    if (!Fountain(play) || !play->colCtx.colHeader)
        return;
    if (sActor && sOwner == play && sActor->dyna.actor.update)
        return;
    if (sActorId < 0) {
        ActorDBInit init;
        init.name = "Bg_ComboFountainValley";
        init.desc = "Shared fitted Snowhead valley";
        init.category = ACTORCAT_BG;
        init.objectId = OBJECT_GAMEPLAY_KEEP;
        init.instanceSize = sizeof(ValleyActor);
        init.flags = ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
        init.init = Init;
        init.destroy = Destroy;
        init.update = Update;
        init.draw = Draw;
        sActorId = ActorDB::Instance->AddEntry(init).entry.id;
    }
    Actor_Spawn(&play->actorCtx, play, sActorId, 0, 0, 0, 0, 0, 0, 0);
}
void Register() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>([] { Ensure(gPlayState); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawBegin>([] {
        if (Fountain(gPlayState))
            ComboFountainValley_Ready(gPlayState);
        else
            RestoreBackdrop();
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([] {
        RestoreBackdrop();
        sActor = nullptr;
        sOwner = nullptr;
        sSceneCollision = nullptr;
    });
}
RegisterShipInitFunc valleyInit(Register);
} // namespace
extern "C" int ComboFountainValley_Ready(PlayState* play) {
    if (!Fountain(play) || !PrepareBackdrop())
        return 0;
    Ensure(play);
    const bool ready =
        sActor && sOwner == play && sActor->dyna.actor.update && sSceneCollision == play->colCtx.colHeader;
    if (!ready)
        RestoreBackdrop(); // Never expose the native opening without its fitted collision.
    return ready;
}
#endif
