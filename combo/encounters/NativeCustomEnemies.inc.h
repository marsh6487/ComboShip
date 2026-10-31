// Included once by each game's native adapter. No scene data is changed.
#include "EncounterPolicy.h"
#include "NativeCustomEnemies.h"
#include <cmath>
#include <array>
extern "C" int NativeLynel_Register(void);
extern "C" void CustomEnemies_ReportSkipped(int, int, int, int);
extern "C" uint8_t ResourceMgr_FileExists(const char*);
extern "C" int NativeLynel_Id(void);
#if !NEI_MM
extern "C" {
extern s16 gEnMiniblinId, gEnHammergeistId, gEnSbeetleId;
}
extern "C" void Trutefel_EnsureActorsRegistered(void);
#endif
#if NEI_MM
#define EncounterScene(play) ((play)->sceneId)
#else
#define EncounterScene(play) ((play)->sceneNum)
#endif
namespace {
using CustomEncounters::Enemy;
struct Habitat {
    int scene;
    unsigned mask;
};
#if NEI_MM
constexpr Habitat habitats[] = { { 0x00, 5 }, { 0x13, 10 }, { 0x1C, 5 }, { 0x2D, 15 }, { 0x37, 12 },
                                 { 0x40, 5 }, { 0x45, 5 },  { 0x50, 6 }, { 0x53, 9 },  { 0x5A, 6 },
                                 { 0x5B, 6 }, { 0x5D, 6 },  { 0x5E, 6 } };
#else
constexpr Habitat habitats[] = { { 0x51, 15 }, { 0x54, 5 },  { 0x57, 12 }, { 0x5A, 9 },
                                 { 0x5B, 5 },  { 0x5C, 10 }, { 0x5E, 9 },  { 0x60, 6 } };
#endif
constexpr const char* keys[] = { "gEnhancements.CustomEnemies.Miniblin", "gEnhancements.CustomEnemies.Molmauk",
                                 "gEnhancements.CustomEnemies.Beetle", "gEnhancements.CustomEnemies.Lynel" };
constexpr const char* gates[] = {
    "__OTR__objects/trutefel/object_miniblin/gMiniblinSkel_body_mesh_layer_Opaque",
    "__OTR__objects/trutefel/object_hammergeist/gHammergeistSkel_body_mesh_layer_Opaque",
    "__OTR__objects/trutefel/object_sbeetle/gScissorsBeetleSkel_bodyfront_mesh_layer_Opaque",
    "__OTR__objects/object_lynel/gLynelRootDL"
};
struct Anchor {
    Vec3f pos{};
    int room = -1;
    Actor* actor = nullptr;
    int kind = 0;
};
std::array<Anchor, 64> anchors{};
size_t anchorCount = 0;
PlayState* encounterPlay = nullptr;
int encounterScene = -1;
CustomEncounters::Visit encounterVisit;
unsigned RegionMask(int scene) {
    for (auto h : habitats)
        if (h.scene == scene)
            return h.mask;
    return 0;
}
bool Source(int id, int params) {
    // Deliberately exclude bosses, flying/aquatic actors, scripted fights and split components.
    switch (id) {
        case ACTOR_EN_TITE:
            return params == -1 || params == -2;
        case ACTOR_EN_DEKUBABA:
            return params == 0 || params == 1;
        default:
            return false;
    }
}
Actor* First(const ActorListEntry& list) {
#if NEI_MM
    return list.first;
#else
    return list.head;
#endif
}
bool Live(Actor* actor) {
    if (!actor)
        return false;
    for (auto& list : gPlayState->actorCtx.actorLists)
        for (Actor* a = First(list); a; a = a->next)
            if (a == actor)
                return a->update && CustomEnemies_Kind(a->id) > 0;
    return false;
}
unsigned Selected() {
    unsigned mask = 0;
    for (int i = 0; i < 4; ++i)
        if (CVarGetInteger(keys[i], 1) && CustomEnemies_Id(i + 1) >= 0)
            mask |= 1u << i;
    return mask;
}
float Floor(PlayState* play, Vec3f p, CollisionPoly** poly, int* bg) {
    p.y += 120;
#if NEI_MM
    return BgCheck_EntityRaycastFloor5(&play->colCtx, poly, bg, &GET_PLAYER(play)->actor, &p);
#else
    return BgCheck_EntityRaycastFloor5(play, &play->colCtx, poly, bg, &GET_PLAYER(play)->actor, &p);
#endif
}
bool Ground(PlayState* play, Vec3f& p, int kind) {
    CollisionPoly* poly = nullptr;
    int bg = 0;
    float y = Floor(play, p, &poly, &bg);
    if (!std::isfinite(y) || !poly || poly->normal.y < 26000 || std::fabs(y - p.y) > 100 || bg != BGCHECK_SCENE)
        return false;
    Vec3f ceilingPos = { p.x, y + 5, p.z };
    float ceilingY;
    CollisionPoly* ceiling = nullptr;
    int ceilingBg;
    if (BgCheck_EntityCheckCeiling(&play->colCtx, &ceilingY, &ceilingPos, (kind == 2 || kind == 4) ? 220 : 80, &ceiling,
                                   &ceilingBg, &GET_PLAYER(play)->actor))
        return false;
    float water;
    WaterBox* box = nullptr;
    if (WaterBox_GetSurface1(play, &play->colCtx, p.x, p.z, &water, &box) && water > y - 5)
        return false;
    const float radius = (kind == 2 || kind == 4) ? 180.0f : 65.0f;
    for (int i = 0; i < 8; ++i) {
        float angle = i * 0.785398163f;
        Vec3f q = { p.x + std::cos(angle) * radius, y, p.z + std::sin(angle) * radius };
        CollisionPoly* edge = nullptr;
        int edgeBg;
        float edgeY = Floor(play, q, &edge, &edgeBg);
        Vec3f start = { p.x, y + 60, p.z }, end = { q.x, y + 60, q.z }, hit;
        CollisionPoly* wall = nullptr;
        int wallBg;
        if (BgCheck_EntityLineTest1(&play->colCtx, &start, &end, &hit, &wall, true, false, false, true, &wallBg))
            return false;
        if (!edge || edgeBg != BGCHECK_SCENE || edge->normal.y < 26000 || std::fabs(edgeY - y) > 35)
            return false;
    }
    p.y = y;
    return true;
}
void EnsureVisit(PlayState* play) {
    if (encounterPlay != play || encounterScene != EncounterScene(play)) {
        CustomEnemies_Reset();
        encounterPlay = play;
        encounterScene = EncounterScene(play);
        encounterVisit.Enter(EncounterScene(play), 0, 0);
    }
}
} // namespace
extern "C" int CustomEnemies_Id(int kind) {
    if (kind < 1 || kind > 4 || !ResourceMgr_FileExists(gates[kind - 1]))
        return -1;
    if (kind == 4)
        return NativeLynel_Register();
#if NEI_MM
    const int ids[] = { ACTOR_EN_MINIBLIN, ACTOR_EN_HAMMERGEIST, ACTOR_EN_SBEETLE };
    return ids[kind - 1];
#else
    Trutefel_EnsureActorsRegistered();
    const int ids[] = { gEnMiniblinId, gEnHammergeistId, gEnSbeetleId };
    return ids[kind - 1];
#endif
}
extern "C" int CustomEnemies_CanSpawn(PlayState* play, int kind, float x, float y, float z) {
    if (!play || kind < 1 || kind > 4 || !GET_PLAYER(play) || CustomEnemies_Id(kind) < 0)
        return false;
    Vec3f p = { x, y, z };
    return Ground(play, p, kind);
}
extern "C" int CustomEnemies_Kind(int id) {
    // ID lookup never registers actors or touches resources in the frame loop.
#if NEI_MM
    const int ids[] = { ACTOR_EN_MINIBLIN, ACTOR_EN_HAMMERGEIST, ACTOR_EN_SBEETLE, ACTOR_EN_NEI_LYNEL };
#else

    const int ids[] = { gEnMiniblinId, gEnHammergeistId, gEnSbeetleId, NativeLynel_Id() };
#endif
    for (int i = 0; i < 4; ++i)
        if (ids[i] >= 0 && ids[i] == id)
            return i + 1;
    return 0;
}
extern "C" void CustomEnemies_ActorDestroyed(Actor* actor) {
    if (!actor)
        return;
    // Clear identities before memory can be reused by any actor with the same ID.
    for (size_t i = 0; i < anchorCount; ++i)
        if (anchors[i].actor == actor) {
            anchors[i].actor = nullptr;
            encounterVisit.Defeated(i);
        }
}
extern "C" void CustomEnemies_Reset(void) {
    // Called before PlayState destruction. Do not dereference old actor pointers.
    anchorCount = 0;
    anchors = {};
    encounterPlay = nullptr;
    encounterScene = -1;
    encounterVisit = CustomEncounters::Visit{};
}
extern "C" void CustomEnemies_RecordEntry(PlayState* play, int16_t id, float x, float y, float z, int16_t params) {
    EnsureVisit(play);
    if (!RegionMask(EncounterScene(play)) || !(Source(id, params) || id == ACTOR_EN_ENCOUNT1) ||
        anchorCount == anchors.size())
        return;
    for (size_t i = 0; i < anchorCount; ++i)
        if (anchors[i].pos.x == x && anchors[i].pos.y == y && anchors[i].pos.z == z)
            return;
    // Cache native spawn locations at scene load, including when disabled. This allows later opt-in
    // and simultaneous randomization without scanning actor pointers or editing scene resources.
    anchors[anchorCount++] = { { x, y, z }, play->roomCtx.curRoom.num, nullptr, 0 };
}
extern "C" void CustomEnemies_ReplaceEntry(PlayState* play, int16_t* id, int16_t* params, float x, float y, float z) {
#if NEI_MM
    if (!Partners_IsActive() || !CVarGetInteger("gEnhancements.CustomEnemies.Randomizer", 0) || !Source(*id, *params) ||
        !RegionMask(EncounterScene(play)))
        return;
    unsigned seed = static_cast<unsigned>(static_cast<int>(x) * 31 + static_cast<int>(z) * 17 + EncounterScene(play));
    int kind = static_cast<int>(CustomEncounters::Choose(seed, Selected(), false));
    Vec3f pos = { x, y, z };
    int replacement = CustomEnemies_Id(kind);
    if (replacement >= 0 && Ground(play, pos, kind)) {
        *id = replacement;
        *params = 0;
    }
#endif
}
extern "C" void CustomEnemies_Tick(void) {
    if (!Partners_IsActive())
        return;
    auto* play = gPlayState;
    EnsureVisit(play);
    bool enabled = CVarGetInteger("gEnhancements.CustomEnemies.NaturalEncounters", 0);
    if (!enabled) {
        for (size_t i = 0; i < anchorCount; ++i) {
            auto& a = anchors[i];
            if (Live(a.actor))
                Actor_Kill(a.actor);
            else if (a.actor)
                encounterVisit.Defeated(i);
        }
        for (auto& a : anchors)
            a.actor = nullptr;
        encounterVisit.Disable();
        return;
    }
    if (!anchorCount || play->pauseCtx.state || play->msgCtx.msgMode != MSGMODE_NONE ||
        play->csCtx.state != CS_STATE_IDLE || play->transitionTrigger != TRANS_TRIGGER_OFF ||
        play->transitionMode != TRANS_MODE_OFF)
        return;
    // At most two additions per outdoor region, far from Link and authored NPCs/doors.
    unsigned mask = RegionMask(EncounterScene(play)) & Selected();
    size_t count = anchorCount < 2 ? anchorCount : 2;
    for (size_t i = 0; i < count; ++i) {
        auto& a = anchors[i];
        if (a.actor) {
            if (!Live(a.actor)) {
                encounterVisit.Defeated(i);
                a.actor = nullptr;
            }
            continue;
        }
        if (a.room != play->roomCtx.curRoom.num || !encounterVisit.Available(i))
            continue;
        int kind = static_cast<int>(CustomEncounters::Choose(EncounterScene(play) * 7 + i, mask, false));
        if (!kind)
            continue;
        Vec3f pos = a.pos;
        pos.x += 240;
        pos.z += 240;
        auto* player = GET_PLAYER(play);
        float dx = pos.x - player->actor.world.pos.x, dz = pos.z - player->actor.world.pos.z;
        if (dx * dx + dz * dz < 450 * 450)
            continue;
        dx = pos.x - player->actor.home.pos.x;
        dz = pos.z - player->actor.home.pos.z;
        if (dx * dx + dz * dz < 400 * 400)
            continue;
        bool blocked = false;
        for (auto& list : play->actorCtx.actorLists)
            for (Actor* other = First(list); other; other = other->next) {
                if (other->category != ACTORCAT_NPC && other->category != ACTORCAT_DOOR)
                    continue;
                float x = pos.x - other->world.pos.x, z = pos.z - other->world.pos.z;
                if (x * x + z * z < 400 * 400)
                    blocked = true;
            }
        if (blocked)
            continue;
        bool valid = Ground(play, pos, kind);
        if (!valid)
            CustomEnemies_ReportSkipped(EncounterScene(play), a.room, static_cast<int>(i), kind);
        if (!encounterVisit.Request(i, true, valid, true))
            continue;
        int id = CustomEnemies_Id(kind);
        if (id < 0)
            continue;
        a.actor = Actor_Spawn(&play->actorCtx, play, id, pos.x, pos.y, pos.z, 0, 0, 0, 0);
        if (a.actor) {
            a.actor->room = -1;
            a.kind = kind;
            encounterVisit.Spawned(i);
        }
    }
}
