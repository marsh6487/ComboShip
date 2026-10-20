#include "2s2h/Enhancements/Gameplay/ComboSaria.h"
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "2s2h/Enhancements/Gameplay/ComboForestActors.h"
#include "2s2h/Enhancements/Gameplay/ComboForestEpona.h"
#include "actors/PlacedSariaABI.h"
#include "actors/SariaHexEffect.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "ComboResolve.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/CustomItem/CustomItem.h"
#include <nlohmann/json.hpp>
#include "2s2h/Rando/Rando.h"
#include "2s2h/Rando/ItemReceiptText.h"
#include <cmath>
#include <memory>
#include <unordered_map>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#define CHECK(x)                                                      \
    do {                                                              \
        if (!(x)) {                                                   \
            std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); \
            std::exit(1);                                             \
        }                                                             \
    } while (0)

static unsigned loads, releases, kills, oc, colliderDestroyed;
static bool missing;
static int alt;
static unsigned spawns;
static bool hasFloor = true;
static Actor spawned;
static unsigned skeletonDraws;
static unsigned lastPose;
static bool talkAccepted;
static u8 textState = TEXT_STATE_NONE;
static unsigned rewards, saves, receipts, hintRegistrations;
static unsigned sparkles;
static Vec3f sparklePos;
static Color_RGBA8 sparklePrim, sparkleEnv;
static Vec3f shimmerOrigin;
static GameInteractor interactor;
GameInteractor* GameInteractor::Instance = &interactor;
namespace Rando {
void GiveItem(RandoItemId id, RandoCheckId) {
    CHECK(id == RI_SONG_SARIA);
    ++rewards;
    ++gSaveContext.save.shipSaveInfo.rando.sariaHintsAvailable;
    gSaveContext.save.shipSaveInfo.nei.ootQuestItems |= (1u << OOT_QUEST_SONG_SARIA);
}
void DrawItem(RandoItemId id, RandoCheckId, Actor*) {
    CHECK(id == RI_SONG_SARIA);
}
bool ApplyItemReceiptText(RandoItemId id, CustomMessage::Entry& entry) {
    CHECK(id == RI_SONG_SARIA);
    entry.msg = "Saria's Song";
    return true;
}
namespace StaticData {
u8 GetIconForZMessage(RandoItemId id) {
    CHECK(id == RI_SONG_SARIA);
    return 0xCF;
}
} // namespace StaticData
namespace MiscBehavior {
void SariasSongHint() {
    ++hintRegistrations;
}
} // namespace MiscBehavior
} // namespace Rando
namespace CustomMessage {
void SetActiveCustomMessage(std::string msg, Entry) {
    CHECK(msg == "Saria's Song");
    ++receipts;
}
void StartTextbox(std::string msg, Entry) {
    CHECK(msg == "Saria's Song");
    ++receipts;
}
} // namespace CustomMessage
void SaveManager_SaveCurrentForCombo() {
    CHECK(gSaveContext.save.shipSaveInfo.nei.comboSariaSongClaimed);
    ++saves;
}
void FrameInterpolation_RecordOpenChild(const void*, int) {
}
void FrameInterpolation_RecordCloseChild() {
}
struct Owner {
    void* limbs[16]{};
    FlexSkeletonHeader skeleton{};
    AnimationHeader animation{};
    Owner() {
        skeleton.sh.segment = limbs;
        skeleton.sh.limbCount = 16;
        skeleton.dListCount = 15;
        animation.common.frameCount = 24;
    }
};
static int Load(unsigned pose, ComboPlacedSariaResources* output) {
    ++loads;
    lastPose = pose;
    if (missing)
        return 0;
    auto* owner = new Owner;
    *output = {};
    output->owner = owner;
    output->skeleton = &owner->skeleton;
    output->animation = &owner->animation;
    output->altEnabled = alt;
    output->custom = alt;
    output->heads[0] = "open";
    output->heads[1] = "half";
    output->heads[2] = "closed";
    return 1;
}
static void Release(void* owner) {
    ++releases;
    delete static_cast<Owner*>(owner);
}
static int Alt() {
    return alt;
}
int MM_PlacedSariaLoad(unsigned pose, ComboPlacedSariaResources* output) {
    return Load(pose, output);
}
void MM_PlacedSariaRelease(void* owner) {
    Release(owner);
}
int MM_PlacedSariaAlt() {
    return Alt();
}

