#ifdef COMBO_BUILD
#include "ComboRegionalPassages.h"
#include "ComboGameMode.h"
#include "ComboResolve.h"
#include "entrances/RegionalPassages.h"
#include "gameplay/CrossGameTitleCard.h"
#include "soh/ActorDB.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/randomizer/SeedContext.h"
#include "soh/SaveManager.h"
#include "soh/ShipInit.hpp"
#include "overlays/actors/ovl_Bg_Haka/z_bg_haka.h"
#include "overlays/actors/ovl_Door_Ana/z_door_ana.h"
#include <array>
#include <cmath>
extern "C" {
void BgHaka_Init(Actor*, PlayState*);
void BgHaka_Destroy(Actor*, PlayState*);
void BgHaka_Draw(Actor*, PlayState*);
void BgHaka_CheckPlayerOnDirtPatch(BgHaka*, Player*);
void DoorAna_Init(Actor*, PlayState*);
void DoorAna_Destroy(Actor*, PlayState*);
void DoorAna_Draw(Actor*, PlayState*);
extern void (*gComboSceneSwitchCallback)(int);
}
namespace {
using namespace ComboRegional;
struct RegionalGrave {
    BgHaka native;
    bool pulling;
};
struct Passage {
    PlayState* play = nullptr;
    Pose entry{}, mouth{}, gravePose{};
    int route = 0, room = -1, slot = -1;
    u32 seed = 0;
    bool queried = false, ready = false, falling = false;
    RegionalGrave* grave = nullptr;
    Actor* hole = nullptr;
} sRegional;
PlayState* sEntrancePlay = nullptr;
unsigned sEntranceCount = 0;
std::array<Origin, 3> sOrigins{};
std::array<int, 3> sOriginAge{ { -1, -1, -1 } };
std::array<bool, 3> sOriginMq{};
int sOriginSlot = -1, sArrivalRoute = 0;
u32 sOriginSeed = 0;
bool sLoadPending = true, sReturnStaged = false, sReturningGrave = false;
RespawnData sPreservedReturn{};
PlayState* sArrivalCoverPlay = nullptr;
void ClearArrivalCover() {
    if (sArrivalCoverPlay && sArrivalCoverPlay == gPlayState)
        sArrivalCoverPlay->envCtx.fillScreen = false;
    sArrivalCoverPlay = nullptr;
}
nlohmann::json sPending;
int sGraveId = -1, sHoleId = -1;
u32 Seed() {
    return IS_RANDO ? Rando::Context::GetInstance()->GetSeed() : 0;
}
int SceneRoute(int scene) {
    if (scene == SCENE_GRAVEYARD)
        return COMBO_PORTAL_IKANA;
    if (scene == SCENE_LAKE_HYLIA)
        return COMBO_PORTAL_GREAT_BAY;
    if (scene == SCENE_ICE_CAVERN)
        return COMBO_PORTAL_SNOWHEAD;
    return 0;
}
bool Bound() {
    return Combo_IsCombinedGame() && gSaveContext.gameMode == GAMEMODE_NORMAL &&
           ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) && sOriginSlot == gSaveContext.fileNum &&
           sOriginSeed == Seed();
}
bool Valid(const Origin& origin) {
    return origin.entrance >= 0 && origin.entrance < ENTR_MAX &&
           ValidOrigin(origin, gEntranceTable[origin.entrance].scene);
}
bool SavedSetup(int index, int scene) {
    return Valid(sOrigins[index]) && sOrigins[index].room == sRegional.room &&
           sOriginAge[index] == gSaveContext.linkAge &&
           sOriginMq[index] == static_cast<bool>(ResourceMgr_IsSceneMasterQuest(scene));
}
struct NativeWorld {
    PlayState* play;
    Actor* ignored = nullptr;
    bool Floor(Point p, float expected, float* height) {
        Vec3f point{ p.x, expected + 100, p.z };
        CollisionPoly* floor = nullptr;
        s32 bg = BGCHECK_SCENE;
        const float y = BgCheck_EntityRaycastFloor5(play, &play->colCtx, &floor, &bg, &GET_PLAYER(play)->actor, &point);
        if (!floor || bg != BGCHECK_SCENE || !std::isfinite(y) || y == BGCHECK_Y_MIN || std::fabs(y - expected) > 35 ||
            floor->normal.y < 23170 || SurfaceType_GetSceneExitIndex(&play->colCtx, floor, bg) != 0)
            return false;
        WaterBox* water = nullptr;
        float waterY;
        if (WaterBox_GetSurface1(play, &play->colCtx, p.x, p.z, &waterY, &water) && waterY >= y - .5f)
            return false;
        *height = y;
        return true;
    }
    bool Wall(Point a, Point b, Point* hit, Point* normal) {
        Vec3f from{ a.x, a.y, a.z }, to{ b.x, b.y, b.z }, found{};
        CollisionPoly* wall = nullptr;
        s32 bg = BGCHECK_SCENE;
        if (!BgCheck_EntityLineTest1(&play->colCtx, &from, &to, &found, &wall, true, false, false, true, &bg) ||
            !wall || bg != BGCHECK_SCENE || std::abs(wall->normal.y) > 4096)
            return false;
        *hit = { found.x, found.y, found.z };
        *normal = { wall->normal.x / 32767.f, 0, wall->normal.z / 32767.f };
        return true;
    }
    bool Clear(Point a, Point b) {
        Vec3f from{ a.x, a.y, a.z }, to{ b.x, b.y, b.z }, found{};
        CollisionPoly* polygon = nullptr;
        s32 bg = BGCHECK_SCENE;
        // A hit of any kind obstructs the apron, including dynamic geometry,
        // floors/ceilings and backfaces. Only Wall() filters a support wall.
        return !BgCheck_EntityLineTest1(&play->colCtx, &from, &to, &found, &polygon, true, true, true, false, &bg);
    }
    bool NearWater(Point p) {
        for (float radius : { 120.f, 280.f, 500.f })
            for (int step = 0; step < 8; ++step) {
                const auto point = Add(p, Forward(static_cast<short>(step * 8192)), radius);
                WaterBox* box = nullptr;
                float y;
                if (WaterBox_GetSurface1(play, &play->colCtx, point.x, point.z, &y, &box) && std::isfinite(y))
                    return true;
            }
        return false;
    }
    bool Occupied(Point p, float radius) {
        for (int category = 0; category < ACTORCAT_MAX; ++category) {
            if (category == ACTORCAT_PLAYER || category == ACTORCAT_MISC || category == ACTORCAT_EXPLOSIVE)
                continue;
            for (Actor* a = play->actorCtx.actorLists[category].head; a; a = a->next) {
                if (a == ignored || !a->update || (a->room >= 0 && a->room != play->roomCtx.curRoom.num) ||
                    std::fabs(a->home.pos.y - p.y) > 180)
                    continue;
                if (Distance2({ a->home.pos.x, a->home.pos.y, a->home.pos.z }, p) < radius * radius)
                    return true;
            }
        }
        return false;
    }
};
bool FindCave(PlayState* play) {
    NativeWorld world{ play };
    const int index = sRegional.route - COMBO_PORTAL_IKANA;
    if (SavedSetup(index, play->sceneNum)) {
        const auto& saved = sOrigins[index].pose;
        const auto front = Forward(saved.yaw);
        const auto support = Add(saved.pos, front, -102);
        if (ValidateMouth(world, support, front, &sRegional.mouth) &&
            Reachable(world, sRegional.entry, sRegional.mouth) &&
            (sRegional.route != COMBO_PORTAL_GREAT_BAY || world.NearWater(support)))
            return true;
    }
    return FindMouth(world, sRegional.entry, &sRegional.mouth, sRegional.route == COMBO_PORTAL_GREAT_BAY);
}
void SetPulled(RegionalGrave* grave) {
    auto& native = grave->native;
    native.state = 1;
    native.dyna.actor.minVelocityY = 60;
    native.dyna.actor.world.rot.y = native.dyna.actor.shape.rot.y + 0x8000;
    native.dyna.actor.world.pos.x = native.dyna.actor.home.pos.x + Math_SinS(native.dyna.actor.world.rot.y) * 60;
    native.dyna.actor.world.pos.z = native.dyna.actor.home.pos.z + Math_CosS(native.dyna.actor.world.rot.y) * 60;
}
void GraveInit(Actor* actor, PlayState* play) {
    BgHaka_Init(actor, play);
    auto* grave = reinterpret_cast<RegionalGrave*>(actor);
    grave->pulling = false;
    if (sReturningGrave) {
        SetPulled(grave);
        sReturningGrave = false;
    }
}
void GraveUpdate(Actor* actor, PlayState* play) {
    auto* grave = reinterpret_cast<RegionalGrave*>(actor);
    auto& native = grave->native;
    Player* player = GET_PLAYER(play);
    if (!player)
        return;
    if (native.state) {
        // Match native IdleOpened: another grave/dynapoly owns the player's
        // moving flag unless this specific grave has an outstanding force.
        if (native.dyna.unk_150 != 0) {
            native.dyna.unk_150 = 0;
            player->stateFlags2 &= ~PLAYER_STATE2_MOVING_DYNAPOLY;
        }
        return;
    }
    // The added grave shares the native pull collision/kinematics and dirt
    // sounds. Its completion owns no vanilla switch, Poe, chest or grave check.
    if (!grave->pulling && native.dyna.unk_150 != 0) {
        if (native.dyna.unk_150 > 0) {
            native.dyna.unk_150 = 0;
            player->stateFlags2 &= ~PLAYER_STATE2_MOVING_DYNAPOLY;
        } else {
            grave->pulling = true;
            actor->world.rot.y = actor->shape.rot.y + 0x8000;
        }
    }
    if (grave->pulling) {
        actor->speedXZ = std::fmin(actor->speedXZ + .05f, 1.5f);
        const bool complete = Math_StepToF(&actor->minVelocityY, 60, actor->speedXZ);
        actor->world.pos.x = actor->home.pos.x + Math_SinS(actor->world.rot.y) * actor->minVelocityY;
        actor->world.pos.z = actor->home.pos.z + Math_CosS(actor->world.rot.y) * actor->minVelocityY;
        Actor_PlaySfx_Flagged(actor, NA_SE_EV_ROCK_SLIDE - SFX_FLAG);
        if (complete) {
            native.state = 1;
            native.dyna.unk_150 = 0;
            grave->pulling = false;
            player->stateFlags2 &= ~PLAYER_STATE2_MOVING_DYNAPOLY;
            Sfx_PlaySfxCentered(NA_SE_SY_CORRECT_CHIME);
        }
    }
    BgHaka_CheckPlayerOnDirtPatch(&native, player);
}
void HoleInit(Actor* actor, PlayState* play) {
    DoorAna_Init(actor, play);
    Actor_SetScale(actor, 0);
}
void HoleUpdate(Actor* actor, PlayState* play) {
    if (!sRegional.grave || !sRegional.grave->native.state)
        return;
    const bool open = Math_StepToF(&actor->scale.x, .01f, .001f);
    Actor_SetScale(actor, actor->scale.x);
    if (!open)
        return;
    auto* player = GET_PLAYER(play);
    if (!Bound() || !player || !gComboSceneSwitchCallback || !Combo_ResolveSym("2ship", "MM_SetComboPortalRoute") ||
        play->pauseCtx.state != 0 || play->transitionMode != TRANS_MODE_OFF ||
        play->transitionTrigger != TRANS_TRIGGER_OFF || Player_InCsMode(play) ||
        player->stateFlags1 & (PLAYER_STATE1_ON_HORSE | PLAYER_STATE1_IN_WATER))
        return;
    const auto& p = player->actor.world.pos;
    if (Distance2({ p.x, p.y, p.z }, sRegional.mouth.pos) < 15 * 15 && p.y >= sRegional.mouth.pos.y - 50 &&
        p.y <= sRegional.mouth.pos.y + 15) {
        player->stateFlags1 |= PLAYER_STATE1_FLOOR_DISABLED;
        sRegional.falling = true;
    }
}
void RegisterActors() {
    if (sGraveId >= 0)
        return;
    ActorDBInit grave;
    grave.name = "Bg_ComboRegionalGrave";
    grave.desc = "Additional Ikana passage grave";
    grave.category = ACTORCAT_BG;
    grave.objectId = OBJECT_HAKA;
    grave.instanceSize = sizeof(RegionalGrave);
    grave.flags = ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    grave.init = GraveInit;
    grave.destroy = BgHaka_Destroy;
    grave.update = GraveUpdate;
    grave.draw = BgHaka_Draw;
    sGraveId = ActorDB::Instance->AddEntry(grave).entry.id;
    ActorDBInit hole;
    hole.name = "Door_ComboRegionalGrave";
    hole.desc = "Native Ikana grave aperture";
    hole.category = ACTORCAT_ITEMACTION;
    hole.objectId = OBJECT_GAMEPLAY_FIELD_KEEP;
    hole.instanceSize = sizeof(DoorAna);
    hole.flags = ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    hole.init = HoleInit;
    hole.destroy = DoorAna_Destroy;
    hole.update = HoleUpdate;
    hole.draw = DoorAna_Draw;
    sHoleId = ActorDB::Instance->AddEntry(hole).entry.id;
}
bool FindGrave(PlayState* play) {
    NativeWorld world{ play };
    // Copy the relative grave/hole arrangement from this setup's own native
    // actors. Existing grave locations, params, collision and exits stay intact.
    const bool preferred = SavedSetup(0, play->sceneNum);
    for (int pass = preferred ? 0 : 1; pass < 2; ++pass) {
        for (Actor* actor = play->actorCtx.actorLists[ACTORCAT_BG].head; actor; actor = actor->next) {
            if (actor->id != ACTOR_BG_HAKA || actor->room != sRegional.room)
                continue;
            for (Actor* hole = play->actorCtx.actorLists[ACTORCAT_ITEMACTION].head; hole; hole = hole->next) {
                if (hole->id != ACTOR_DOOR_ANA || hole->room != sRegional.room)
                    continue;
                const Point base{ actor->home.pos.x, actor->home.pos.y, actor->home.pos.z };
                const Point nativeHole{ hole->home.pos.x, hole->home.pos.y, hole->home.pos.z };
                if (Distance2(base, nativeHole) > 120 * 120 || std::fabs(base.y - nativeHole.y) > 10)
                    continue;
                const Point front = Forward(actor->shape.rot.y), right{ front.z, 0, -front.x };
                for (float offset : { 150.f, -150.f, 300.f, -300.f, 450.f, -450.f }) {
                    const Point grave = Add(base, right, offset), aperture = Add(nativeHole, right, offset);
                    if (pass == 0 && Distance2(aperture, sOrigins[0].pose.pos) > 1)
                        continue;
                    if (world.Occupied(grave, 95) || world.Occupied(aperture, 75))
                        continue;
                    bool safe = true;
                    for (float width : { -42.f, 0.f, 42.f })
                        for (float depth : { -100.f, -60.f, 0.f, 60.f, 100.f }) {
                            const Point point = Add(Add(grave, right, width), front, depth);
                            float y;
                            if (!world.Floor(point, base.y, &y) || std::fabs(y - base.y) > 8) {
                                safe = false;
                                continue;
                            }
                            Point bottom = point, top = point;
                            bottom.y = y + 1;
                            top.y = y + 180;
                            if (!world.Clear(bottom, top))
                                safe = false;
                        }
                    for (float width : { -42.f, 0.f, 42.f })
                        for (float height : { 20.f, 80.f, 140.f }) {
                            Point a = Add(Add(grave, right, width), front, -100), b = Add(a, front, 200), hit{},
                                  normal{};
                            a.y = b.y = base.y + height;
                            if (!world.Clear(a, b))
                                safe = false;
                        }
                    const Pose nativeApproach{ Add(nativeHole, Forward(hole->home.rot.y), 100), hole->home.rot.y };
                    const Pose addedMouth{ aperture, hole->home.rot.y, Forward(hole->home.rot.y) };
                    if (!safe || !Reachable(world, nativeApproach, addedMouth))
                        continue;
                    RegisterActors();
                    auto* added = Actor_Spawn(&play->actorCtx, play, sGraveId, grave.x, grave.y, grave.z, 0,
                                              actor->shape.rot.y, 0, 1);
                    if (!added)
                        return false;
                    auto* addedHole = Actor_Spawn(&play->actorCtx, play, sHoleId, aperture.x, aperture.y, aperture.z, 0,
                                                  hole->home.rot.y, 0, 0);
                    if (!addedHole) {
                        Actor_Kill(added);
                        return false;
                    }
                    sRegional.grave = reinterpret_cast<RegionalGrave*>(added);
                    sRegional.hole = addedHole;
                    sRegional.gravePose = { grave, actor->shape.rot.y, front };
                    sRegional.mouth = { aperture, hole->home.rot.y, Forward(hole->home.rot.y) };
                    return true;
                }
            }
        }
    }
    return false;
}
void CaptureOrigin() {
    if (!Bound() || !sRegional.ready)
        return;
    SaveManager::Instance->ThreadPoolWait();
    Pose arrival = sRegional.mouth;
    if (sRegional.route != COMBO_PORTAL_IKANA)
        arrival.pos = Add(arrival.pos, arrival.forward, 100);
    const auto index = sRegional.route - COMBO_PORTAL_IKANA;
    sOrigins[index] = { sRegional.route, gSaveContext.entranceIndex, sRegional.room, arrival };
    sOriginAge[index] = gSaveContext.linkAge;
    sOriginMq[index] = ResourceMgr_IsSceneMasterQuest(sRegional.play->sceneNum);
}
void StageArrival(const Origin& origin) {
    sPreservedReturn = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    sReturnStaged = true;
    auto& arrival = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    arrival = {};
    arrival.entranceIndex = origin.entrance;
    arrival.roomIndex = origin.room;
    arrival.pos = { origin.pose.pos.x, origin.pose.pos.y, origin.pose.pos.z };
    arrival.yaw = origin.pose.yaw;
    arrival.playerParams = origin.route == COMBO_PORTAL_IKANA ? 0x04FF : 0x0EFF;
    arrival.data = 1;
    gSaveContext.entranceIndex = origin.entrance;
    gSaveContext.respawnFlag = 2;
    gSaveContext.cutsceneIndex = 0;
    gSaveContext.cutsceneTrigger = 0;
    sReturningGrave = origin.route == COMBO_PORTAL_IKANA;
}
void PlayerUpdate() {
    auto* play = gPlayState;
    if (!Bound() || !play) {
        ClearArrivalCover();
        sArrivalRoute = 0;
        return;
    }
    if (sReturnStaged) {
        gSaveContext.respawn[RESPAWN_MODE_RETURN] = sPreservedReturn;
        sReturnStaged = false;
    }
    if (!sArrivalRoute || !play->state.running)
        return;
    if (!ComboRegionalPassagesOot_Get(play, nullptr, nullptr, nullptr) || sRegional.route != sArrivalRoute) {
        // An unsupported scene/setup reveals its safe native approach instead
        // of trusting an old point or keeping the screen covered indefinitely.
        ClearArrivalCover();
        sArrivalRoute = 0;
        sReturningGrave = false;
        return;
    }
    sArrivalCoverPlay = play;
    play->envCtx.fillScreen = true;
    play->envCtx.screenFillColor[0] = play->envCtx.screenFillColor[1] = play->envCtx.screenFillColor[2] = 0;
    play->envCtx.screenFillColor[3] = 255;
    if (play->transitionTrigger != TRANS_TRIGGER_OFF || play->transitionMode != TRANS_MODE_OFF ||
        play->pauseCtx.state != 0 || Player_InCsMode(play))
        return;
    // First visits and changed age/layout start at a native entrance under
    // cover. Resolve its loaded floor, then let Player_Init consume the real
    // native return start mode and reveal the matching passage once.
    CaptureOrigin();
    const auto origin = sOrigins[sArrivalRoute - COMBO_PORTAL_IKANA];
    sArrivalRoute = 0;
    if (!Valid(origin)) {
        ClearArrivalCover();
        return;
    }
    StageArrival(origin);
    play->nextEntranceIndex = origin.entrance;
    play->transitionType = gSaveContext.nextTransitionType = TRANS_TYPE_FADE_BLACK;
    play->transitionTrigger = TRANS_TRIGGER_START;
    Combo_SOH_RequestCrossGameTitleCard();
}
void BindFile(int slot) {
    if (!sLoadPending && sOriginSlot == slot && sOriginSeed == Seed())
        return;
    SaveManager::Instance->ThreadPoolWait();
    sLoadPending = false;
    sOriginSlot = slot;
    sOriginSeed = Seed();
    sOrigins = {};
    sOriginAge = { { -1, -1, -1 } };
    sOriginMq = {};
    sArrivalRoute = 0;
    sReturnStaged = false;
    sReturningGrave = false;
    if (!Bound() || !sPending.is_object())
        return;
    try {
        if (sPending.at("version") != 1 || sPending.at("seed") != sOriginSeed || sPending.at("slot") != slot)
            return;
        const auto& records = sPending.at("origins");
        if (!records.is_array() || records.size() != 3)
            return;
        for (size_t i = 0; i < 3; ++i) {
            const auto& p = records[i];
            if (p.is_null())
                continue;
            const int yaw = p.at("yaw");
            const Origin origin{ static_cast<int>(COMBO_PORTAL_IKANA + i),
                                 p.at("entrance"),
                                 p.at("room"),
                                 { { p.at("x"), p.at("y"), p.at("z") }, static_cast<short>(yaw) } };
            const int age = p.at("age");
            const bool mq = p.at("mq");
            if (yaw >= INT16_MIN && yaw <= INT16_MAX && (age == 0 || age == 1) && Valid(origin)) {
                sOrigins[i] = origin;
                sOriginAge[i] = age;
                sOriginMq[i] = mq;
            }
        }
    } catch (const nlohmann::json::exception&) { sOrigins = {}; }
}
void SaveOrigins(const SaveContext& snapshot, int, bool) {
    if (!Combo_IsCombinedGame() || snapshot.fileNum != sOriginSlot)
        return;
    nlohmann::json origins = nlohmann::json::array();
    for (size_t i = 0; i < sOrigins.size(); ++i) {
        const auto& origin = sOrigins[i];
        if (!Valid(origin)) {
            origins.push_back(nullptr);
            continue;
        }
        origins.push_back({ { "entrance", origin.entrance },
                            { "room", origin.room },
                            { "x", origin.pose.pos.x },
                            { "y", origin.pose.pos.y },
                            { "z", origin.pose.pos.z },
                            { "yaw", origin.pose.yaw },
                            { "age", sOriginAge[i] },
                            { "mq", sOriginMq[i] } });
    }
    const nlohmann::json value = {
        { "version", 1 }, { "slot", sOriginSlot }, { "seed", sOriginSeed }, { "origins", origins }
    };
    SaveManager::Instance->SaveData("native", value);
}
void Register() {
    SaveManager::Instance->AddInitFunction([](bool) {
        sPending = nullptr;
        sLoadPending = true;
    });
    SaveManager::Instance->AddLoadFunction("regionalPassages", 1, [] {
        SaveManager::Instance->LoadData("native", sPending);
        sLoadPending = true;
    });
    SaveManager::Instance->AddSaveFunction("regionalPassages", 1, SaveOrigins, true, SECTION_PARENT_NONE);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>(BindFile);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(PlayerUpdate);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>(ComboRegionalPassagesOot_Reset);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnExitGame>([](int32_t) {
        sArrivalRoute = 0;
        ComboRegionalPassagesOot_Reset();
    });
}
RegisterShipInitFunc regionalPassagesInit(Register);
} // namespace
extern "C" void ComboRegionalPassagesOot_CancelDeparture(void) {
    if (sRegional.falling && sRegional.play && sRegional.play == gPlayState && GET_PLAYER(gPlayState)) {
        auto* player = GET_PLAYER(gPlayState);
        player->stateFlags1 &= ~PLAYER_STATE1_FLOOR_DISABLED;
        const auto safe = Add(sRegional.mouth.pos, sRegional.mouth.forward, 80);
        player->actor.world.pos = { safe.x, safe.y, safe.z };
    }
    sRegional.falling = false;
}
extern "C" void ComboRegionalPassagesOot_Reset(void) {
    ComboRegionalPassagesOot_CancelDeparture();
    ClearArrivalCover();
    sRegional = {};
}
extern "C" void ComboRegionalPassagesOot_ReadEntrances(PlayState* play, unsigned count) {
    sEntrancePlay = play;
    sEntranceCount = count;
}
extern "C" void ComboRegionalPassagesOot_ReadSpawns(PlayState* play, const ActorEntry* entries, unsigned count) {
    sRegional = {};
    // Farore is composed after the portal boot. Observe its native TOP start
    // mode before Player_Init consumes it, including recalls into this region.
    if (gSaveContext.respawnFlag == 3) {
        sArrivalRoute = 0;
        sReturningGrave = false;
        if (sReturnStaged)
            gSaveContext.respawn[RESPAWN_MODE_RETURN] = sPreservedReturn;
        sReturnStaged = false;
    }
    if (!Combo_IsCombinedGame() || !play || !SceneRoute(play->sceneNum) || !entries || !play->setupEntranceList ||
        !count)
        return;
    const unsigned spawn = play->sceneNum == SCENE_LAKE_HYLIA ? 4 : 0;
    if (sEntrancePlay != play || spawn >= sEntranceCount)
        return;
    const auto native = play->setupEntranceList[spawn];
    if (native.spawn >= count)
        return;
    const auto& source = entries[native.spawn];
    sRegional.play = play;
    sRegional.route = SceneRoute(play->sceneNum);
    sRegional.room = native.room;
    sRegional.entry = { { static_cast<float>(source.pos.x), static_cast<float>(source.pos.y),
                          static_cast<float>(source.pos.z) },
                        source.rot.y };
    sRegional.slot = gSaveContext.fileNum;
    sRegional.seed = Seed();
}
extern "C" int ComboRegionalPassagesOot_Get(PlayState* play, int* route, Vec3f* anchor, s16* yaw) {
    if (!Bound() || !play || play != sRegional.play || SceneRoute(play->sceneNum) != sRegional.route ||
        play->roomCtx.curRoom.num != sRegional.room || sRegional.slot != gSaveContext.fileNum ||
        sRegional.seed != Seed() || !play->colCtx.colHeader || !GET_PLAYER(play))
        return false;
    if (!sRegional.queried) {
        sRegional.queried = true;
        if (sRegional.route == COMBO_PORTAL_IKANA)
            sRegional.ready = FindGrave(play);
        else
            sRegional.ready = FindCave(play);
    }
    if (!sRegional.ready)
        return false;
    if (route)
        *route = sRegional.route;
    if (anchor)
        *anchor = { sRegional.mouth.pos.x, sRegional.mouth.pos.y, sRegional.mouth.pos.z };
    if (yaw)
        *yaw = sRegional.mouth.yaw;
    return true;
}
extern "C" int ComboRegionalPassagesOot_OwnsGraveFall(PlayState* play) {
    return sRegional.falling && ComboRegionalPassagesOot_Get(play, nullptr, nullptr, nullptr) &&
           sRegional.route == COMBO_PORTAL_IKANA;
}
extern "C" int ComboRegionalPassagesOot_AtDeparture(PlayState* play, const Vec3f* position) {
    if (!position || !ComboRegionalPassagesOot_Get(play, nullptr, nullptr, nullptr))
        return 0;
    const Point point{ position->x, position->y, position->z };
    const bool entering = sRegional.route == COMBO_PORTAL_IKANA
                              ? sRegional.falling && point.y < sRegional.mouth.pos.y - 8
                              : InMouth(sRegional.mouth, point);
    if (!entering)
        return 0;
    CaptureOrigin();
    return sRegional.route;
}
extern "C" int ComboRegionalPassagesOot_ApplyArrival(int route) {
    if (!Bound() || !ComboRegionalPassages_IsRoute(route))
        return 0;
    // Saved coordinates select a preferred mouth only after its current native
    // floor/wall have loaded and passed validation. The safe approach stays
    // covered until a native RETURN/GROTTO start can reveal that passage.
    sArrivalRoute = route;
    gSaveContext.entranceIndex = route == COMBO_PORTAL_IKANA       ? ENTR_GRAVEYARD_ENTRANCE
                                 : route == COMBO_PORTAL_GREAT_BAY ? ENTR_LAKE_HYLIA_OUTSIDE_LAB
                                                                   : ENTR_ICE_CAVERN_ENTRANCE;
    gSaveContext.respawnFlag = 0;
    gSaveContext.cutsceneIndex = 0;
    gSaveContext.cutsceneTrigger = 0;
    sReturningGrave = route == COMBO_PORTAL_IKANA;
    gSaveContext.nextTransitionType = TRANS_TYPE_FADE_BLACK;
    return 1;
}
#endif