extern "C" {
void gSPVertex(Gfx* command, uintptr_t vertices, int count, int first) {
    __gSPVertex(command, vertices, count, first);
}
void gSPSegment(void* command, int segment, uintptr_t target) {
    __gSPSegment(static_cast<Gfx*>(command), segment, target);
}
SaveContext gSaveContext{};
PlayState* gPlayState;
GameState* gGameState;
int gComboGameMode;
bool Player_InCsMode(PlayState*) {
    return false;
}
f32 BgCheck_EntityRaycastFloor5(CollisionContext*, CollisionPoly** poly, s32*, Actor*, Vec3f*) {
    static CollisionPoly floor;
    *poly = hasFloor ? &floor : nullptr;
    return hasFloor ? 42.0f : BGCHECK_Y_MIN;
}
Actor* Actor_Spawn(ActorContext*, PlayState*, s16 id, f32 x, f32 y, f32 z, s16, s16 yaw, s16, s32 params) {
    ++spawns;
    spawned = {};
    spawned.id = id;
    spawned.params = params;
    spawned.world.pos = { x, y, z };
    spawned.shape.rot.y = yaw;
    return &spawned;
}
void Actor_Kill(Actor*) {
    ++kills;
}
void Actor_ChangeCategory(PlayState*, ActorContext*, Actor* a, u8 category) {
    a->category = category;
}
void Actor_SetScale(Actor* a, f32 scale) {
    a->scale.x = a->scale.y = a->scale.z = scale;
}
void ActorShape_Init(ActorShape*, f32, ActorShadowFunc, f32) {
}
void ActorShadow_DrawCircle(Actor*, Lights*, PlayState*) {
}
void Actor_SetFocus(Actor* a, f32 height) {
    a->focus.pos = a->world.pos;
    a->focus.pos.y += height;
}
s32 Collider_InitCylinder(PlayState*, ColliderCylinder*) {
    return 1;
}
s32 Collider_SetCylinder(PlayState*, ColliderCylinder* c, Actor*, ColliderCylinderInit* init) {
    c->dim = init->dim;
    return 1;
}
s32 Collider_DestroyCylinder(PlayState*, ColliderCylinder*) {
    ++colliderDestroyed;
    return 1;
}
void Collider_UpdateCylinder(Actor*, ColliderCylinder*) {
}
s32 CollisionCheck_SetOC(PlayState*, CollisionCheckContext*, Collider*) {
    ++oc;
    return 1;
}
s16 Rand_S16Offset(s16 base, s16) {
    return base;
}
s32 Actor_TalkOfferAccepted(Actor*, GameState*) {
    bool result = talkAccepted;
    talkAccepted = false;
    return result;
}
s32 Actor_OfferTalk(Actor*, PlayState*, f32) {
    return 1;
}
u8 Message_GetState(MessageContext*) {
    return textState;
}
void SkelAnime_InitFlex(PlayState*, SkelAnime* a, FlexSkeletonHeader* s, AnimationHeader* animation, Vec3s* joints,
                        Vec3s* morph, s32 count) {
    a->limbCount = count;
    a->dListCount = s->dListCount;
    a->skeleton = s->sh.segment;
    a->jointTable = joints;
    a->morphTable = morph;
    a->animation = animation;
    a->curFrame = 0;
    a->endFrame = animation->common.frameCount - 1;
    joints[0].y = 7903;
}
s32 SkelAnime_Update(SkelAnime* a) {
    a->jointTable[0].y = 7903;
    a->curFrame = std::fmod(a->curFrame + 1, 24.0f);
    return 0;
}
void Graph_OpenDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {
}
void Graph_CloseDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {
}
void Gfx_SetupDL25_Opa(GraphicsContext*) {
}
void Gfx_SetupDL25_Xlu(GraphicsContext*) {
}
void Matrix_Push() {
}
void Matrix_Pop() {
}
void Matrix_Translate(f32 x, f32 y, f32 z, MatrixMode mode) {
    CHECK(mode == MTXMODE_NEW);
    shimmerOrigin = { x, y, z };
}
Mtx* Matrix_Finalize(GraphicsContext*) {
    static Mtx matrix;
    return &matrix;
}
void Matrix_Scale(f32, f32, f32, MatrixMode) {
}
void EffectSsKirakira_SpawnDispersed(PlayState*, Vec3f* pos, Vec3f*, Vec3f*, Color_RGBA8* prim, Color_RGBA8* env,
                                     s16 scale, s32 life) {
    ++sparkles;
    sparklePos = *pos;
    sparklePrim = *prim;
    sparkleEnv = *env;
    CHECK(scale > 0 && scale <= 600 && life == 24);
}
void SkelAnime_DrawFlexOpa(PlayState*, void**, Vec3s*, s32, OverrideLimbDrawOpa, PostLimbDrawOpa, Actor*) {
    ++skeletonDraws;
}
}

/* PRODUCTION_SARIA_RUNTIME */

int main() {
    static PlayState play;
    Actor first{}, second{}, absent{};
    first.params = 0x7F03;
    second.params = 0x7F13;
    first.world.pos = { 20, 30, 40 };
    first.world.rot = { 100, 200, 300 };
    sBridge.load = Load;
    sBridge.release = Release;
    sBridge.alt = Alt;
    CHECK(ComboSaria_IsParam(0x7F03) && ComboSaria_IsParam(0x7F13) && !ComboSaria_IsParam(1));
    CHECK(ComboSaria_IsParam(0x7E13) && ComboSaria_IsParam(0x7E23) && !ComboSaria_IsParam(0x7E03));
    missing = true;
    ComboSaria_Init(&absent, &play);
    CHECK(kills == 1 && sStates.empty() && releases == 0);
    ComboSaria_Destroy(&absent, &play);
    missing = false;
    ComboSaria_Init(&first, &play);
    ComboSaria_Init(&second, &play);
    CHECK(sStates.size() == 2 && first.category == ACTORCAT_NPC && first.scale.x == 0.01f);
    CHECK(first.shape.rot.x == 0 && first.shape.rot.y == 200 && first.shape.rot.z == 0);
    CHECK(!(first.flags & ACTOR_FLAG_UPDATE_DURING_SOARING_AND_SOT_CS));
    auto& state = *sStates.at(&first);
    CHECK(state.animation.jointTable == state.joints && state.animation.morphTable == state.morph);
    CHECK(state.collider.dim.radius == 8 && state.collider.dim.height == 46);
    CHECK(first.floorPoly && first.floorHeight == 42 && first.world.pos.y == 30);
    static GraphicsContext gfx;
    Gfx commands[32]{};
    Gfx hexCommands[512]{};
    alignas(16) Vtx hexVertices[144]{};
    gfx.polyOpa.p = commands;
    gfx.polyOpa.d = reinterpret_cast<Gfx*>(hexVertices + 144);
    gfx.polyXlu.p = hexCommands;
    play.state.gfxCtx = &gfx;
    ComboSaria_Draw(&first, &play);
    Gfx* segmentC = nullptr;
    bool env = false;
    for (Gfx* command = commands; command != gfx.polyOpa.p; ++command) {
        u32 word = command->words.w0;
        if ((word >> 24) == G_MOVEWORD && (word & 0xFFFFFF) == ((G_MW_SEGMENT << 16) | (12 * 4)))
            segmentC = reinterpret_cast<Gfx*>(command->words.w1);
        if ((word >> 24) == G_SETENVCOLOR && command->words.w1 == 255)
            env = true;
    }
    CHECK(skeletonDraws == 1 && segmentC && (segmentC->words.w0 >> 24) == G_ENDDL && env);
    state.blinkTimer = 1;
    ComboSaria_Update(&first, &play);
    CHECK(state.eye == 1 && sStates.at(&second)->eye == 0 && oc == 1);
    ComboSaria_Update(&first, &play);
    CHECK(state.eye == 2);
    Gfx* display = nullptr;
    Override(&play, 16, &display, nullptr, nullptr, &first);
    CHECK(reinterpret_cast<const char*>(display) == state.resources.heads[2]);
    const float previousFrame = state.animation.curFrame;
    alt = 1;
    ComboSaria_Update(&first, &play);
    CHECK(releases == 1 && state.resources.altEnabled == 1 && state.animation.curFrame == previousFrame + 1);
    CHECK(first.world.pos.x == 20 && first.world.pos.y == 30 && first.world.pos.z == 40 && first.textId == 0x8F28);
    missing = true;
    alt = 0;
    ComboSaria_Update(&first, &play);
    CHECK(kills == 2 && releases == 1);
    ComboSaria_Destroy(&first, &play);
    ComboSaria_Destroy(&first, &play);
    ComboSaria_Destroy(&second, &play);
    CHECK(sStates.empty() && releases == 3 && colliderDestroyed == 2);
    missing = false;
    ComboSaria_Init(&first, &play);
    ComboSaria_Init(&second, &play);
    ResetSariaResources();
    ResetSariaResources();
    CHECK(sStates.empty() && releases == 5);
    alt = 1;
    ComboSaria_Init(&second, &play);
    auto& waiting = *sStates.at(&second);
    CHECK(lastPose == 1 && waiting.resources.custom);
    for (unsigned eye : { 0u, 1u, 2u, 0u }) {
        CHECK(waiting.eye == eye);
        Gfx* head = nullptr;
        Override(&play, 16, &head, nullptr, nullptr, &second);
        CHECK(reinterpret_cast<const char*>(head) == waiting.resources.heads[eye]);
        waiting.blinkTimer = 1;
        ComboSaria_Update(&second, &play);
    }
    ComboSaria_Destroy(&second, &play);
    CHECK(sStates.empty() && releases == 6);
    gPlayState = &play;
    gGameState = &play.state;
    play.state.running = true;
    play.sceneId = SCENE_LOST_WOODS;
    gComboGameMode = COMBO_GAME_BOTH;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    gSaveContext.save.entrance = ENTRANCE(LOST_WOODS, 0);
    gSaveContext.fileNum = 0;
    SaveContext before = gSaveContext;
    gSaveContext.save.cutsceneIndex = 1;
    SpawnForestSaria();
    CHECK(spawns == 0 && !sSpawnAttempted);
    gSaveContext = before;
    gComboGameMode = COMBO_GAME_MM;
    SpawnForestSaria();
    CHECK(spawns == 0);
    gComboGameMode = COMBO_GAME_BOTH;
    play.roomCtx.curRoom.num = 1;
    SpawnForestSaria();
    CHECK(spawns == 0);
    play.roomCtx.curRoom.num = 0;
    play.pauseCtx.state = 1;
    SpawnForestSaria();
    CHECK(spawns == 0);
    play.pauseCtx.state = PAUSE_STATE_OFF;
    gGameState = nullptr;
    SpawnForestSaria();
    CHECK(spawns == 0);
    gGameState = &play.state;
    play.roomCtx.status = 1;
    play.roomCtx.curRoom.segment = nullptr;
    SpawnForestSaria();
    CHECK(spawns == 0 && !sSpawnAttempted);
    play.roomCtx.status = 0;
    SpawnForestSaria();
    CHECK(spawns == 0 && !sSpawnAttempted);
    play.roomCtx.curRoom.segment = reinterpret_cast<void*>(1);
    Actor authored{};
    authored.id = ACTOR_EN_VIEWER;
    authored.params = 0x7F13;
    play.actorCtx.actorLists[ACTORCAT_NPC].first = &authored;
    SpawnForestSaria();
    CHECK(spawns == 0 && sSpawnAttempted);
    play.actorCtx.actorLists[ACTORCAT_NPC].first = nullptr;
    sSpawnAttempted = false;
    hasFloor = false;
    SpawnForestSaria();
    CHECK(spawns == 0 && sSpawnAttempted);
    sSpawnAttempted = false;
    hasFloor = true;
    SpawnForestSaria();
    SpawnForestSaria();
    CHECK(spawns == 1 && spawned.id == ACTOR_EN_VIEWER && spawned.params == 0x7F13 && spawned.world.pos.y == 42);
    CHECK(std::memcmp(&before, &gSaveContext, sizeof(before)) == 0);

    // Seated scenery never grants; only the dedicated Termina Field gift does.
    Actor seated{};
    seated.params = 0x7F33;
    Actor gift{};
    gift.params = 0x7E33;
    CHECK(ComboSaria_IsParam(seated.params) && ComboSaria_IsParam(gift.params));
    play.sceneId = SCENE_00KEIKOKU;
    ComboSaria_Init(&seated, &play);
    CHECK(lastPose == 3);
    ComboSaria_Init(&gift, &play);
    CHECK(lastPose == 3);
    auto& sitting = *sStates.at(&gift);
    CHECK(sitting.collider.dim.height == 24 && sitting.collider.dim.radius == 12);
    ComboSaria_Update(&gift, &play);
    CHECK(sitting.joints[0].y == 900 && gift.focus.pos.y == gift.world.pos.y + 20);
    for (unsigned eye : { 0u, 1u, 2u }) {
        sitting.eye = eye;
        Gfx* head = nullptr;
        Override(&play, 16, &head, nullptr, nullptr, &gift);
        CHECK(reinterpret_cast<const char*>(head) == sitting.resources.heads[eye]);
    }
    auto talkAndClose = [&](Actor& actor) {
        textState = TEXT_STATE_DONE;
        talkAccepted = true;
        ComboSaria_Update(&actor, &play);
        CHECK(rewards == 0);
        textState = TEXT_STATE_NONE;
        ComboSaria_Update(&actor, &play);
    };
    talkAndClose(seated);
    CHECK(interactor.events.empty());
    play.sceneId = SCENE_LOST_WOODS;
    talkAndClose(gift);
    CHECK(interactor.events.empty());
    play.sceneId = SCENE_00KEIKOKU;
    gComboGameMode = COMBO_GAME_OOT;
    talkAndClose(gift);
    CHECK(interactor.events.empty());
    gComboGameMode = COMBO_GAME_BOTH;
    // Changing the reward NPC's pose keeps the same fixed, one-time song grant.
    for (s16 params : { 0x7E13, 0x7E23 }) {
        Actor posed{};
        posed.params = params;
        ComboSaria_Init(&posed, &play);
        CHECK(lastPose == ((params >> 4) & 15));
        talkAndClose(posed);
        CHECK(interactor.events.size() == 1 && rewards == 0);
        interactor.events.clear();
        const unsigned before = spawns;
        posed.id = ACTOR_EN_VIEWER;
        posed.world.pos = { 800, 100, -200 };
        play.actorCtx.actorLists[ACTORCAT_NPC].first = &posed;
        sSpawnAttempted = false;
        SpawnForestSaria();
        CHECK(sSpawnAttempted && spawns == before && posed.world.pos.x == 800);
        play.actorCtx.actorLists[ACTORCAT_NPC].first = nullptr;
        ComboSaria_Destroy(&posed, &play);
    }
    talkAndClose(gift);
    CHECK(interactor.events.size() == 1 && rewards == 0 && saves == 0);
    talkAndClose(gift);
    CHECK(interactor.events.size() == 1);
    // Event no longer depends on the NPC's lifetime. Queued/active duplication is blocked.
    auto event = std::get<GIEventGiveItem>(interactor.events.front());
    interactor.events.clear();
    interactor.currentEvent = event;
    talkAndClose(gift);
    CHECK(interactor.events.empty());
    ComboSaria_Destroy(&gift, &play);
    ComboSaria_Destroy(&seated, &play);
    Actor item{};
    item.home.rot.z = event.param;
    item.home.rot.x = CustomItem::GIVE_ITEM_CUTSCENE;
    ++gSaveContext.fileNum;
    event.giveItem(&item, &play);
    CHECK(rewards == 0);
    --gSaveContext.fileNum;
    event.giveItem(&item, &play);
    event.giveItem(&item, &play);
    CHECK(rewards == 1 && receipts == 1 && saves == 1 && hintRegistrations == 1);
    CHECK(gSaveContext.save.shipSaveInfo.nei.comboSariaSongClaimed == 1);
    CHECK(gSaveContext.save.shipSaveInfo.rando.sariaHintsAvailable == 1);
    // Consuming the hint/song or clearing cycle state never resets the permanent claim.
    gSaveContext.save.shipSaveInfo.rando.sariaHintsAvailable = 0;
    gSaveContext.save.shipSaveInfo.nei.ootQuestItems = 0;
    interactor.currentEvent = GIEventNone{};
    for (s16 params : { 0x7E13, 0x7E23, 0x7E33 }) {
        gift.params = params;
        ComboSaria_Init(&gift, &play);
        textState = TEXT_STATE_DONE;
        talkAccepted = true;
        ComboSaria_Update(&gift, &play);
        textState = TEXT_STATE_NONE;
        ComboSaria_Update(&gift, &play);
        CHECK(interactor.events.empty() && rewards == 1);
        ComboSaria_Destroy(&gift, &play);
    }
    sSpawnAttempted = false;
    SpawnForestSaria();
    CHECK(spawns == 2 && spawned.params == 0x7E33);
    // cor's Termina Field export: preserve the seated height and native
    // MM packed rotation (-30593 decodes to 272 degrees), without floor snap.
    CHECK(spawned.world.pos.x == 975 && spawned.world.pos.y == -99 && spawned.world.pos.z == 2101);
    CHECK(spawned.shape.rot.y == static_cast<s16>(0xC16C));
    sSpawnAttempted = false;
    authored.params = 0x7E33;
    play.actorCtx.actorLists[ACTORCAT_NPC].first = &authored;
    SpawnForestSaria();
    CHECK(spawns == 2 && sSpawnAttempted);
    // Actual translucent draw commands must contain hollow green hexes and
    // follow every authored pose, without relying on native star particles.
    const unsigned beforeSparkles = sparkles;
    for (auto params : { 0x7F13, 0x7F23, 0x7F33, 0x7E13, 0x7E23, 0x7E33 }) {
        Actor magical{};
        magical.params = params;
        magical.world.pos = { 500, 80, -400 };
        ComboSaria_Init(&magical, &play);
        auto draw = [&] {
            gfx.polyOpa.p = commands;
            gfx.polyOpa.d = reinterpret_cast<Gfx*>(hexVertices + 144);
            gfx.polyXlu.p = hexCommands;
            ComboSaria_Draw(&magical, &play);
        };
        draw();
        unsigned loads = 0, triangles = 0;
        for (Gfx* command = hexCommands; command != gfx.polyXlu.p; ++command) {
            loads += (command->words.w0 >> 24) == G_VTX;
            triangles += (command->words.w0 >> 24) == G_TRI2;
        }
        CHECK(loads == 12 && triangles == 72);
        CHECK(shimmerOrigin.x == 500 && shimmerOrigin.y == 80 && shimmerOrigin.z == -400);
        for (const auto& vertex : hexVertices) {
            CHECK(vertex.v.cn[1] > vertex.v.cn[0] && vertex.v.cn[1] > vertex.v.cn[2]);
            CHECK(std::abs(vertex.v.ob[0]) < 320 && std::abs(vertex.v.ob[2]) < 320);
            CHECK(vertex.v.ob[1] >= 0 && vertex.v.ob[1] < 530);
        }
        for (int h = 0; h < 12; ++h) {
            float center[3]{};
            for (int side = 0; side < 6; ++side)
                for (int axis = 0; axis < 3; ++axis)
                    center[axis] += hexVertices[h * 12 + side].v.ob[axis] / 6.0f;
            for (int side = 0; side < 6; ++side) {
                float outer = 0, inner = 0;
                for (int axis = 0; axis < 3; ++axis) {
                    const float a = hexVertices[h * 12 + side].v.ob[axis] - center[axis];
                    const float b = hexVertices[h * 12 + side + 6].v.ob[axis] - center[axis];
                    outer += a * a;
                    inner += b * b;
                }
                CHECK(inner > outer * 0.22f && inner < outer * 0.65f);
            }
        }
        if (((params >> 4) & 15) == 2) {
            Gfx* head = nullptr;
            Override(&play, 16, &head, nullptr, nullptr, &magical);
            CHECK(reinterpret_cast<const char*>(head) == sStates.at(&magical)->resources.heads[2]);
        }
        Vtx previous[144];
        std::memcpy(previous, hexVertices, sizeof(previous));
        ++play.gameplayFrames;
        draw();
        CHECK(std::memcmp(previous, hexVertices, sizeof(previous)) != 0);
        magical.world.pos = { -120, 12, 620 };
        draw();
        CHECK(shimmerOrigin.x == -120 && shimmerOrigin.y == 12 && shimmerOrigin.z == 620);
        play.pauseCtx.state = 1;
        draw();
        CHECK(gfx.polyXlu.p == hexCommands);
        play.pauseCtx.state = PAUSE_STATE_OFF;
        magical.xzDistToPlayer = 1000;
        draw();
        CHECK(gfx.polyXlu.p == hexCommands);
        ComboSaria_Destroy(&magical, &play);
    }
    CHECK(sparkles == beforeSparkles);
    const unsigned beforeHorse = spawns;
    sHorseSpawnAttempted = false;
    SpawnForestEpona();
    CHECK(spawns == beforeHorse && !sHorseSpawnAttempted);
    play.sceneId = SCENE_LOST_WOODS;
    play.actorCtx.actorLists[ACTORCAT_NPC].first = nullptr;
    gSaveContext.save.cutsceneIndex = 1;
    SpawnForestEpona();
    CHECK(spawns == beforeHorse && !sHorseSpawnAttempted);
    gSaveContext.save.cutsceneIndex = 0;
    gComboGameMode = COMBO_GAME_MM;
    SpawnForestEpona();
    CHECK(spawns == beforeHorse);
    gComboGameMode = COMBO_GAME_BOTH;
    play.roomCtx.status = 1;
    play.roomCtx.curRoom.segment = nullptr;
    SpawnForestEpona();
    CHECK(spawns == beforeHorse && !sHorseSpawnAttempted);
    play.roomCtx.status = 0;
    SpawnForestEpona();
    CHECK(spawns == beforeHorse && !sHorseSpawnAttempted);
    play.roomCtx.curRoom.segment = reinterpret_cast<void*>(1);
    gComboGameMode = COMBO_GAME_BOTH;
    Actor authoredHorse{};
    authoredHorse.id = ACTOR_EN_HORSE;
    authoredHorse.params = COMBO_FOREST_EPONA_PARAMS;
    play.actorCtx.actorLists[ACTORCAT_BG].first = &authoredHorse;
    SpawnForestEpona();
    CHECK(spawns == beforeHorse && sHorseSpawnAttempted);
    play.actorCtx.actorLists[ACTORCAT_BG].first = nullptr;
    sHorseSpawnAttempted = false;
    hasFloor = false;
    SpawnForestEpona();
    CHECK(spawns == beforeHorse && sHorseSpawnAttempted);
    hasFloor = true;
    sHorseSpawnAttempted = false;
    const Vec3f portal = ComboOpeningForest_ReturnAnchor(&play);
    const float portalAngle = ComboOpeningForest_ReturnYaw(&play) * (6.28318530718f / 65536);
    authored.params = 0x7F13;
    authored.world.pos = { portal.x + 100, portal.y, portal.z + 70 };
    play.actorCtx.actorLists[ACTORCAT_NPC].first = &authored;
    SaveContext beforeHorseSave = gSaveContext;
    SpawnForestEpona();
    SpawnForestEpona();
    CHECK(spawns == beforeHorse + 1 && spawned.id == ACTOR_EN_HORSE && spawned.params == COMBO_FOREST_EPONA_PARAMS);
    const float dx = spawned.world.pos.x - portal.x, dz = spawned.world.pos.z - portal.z;
    CHECK(std::fabs(dx * std::cos(portalAngle) - dz * std::sin(portalAngle) - 145) < .01);
    CHECK(std::fabs(dx * std::sin(portalAngle) + dz * std::cos(portalAngle) + 90) < .01);
    CHECK(spawned.world.pos.y == 42);
    // Moving Saria to the opposite flank flips the horse's automatic side.
    authored.world.pos = { portal.x + 130 * std::cos(portalAngle), portal.y, portal.z - 130 * std::sin(portalAngle) };
    sHorseSpawnAttempted = false;
    SpawnForestEpona();
    const float flipped = (spawned.world.pos.x - portal.x) * std::cos(portalAngle) -
                          (spawned.world.pos.z - portal.z) * std::sin(portalAngle);
    CHECK(flipped < -144 && std::memcmp(&beforeHorseSave, &gSaveContext, sizeof(beforeHorseSave)) == 0);
    // Solo MM has its own forest copy and owns Saria's asset snapshot locally.
    play.actorCtx.actorLists[ACTORCAT_NPC].first = nullptr;
    gComboGameMode = COMBO_GAME_MM;
    play.sceneId = COMBO_SOLO_FOREST_MM_SCENE;
    gSaveContext.save.entrance = COMBO_SOLO_FOREST_MM_ENTRANCE;
    unsigned soloBefore = spawns;
    sSpawnAttempted = sHorseSpawnAttempted = false;
    SpawnForestSaria();
    CHECK(spawns == soloBefore + 1 && spawned.params == 0x7F13);
    CHECK(sBridge.load == MM_PlacedSariaLoad && sBridge.release == MM_PlacedSariaRelease &&
          sBridge.alt == MM_PlacedSariaAlt);
    SpawnForestEpona();
    CHECK(spawns == soloBefore + 2 && spawned.params == COMBO_FOREST_EPONA_PARAMS);
    // The fixed song gift is also a native local-file check in solo MM.
    play.sceneId = SCENE_00KEIKOKU;
    gSaveContext.fileNum = 1;
    gSaveContext.save.shipSaveInfo.nei.comboSariaSongClaimed = 0;
    interactor.events.clear();
    interactor.currentEvent = GIEventNone{};
    ComboSaria_Init(&gift, &play);
    textState = TEXT_STATE_DONE;
    talkAccepted = true;
    ComboSaria_Update(&gift, &play);
    textState = TEXT_STATE_NONE;
    ComboSaria_Update(&gift, &play);
    CHECK(interactor.events.size() == 1);
    auto soloEvent = std::get<GIEventGiveItem>(interactor.events.front());
    item.home.rot.z = soloEvent.param;
    soloEvent.giveItem(&item, &play);
    soloEvent.giveItem(&item, &play);
    CHECK(rewards == 2 && saves == 2 && hintRegistrations == 2 &&
          gSaveContext.save.shipSaveInfo.nei.comboSariaSongClaimed);
    ComboSaria_Destroy(&gift, &play);
    puts("PASS solo MM forest and song routing: own exploration copy, local asset bridge, portal horse, fixed gift "
         "once per native file");
    puts("PASS MM Saria lifecycle: missing donor, independent blink, fixed placement/yaw, collider/talk, Alt "
         "rebind/frame preservation, failure cleanup and exactly-once release");
    puts("PASS Saria spawn guards: combined exploration/room zero only; cutscene, solo MM, pause, inactive play and "
         "missing floor excluded; authored placement wins; single spawn; no save writes");
    puts("PASS hands-behind-back default and custom waiting blink: open, half, closed, open head selection across the "
         "full native blink cycle");
    puts("PASS seated fixed song gift: grounded pose, custom blinking, talk/close, deferred existing-item delivery, "
         "queued/active duplicate suppression, file/mode/scene guards, post-consumption permanent claim, authored "
         "field placement");
    puts("PASS all Saria poses and song variants: green hollow-hex draw commands, follow authored placements, "
         "pause/distance guards");
    puts("PASS portal Epona placement: opposite Saria in portal coordinates, clear approach, floor probe, authored "
         "override, no duplicate or prologue/solo/other-scene spawn");
}
