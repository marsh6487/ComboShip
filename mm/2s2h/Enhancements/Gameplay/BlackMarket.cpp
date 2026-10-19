#ifdef COMBO_BUILD
#include "BlackMarket.h"
#include "BlackMarketItems.h"
#include "BlackMarketResources.h"
#include "ComboExport.h"
#include "ComboOpeningForest.h"
#include "entrances/BlackMarketEntrance.h"
#include "entrances/SoloForest.h"
#include "BlackMarketLock.h"
#include "BlackMarketKeeper.h"
#include "BlackMarketGrannyResources.h"
#include "BlackMarketBrowsing.h"
#include "2s2h/BenPort.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#include "2s2h/Enhancements/Graphics/ComboPlaceTitleCard.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/Rando/Rando.h"
#include "2s2h/SaveManager/SaveManager.h"
#include "2s2h/ShipInit.hpp"
#include <array>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <libultraship/bridge/resourcebridge.h>
#include <libultraship/bridge/consolevariablebridge.h>
int SaveManager_ReadSaveFile(const std::filesystem::path&, nlohmann::json&);
extern "C" {
#include "overlays/actors/ovl_En_Takaraya/z_en_takaraya.h"
extern SceneEntranceTableEntry sSceneEntranceTable[ENTR_SCENE_MAX];
u8 ResourceMgr_FileExists(const char* path);
void Player_Action_35(Player* player, PlayState* play);
}
#include "BlackMarketShelvesNative.h"

namespace {
using namespace ComboRando::BlackMarket;
Session sSession;
uint16_t sClears = 0;
void (*sReportClear)(int, int) = nullptr;
bool sEntering = false, sReturnPending = false, sDoorArmed = false, sTitlePending = false;
bool sTalking = false;
bool sResumeFromMarket = false;
int sTravelSlot = -1;
u32 sTravelSeed = 0;
int sSelected = 0;
float sDoorFloor = 0;
std::string sStatus;
constexpr u16 kBrowseText = 0x8F10, kResultText = 0x8F11, kWelcomeText = 0x8F12, kConfirmText = 0x8F13;
constexpr char kScene[] = "scenes/nonmq/skiejer_market/skiejer_market";
constexpr char kRoom[] = "scenes/nonmq/skiejer_market/skiejer_market_room_0";
constexpr char kKeeper[] = "__OTR__custom/skiejer_market/keeper/gTreasureChestShopGalSkel";
alignas(2) constexpr char kCard[] = "__OTR__custom/skiejer_market/title";
alignas(2) constexpr char kExterior[] = "__OTR__custom/skiejer_market/entrance";
constexpr Vec3f kShelves[8] = {
    // Two framed shelf banks; keep all offers on one frontal plane.
    // Upper anchors clear the physical middle board and selected-item seals.
    { 35, 58, 110 },  { 35, 110, 110 },  { 70, 58, 110 },  { 70, 110, 110 },
    { -35, 58, 110 }, { -35, 110, 110 }, { -70, 58, 110 }, { -70, 110, 110 }
};
std::array<Actor, 8> sDisplays;
BlackMarketShelvesNative::Fixture sShelfFixture;
Vec3f sSoloReturn = { 184, 0, -300 };
s16 sSoloReturnYaw = -16384;
u16 sSoloReturnEntrance = COMBO_SOLO_FOREST_MM_ENTRANCE;
RespawnData sPreservedReturn;
bool sReturnStaged = false;

bool SoloExterior(const PlayState* play) {
    return play && play->roomCtx.curRoom.num == 0 &&
           ComboSoloForest_IsMmVisit(play->sceneId, play->roomCtx.curRoom.num, gSaveContext.gameMode,
                                     gSaveContext.fileNum, gSaveContext.sceneLayer, gSaveContext.save.cutsceneIndex);
}
bool Exterior(const PlayState* play) {
    return (ComboOpeningForest_IsExploration(play) && play->roomCtx.curRoom.num == 0) || SoloExterior(play);
}
Vec3f Door() {
    return { COMBO_BLACK_MARKET_DOOR_X, 0, COMBO_BLACK_MARKET_DOOR_Z };
}
u16 ReturnEntrance() {
    return gComboGameMode == COMBO_GAME_MM ? sSoloReturnEntrance : ENTRANCE(LOST_WOODS, 0);
}
void StageSoloReturn() {
    if (gComboGameMode != COMBO_GAME_MM || sReturnStaged)
        return;
    // Local forest arrivals require RETURN's position fields. Preserve
    // that disposable point across this one load; TOP/Farore's Wind is untouched.
    auto& arrival = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    sPreservedReturn = arrival;
    sReturnStaged = true;
    arrival = {};
    arrival.entrance = sSoloReturnEntrance;
    arrival.roomIndex = 0;
    arrival.pos = sSoloReturn;
    arrival.yaw = sSoloReturnYaw;
    arrival.playerParams = PLAYER_PARAMS(0xFF, PLAYER_START_MODE_E);
    arrival.data = 1;
    gSaveContext.respawnFlag = 2;
}

bool PackAvailable() {
    // The manifest is last in the builder's dependency-closed candidate. No
    // doorway is enabled by an unrelated texture or a partial donor archive.
    return std::all_of(std::begin(kBlackMarketResources), std::end(kBlackMarketResources),
                       [](const char* path) { return ResourceMgr_FileExists(path); });
}
bool Interior() {
    return gComboGameMode != COMBO_GAME_OOT && gPlayState && gPlayState->sceneId == COMBO_BLACK_MARKET_SCENE;
}
bool Active() {
    return Interior() && sSession.Active(gSaveContext.fileNum);
}

BlackMarketGranny::Resources sGrannyResources;
bool sGrannyPending = false, sGrannyActive = false;
RoomShape sGrannyShape{};
RoomShapeDListsEntry sGrannyEntry{};
std::vector<EnvLightSettings> sGrannyLights;
void GrannySceneBegin() {
    sGrannyActive = false;
    sGrannyPending = Interior() && CVarGetInteger(BlackMarketGranny::kCvar, 0) != 0;
}
void ApplyGrannyInterior() {
    auto* play = gPlayState;
    if (!sGrannyPending || !Interior() || play->roomCtx.curRoom.num != 0 || !play->roomCtx.curRoom.roomShape)
        return;
    sGrannyPending = false;
    if (!sGrannyResources.Prepare("mm"))
        return;
    std::array<Actor*, 2> flames{};
    size_t index = 0;
    for (const auto& actor : sGrannyResources.Decorations()) {
        auto* spawned = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_LIGHT, actor.x, actor.y, actor.z, actor.rx,
                                    actor.ry, actor.rz, BlackMarketGranny::MmFlameParams(actor.params));
        if (!spawned) {
            for (auto* previous : flames)
                if (previous)
                    Actor_Kill(previous);
            CVarSetString(BlackMarketGranny::kErrorCvar, "Unable to create Granny's decorative flames.");
            return;
        }
        flames[index++] = spawned;
    }
    static_assert(sizeof(EnvLightSettings) == 22);
    sGrannyLights.resize(sGrannyResources.Lights().size());
    std::memcpy(sGrannyLights.data(), sGrannyResources.Lights().data(),
                sGrannyLights.size() * sizeof(EnvLightSettings));
    play->envCtx.numLightSettings = sGrannyLights.size();
    play->envCtx.lightSettingsList = sGrannyLights.data();
    play->envCtx.lightMode = LIGHT_MODE_SETTINGS;
    play->envCtx.lightSetting = 0;
    std::memcpy(&play->envCtx.lightSettings, sGrannyLights.data(), sizeof(EnvLightSettings));
    sGrannyShape.normal = { { ROOM_SHAPE_TYPE_NORMAL }, 1, &sGrannyEntry, &sGrannyEntry + 1 };
    play->roomCtx.curRoom.roomShape = &sGrannyShape;
    sGrannyActive = true;
}
void DrawGrannyRoom() {
    ApplyGrannyInterior();
    if (!sGrannyActive || !Interior())
        return;
    auto* play = gPlayState;
    auto* frame = static_cast<Gfx*>(GRAPH_ALLOC(play->state.gfxCtx, 12 * sizeof(Gfx)));
    const auto tick = play->gameplayFrames;
    BlackMarketGranny::Wrap(frame, frame + 5, sGrannyResources.Opaque(), sGrannyResources.Translucent(),
                            Gfx_TexScrollEx(play->state.gfxCtx, 0, (tick * 3) % 128, 32, 32, 0, 3),
                            Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, 0, 1023 - (tick * 3) % 1024, 16, 256, 1, 0,
                                               1023 - (tick * 6) % 1024, 16, 256, 0, -3, 0, -6),
                            sGrannyResources.Route());
    sGrannyEntry = { frame, frame + 5 };
}
void RememberTravel() {
    sTravelSlot = gSaveContext.fileNum;
    sTravelSeed = gSaveContext.save.shipSaveInfo.rando.finalSeed;
}
bool MatchesTravel() {
    return sTravelSlot == gSaveContext.fileNum && sTravelSeed == gSaveContext.save.shipSaveInfo.rando.finalSeed;
}
bool Safe(const PlayState* play) {
    return play && gGameState == &play->state && play->state.running && gSaveContext.gameMode == GAMEMODE_NORMAL &&
           ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) && play->transitionTrigger == TRANS_TRIGGER_OFF &&
           play->transitionMode == TRANS_MODE_OFF && play->pauseCtx.state == PAUSE_STATE_OFF &&
           play->gameOverCtx.state == GAMEOVER_INACTIVE && play->csCtx.state == CS_STATE_IDLE &&
           !gSaveContext.cutsceneTrigger && !play->soaringCsOrSoTCsPlaying;
}
void Transition(PlayState* play, u16 entrance) {
    play->nextEntrance = entrance;
    play->transitionType = TRANS_TYPE_FADE_BLACK;
    gSaveContext.nextTransitionType = TRANS_TYPE_FADE_BLACK;
    play->transitionTrigger = TRANS_TRIGGER_START;
}
const Item* Offer(int slot) {
    return sSession.Active(gSaveContext.fileNum) && slot >= 0 && slot < 8
               ? Find(sSession.market["stock"][slot]["key"].get<std::string>())
               : nullptr;
}
PurchaseResult Status(int slot) {
    const Item* item = Offer(slot);
    if (!item)
        return PurchaseResult::Unavailable;
    return CanPurchase(sSession.market, sSession.state, slot,
                       gSaveContext.save.saveInfo.playerData.rupees + std::min<int>(0, gSaveContext.rupeeAccumulator),
                       ClearCount(sClears), ComboBlackMarketItems::Owned(*item),
                       ComboBlackMarketItems::Available(*item));
}
void Purchase() {
    switch (Status(sSelected)) {
        case PurchaseResult::Ready: {
            const Item* item = Offer(sSelected);
            const int price = sSession.market["stock"][sSelected]["price"];
            if (!item || !ComboBlackMarketItems::Grant(*item)) {
                sStatus = "That offer cannot be delivered yet.";
                break;
            }
            // Rupees_ChangeBy only queues HUD animation. Set the actual saved
            // balance so this snapshot cannot persist the reward with unpaid rupees.
            gSaveContext.save.saveInfo.playerData.rupees -= price;
            CommitPurchase(sSession.state, sSelected);
            SaveManager_SaveCurrentForCombo(true); // item, balance and shelf bit in one MM snapshot
            sStatus = std::string("Yours: ") + item->label + ".";
            break;
        }
        case PurchaseResult::SoldOut:
            sStatus = "That one has already been sold.";
            break;
        case PurchaseResult::Owned:
            sStatus = "You already own that power.";
            break;
        case PurchaseResult::Locked: {
            const int clears = sSession.market["stock"][sSelected]["clears"];
            sStatus = "The seal needs " + std::to_string(clears) + " unique dungeon clear" + (clears == 1 ? "." : "s.");
            break;
        }
        case PurchaseResult::NeedRupees:
            sStatus = "You need more rupees.";
            break;
        case PurchaseResult::Unavailable:
            sStatus = Offer(sSelected) && Offer(sSelected)->renewable ? "Bring an empty bottle for this refill."
                                                                      : "You need its prerequisite first.";
            break;
    }
}

#include "BlackMarketBrowsingNative.h"

void KeeperUpdate(Actor* actor, PlayState* play) {
    auto* gal = reinterpret_cast<EnTakaraya*>(actor);
    SkelAnime_Update(&gal->skelAnime);
    if (gal->timer > 0)
        --gal->timer;
    if (!gal->timer) {
        gal->eyeTexIndex = (gal->eyeTexIndex + 1) % 4;
        gal->timer = gal->eyeTexIndex ? 2 : 80 + (play->gameplayFrames % 53);
    }
    Actor_SetFocus(actor, 55.0f);
    if (!Active()) {
        EndBrowsing(play);
        return;
    }
    if (Actor_TalkOfferAccepted(actor, &play->state)) {
        if (!BeginBrowsing(play)) {
            Message_CloseTextbox(play);
            return;
        }
        Message_ContinueTextbox(play, kWelcomeText);
    } else if (!sTalking) {
        actor->textId = kWelcomeText;
        Actor_OfferTalk(actor, play, 150.0f);
    }
    if (!sTalking)
        return;
    BrowseInput(play);
}
s32 KeeperLimb(PlayState*, s32 limb, Gfx** display, Vec3f*, Vec3s*, Actor* actor) {
    if (limb == TREASURE_CHEST_SHOP_GAL_LIMB_HEAD) {
        static const char* heads[] = { "__OTR__custom/skiejer_market/keeper/ShopGalMMDBlinkHead0DL",
                                       "__OTR__custom/skiejer_market/keeper/ShopGalMMDBlinkHead1DL",
                                       "__OTR__custom/skiejer_market/keeper/ShopGalMMDBlinkHead2DL",
                                       "__OTR__custom/skiejer_market/keeper/ShopGalMMDBlinkHead1DL" };
        *display = reinterpret_cast<Gfx*>(const_cast<char*>(heads[reinterpret_cast<EnTakaraya*>(actor)->eyeTexIndex]));
    }
    return false;
}
void KeeperDraw(Actor* actor, PlayState* play) {
    auto* gal = reinterpret_cast<EnTakaraya*>(actor);
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    SkelAnime_DrawFlexOpa(play, gal->skelAnime.skeleton, gal->skelAnime.jointTable, gal->skelAnime.dListCount,
                          KeeperLimb, nullptr, actor);
}
void InitKeeper(Actor* actor) {
    if (!Interior())
        return;
    auto* gal = reinterpret_cast<EnTakaraya*>(actor);
    ActorShape_Init(&actor->shape, 0, nullptr, 0);
    Actor_SetScale(actor, BlackMarketKeeper::kScale);
    SkelAnime_InitFlex(gPlayState, &gal->skelAnime, (FlexSkeletonHeader*)kKeeper,
                       (AnimationHeader*)BlackMarketKeeper::kAnimation, gal->jointTable, gal->morphTable,
                       TREASURE_CHEST_SHOP_GAL_LIMB_MAX);
    actor->update = KeeperUpdate;
    actor->draw = KeeperDraw;
    actor->destroy = nullptr; // native destructor changes minigame switches and timers
    actor->flags |= ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_FRIENDLY | ACTOR_FLAG_UPDATE_CULLING_DISABLED;
    actor->shape.yOffset = 0;
    gal->eyeTexIndex = 0;
    gal->timer = 90;
    sTalking = false;
}
void MarketText(u16* textId, bool* fromTable) {
    if (!Active() ||
        (*textId != kBrowseText && *textId != kResultText && *textId != kWelcomeText && *textId != kConfirmText))
        return;
    CustomMessage::Entry entry;
    if (*textId == kWelcomeText) {
        entry.msg = "Welcome to the Black Market.\nLeft / Right: browse items\nA: Browse   B: Leave\x19";
    } else if (*textId == kConfirmText) {
        const auto* item = Offer(sSelected);
        if (!item)
            return;
        entry.firstItemCost = sSession.market["stock"][sSelected]["price"];
        entry.msg = std::string(item->label) + " - " + std::to_string(entry.firstItemCost) +
                    " rupees\n%g\xC2"
                    "Buy\nCancel\xBF";
    } else if (*textId == kResultText) {
        entry.msg = sStatus + "\x19";
    } else {
        const Item* item = Offer(sSelected);
        if (!item)
            return;
        const auto& stock = sSession.market["stock"][sSelected];
        std::string label;
        switch (Status(sSelected)) {
            case PurchaseResult::SoldOut:
                label = "Sold out";
                break;
            case PurchaseResult::Owned:
                label = "Already owned";
                break;
            case PurchaseResult::Locked:
                label = "Sealed: " + std::to_string(stock["clears"].get<int>()) + " dungeon clears";
                break;
            case PurchaseResult::Unavailable:
                label = item->renewable ? "Empty bottle required" : "Prerequisite required";
                break;
            default:
                label = "Tier " + std::to_string(item->tier);
                break;
        }
        entry.firstItemCost = stock["price"];
        entry.msg = std::string(item->label) + " - " + std::to_string(stock["price"].get<int>()) + " rupees\n" + label +
                    " (" + std::to_string(sSelected + 1) + "/8)\nA: Select   B: Back\x19";
    }
    CustomMessage::LoadCustomMessageIntoFont(entry);
    *fromTable = false;
}

void DrawSeal(PlayState* play, const BlackMarketVisual::Frame& frame, bool selected) {
    using namespace BlackMarketVisual;
    const auto mesh = BuildLock(frame, play->gameplayFrames, selected);
    if (!mesh.valid)
        return;
    auto* vertices = (Vtx*)GRAPH_ALLOC(play->state.gfxCtx, kLockVertices * sizeof(Vtx));
    if (!vertices)
        return;
    for (int i = 0; i < kLockVertices; ++i) {
        const auto& source = mesh.vertices[i];
        auto& v = vertices[i];
        v = {};
        v.v.ob[0] = source.x;
        v.v.ob[1] = source.y;
        v.v.ob[2] = source.z;
        v.v.tc[0] = source.s;
        v.v.tc[1] = source.t;
        v.v.cn[0] = v.v.cn[1] = v.v.cn[2] = v.v.cn[3] = 255;
    }
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    Matrix_Push();
    Matrix_Translate(mesh.center.x, mesh.center.y, mesh.center.z, MTXMODE_NEW);
    Matrix_Scale(1.f / kLockVertexScale, 1.f / kLockVertexScale, 1.f / kLockVertexScale, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gSPClearGeometryMode(POLY_XLU_DISP++, G_LIGHTING | G_FOG | G_CULL_BOTH);
    // SETUPDL_25 uses opaque blending even on the translucent command stream.
    gDPSetRenderMode(POLY_XLU_DISP++, G_RM_PASS, G_RM_AA_ZB_XLU_SURF2);
    gDPSetTextureLUT(POLY_XLU_DISP++, G_TT_NONE);
    gDPSetCombineMode(POLY_XLU_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, mesh.color[0], mesh.color[1], mesh.color[2], mesh.color[3]);
    gSPTexture(POLY_XLU_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPLoadTextureBlock(POLY_XLU_DISP++, kLockTexture.data(), G_IM_FMT_IA, G_IM_SIZ_8b, kLockTextureWidth,
                        kLockTextureHeight, 0, G_TX_WRAP, G_TX_CLAMP, 5, 3, 0, 0);
    for (int batch = 0; batch < kLockVertices; batch += 32) {
        gSPVertex(POLY_XLU_DISP++, (uintptr_t)(vertices + batch), 32, 0);
        for (int i = 0; i < 8; ++i)
            gSP2Triangles(POLY_XLU_DISP++, i * 4, i * 4 + 1, i * 4 + 2, 0, i * 4, i * 4 + 2, i * 4 + 3, 0);
    }
    gSPTexture(POLY_XLU_DISP++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    Matrix_Pop();
    gDPPipeSync(POLY_XLU_DISP++);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
}

void DrawShelves(PlayState* play) {
    if (!sShelfFixture.Ensure(play))
        return;
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    Matrix_Push();
    Matrix_Translate(0, 0, 0, MTXMODE_NEW);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    const auto color =
        CVarGetColor("gCosmetic.World.BlackMarketShelves.Color",
                     Color_RGBA8{ BlackMarketShelves::kDefaultTint[0], BlackMarketShelves::kDefaultTint[1],
                                  BlackMarketShelves::kDefaultTint[2], 255 });
    sShelfFixture.Draw(POLY_OPA_DISP, sGrannyActive, sGrannyResources.ShelfTextures(),
                       BlackMarketShelves::Tint(CVarGetInteger("gCosmetic.World.BlackMarketShelves.Changed", 0) != 0,
                                                { color.r, color.g, color.b, 255 }));
    Matrix_Pop();
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
}
void DrawMarket() {
    PlayState* play = gPlayState;
    if (!Active() || !play->state.running)
        return;
    DrawShelves(play);
    for (int slot = 0; slot < 8; ++slot) {
        const Item* item = Offer(slot);
        if (!item)
            continue;
        const bool locked = Status(slot) == PurchaseResult::Locked;
        BlackMarketVisual::Capture capture(locked);
        Actor& display = sDisplays[slot];
        display.id = ACTOR_EN_GIRLA;
        display.world.pos = kShelves[slot];
        display.scale = { .25f, .25f, .25f };
        Matrix_Push();
        Matrix_Translate(display.world.pos.x,
                         display.world.pos.y + 2.0f * std::sin((play->gameplayFrames + slot * 7) * .04f),
                         display.world.pos.z, MTXMODE_NEW);
        Matrix_RotateYS(play->gameplayFrames * 100 + slot * 4096, MTXMODE_APPLY);
        const float scale = sTalking && slot == sSelected ? .32f : .25f;
        Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
        Rando::DrawResolvedItem(ComboBlackMarketItems::Id(*item), RC_UNKNOWN, &display);
        Matrix_Pop();
        if (locked && CVarGetInteger(BlackMarketVisual::kSealsCvar, 1))
            DrawSeal(play, capture.frame, sTalking && slot == sSelected);
    }
}

void PlayerUpdate(Actor* actor) {
    PlayState* play = gPlayState;
    if (!Safe(play) || Player_InCsMode(play))
        return;
    if (Interior()) {
        if (!Active() || ComboBlackMarket_AtExit(actor->world.pos.x, actor->world.pos.y, actor->world.pos.z)) {
            RememberTravel();
            sReturnPending = true;
            Transition(play, ReturnEntrance());
        }
        return;
    }
    if (!Exterior(play) || !sSession.Active(gSaveContext.fileNum) || !PackAvailable())
        return;
    const auto door = Door();
    Vec3f probe = { door.x, 120.0f, door.z };
    CollisionPoly* floor = nullptr;
    s32 bg = BGCHECK_SCENE;
    const float height = BgCheck_EntityRaycastFloor5_3(play, &play->colCtx, &floor, &bg, actor, &probe);
    if (height == BGCHECK_Y_MIN || !floor || floor->normal.y < 23170)
        return;
    sDoorFloor = height;
    const float dx = actor->world.pos.x - door.x;
    const float dz = actor->world.pos.z - door.z;
    if (dx * dx + dz * dz > 14400) {
        sDoorArmed = true;
        if (SoloExterior(play) && actor->floorPoly && actor->floorPoly->normal.y >= 23170 &&
            std::isfinite(actor->floorHeight) && std::abs(actor->world.pos.y - actor->floorHeight) < 35.0f) {
            sSoloReturn = actor->world.pos;
            sSoloReturnYaw = actor->shape.rot.y;
            sSoloReturnEntrance = gSaveContext.save.entrance;
        }
    }
    const bool atDoor = ComboBlackMarket_InDoor(actor->world.pos.x, actor->world.pos.y, actor->world.pos.z, height);
    if (sDoorArmed && atDoor) {
        sDoorArmed = false;
        sEntering = true;
        RememberTravel();
        Transition(play, COMBO_BLACK_MARKET_ENTRANCE);
    }
}
void SceneInit(s8 scene, s8 spawn) {
    sShelfFixture.Reset();
    EndBrowsing(gPlayState, true);
    GrannySceneBegin();
    ApplyGrannyInterior();
    if (sReturnStaged) {
        gSaveContext.respawn[RESPAWN_MODE_RETURN] = sPreservedReturn;
        sReturnStaged = false;
    }
    sEntering = false;
    sDoorArmed = false;
    sTalking = false;
    sTitlePending = false;
    if (scene == COMBO_BLACK_MARKET_SCENE) {
        sSelected = 0;
        sDisplays = {};
        sTitlePending = Active() && PackAvailable();
    } else if (sReturnPending && MatchesTravel() && Exterior(gPlayState)) {
        sReturnPending = false;
        Player* player = GET_PLAYER(gPlayState);
        if (!player)
            return;
        Vec3f pos = SoloExterior(gPlayState)
                        ? sSoloReturn
                        : Vec3f{ COMBO_BLACK_MARKET_DOOR_X - 226.0f, 0, COMBO_BLACK_MARKET_DOOR_Z };
        pos.y += 120.0f;
        CollisionPoly* floor = nullptr;
        s32 bg = BGCHECK_SCENE;
        const float y =
            BgCheck_EntityRaycastFloor5_3(gPlayState, &gPlayState->colCtx, &floor, &bg, &player->actor, &pos);
        if (y == BGCHECK_Y_MIN || !floor || floor->normal.y < 23170)
            return;
        pos.y = y;
        const s16 yaw = SoloExterior(gPlayState) ? sSoloReturnYaw : -16384;
        if (player->actionFunc == Player_Action_35) {
            const float turn = (s16)(yaw - player->actor.shape.rot.y) * (6.28318530718f / 65536.0f);
            const Vec3f delta = { player->unk_3A0.x - player->actor.world.pos.x,
                                  player->unk_3A0.y - player->actor.world.pos.y,
                                  player->unk_3A0.z - player->actor.world.pos.z };
            player->unk_3A0 = { pos.x + delta.x * std::cos(turn) + delta.z * std::sin(turn), pos.y + delta.y,
                                pos.z - delta.x * std::sin(turn) + delta.z * std::cos(turn) };
        }
        player->actor.world.pos = player->actor.prevPos = player->actor.home.pos = pos;
        player->yaw = player->actor.world.rot.y = player->actor.shape.rot.y = yaw;
        player->actor.velocity = {};
        player->actor.speed = 0;
        player->actor.floorHeight = y;
        player->actor.floorPoly = floor;
        player->actor.floorBgId = bg;
        Camera_InitFocalActorSettings(&gPlayState->mainCamera, &player->actor);
    } else {
        sReturnPending = false;
    }
}
void DrawExterior() {
    PlayState* play = gPlayState;
    if (!Exterior(play) || !sSession.Active(gSaveContext.fileNum) || !PackAvailable())
        return;
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    Matrix_Push();
    const auto door = Door();
    Matrix_Translate(door.x, sDoorFloor, door.z, MTXMODE_NEW);
    Matrix_RotateYS(COMBO_BLACK_MARKET_DOOR_YAW, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)kExterior);
    Matrix_Pop();
    CLOSE_DISPS(play->state.gfxCtx);
}
void LoadSolo(s16 slot) {
    if (gComboGameMode != COMBO_GAME_MM || !ComboLostWoods_IsPlayableSlot(slot))
        return;
    nlohmann::json saved;
    if (SaveManager_ReadSaveFile(SaveManager_GetFileName(slot + 1), saved) != 0) {
        sSession.Bind(nullptr, -1);
        return;
    }
    const bool create = !saved.contains("blackMarket");
    if (create) {
        Options options;
        options.games = Games::Mm;
        options.dungeonUnlocks = CVarGetInteger("gCombo.Rando.BlackMarketDungeonUnlocks", 1) != 0;
        saved["blackMarket"] = { { "market", Generate(gSaveContext.save.shipSaveInfo.rando.finalSeed, options) } };
    }
    ComboBlackMarket_LoadState(saved, slot);
    if (create && sSession.Active(slot))
        SaveManager_WriteSaveFile(SaveManager_GetFileName(slot + 1), saved);
    // Native load hooks may still replace entrance/respawns. Play_Init's
    // normalizer consumes the seed/slot-bound saved-inside marker last.
}

void RegisterMarket() {
    static EntranceTableEntry entries[4] = { { COMBO_BLACK_MARKET_SCENE, 0, TRANS_TYPE_FADE_BLACK << 7 },
                                             { COMBO_BLACK_MARKET_SCENE, 0, TRANS_TYPE_FADE_BLACK << 7 },
                                             { COMBO_BLACK_MARKET_SCENE, 0, TRANS_TYPE_FADE_BLACK << 7 },
                                             { COMBO_BLACK_MARKET_SCENE, 0, TRANS_TYPE_FADE_BLACK << 7 } };
    static EntranceTableEntry* spawns[] = { entries };
    auto* table = &gSceneTable[COMBO_BLACK_MARKET_SCENE];
    table->segment.vromStart = table->segment.vromEnd = 0;
    table->segment.fileName = (char*)"skiejer_market";
    table->titleTextId = 0;
    table->drawConfig = SCENE_DRAW_CFG_DEFAULT;
    table->unk_A = table->unk_C = table->unk_D = 0;
    sSceneEntranceTable[ENTR_SCENE_UNSET_09].tableCount = 1;
    sSceneEntranceTable[ENTR_SCENE_UNSET_09].table = spawns;
    sSceneEntranceTable[ENTR_SCENE_UNSET_09].name = (char*)"SKIEJER_MARKET";
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>(SceneInit);
    Ship::CrossRMRegistry::RegisterTeardownListener([] { sGrannyResources.Clear(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawWorldStart>(DrawGrannyRoom);
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::ShouldActorInit>(
        ACTOR_EN_TAKARAYA, [](Actor* actor, bool* should) {
            if (!Interior())
                return;
            // Returning false kills the actor in this engine. Replace its init
            // pointer before dispatch, then let the normal lifecycle call us.
            actor->init = [](Actor* keeper, PlayState*) { InitKeeper(keeper); };
            *should = true;
        });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSaveLoad>(LoadSolo);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSaveInit>([](s16) {
        if (gComboGameMode != COMBO_GAME_MM)
            return;
        sSession.Bind(nullptr, -1);
        sClears = 0;
        sEntering = sReturnPending = sReturnStaged = sResumeFromMarket = false;
    });
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnActorUpdate>(ACTOR_PLAYER, PlayerUpdate);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnOpenText>(MarketText);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawWorldEnd>([] {
        DrawMarket();
        DrawBrowseCursor();
        DrawExterior();
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameStateUpdate>([] {
        MonitorBrowsing();
        if (sTitlePending && Active() && ComboPlaceTitleCard_Show(gPlayState, (TexturePtr)kCard, 192, 24) != 0)
            sTitlePending = false;
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([] {
        sShelfFixture.Reset();
        EndBrowsing(gPlayState, true);
        sGrannyActive = sGrannyPending = false;
        sTitlePending = sTalking = sDoorArmed = false;
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnBossDefeated>([](s16 actor) {
        if (!gPlayState || !sSession.Active(gSaveContext.fileNum) || gSaveContext.gameMode != GAMEMODE_NORMAL)
            return;
        int boss = actor == ACTOR_BOSS_01        ? 8
                   : actor == ACTOR_BOSS_HAKUGIN ? 9
                   : actor == ACTOR_BOSS_03      ? 10
                   : actor == ACTOR_BOSS_02      ? 11
                                                 : -1;
        static constexpr int scenes[] = { SCENE_MITURIN_BS, SCENE_HAKUGIN_BS, SCENE_SEA_BS, SCENE_INISIE_BS };
        if (boss >= 0 && gPlayState->sceneId != scenes[boss - 8])
            return;
        if (boss < 0)
            return;
        if (gComboGameMode == COMBO_GAME_MM) {
            const auto next = static_cast<uint16_t>(sClears | (1u << boss));
            if (next != sClears) {
                sClears = next;
                SaveManager_SaveCurrentForCombo(true);
            }
        } else if (Combo_IsCombinedGame() && sReportClear)
            sReportClear(boss, gSaveContext.fileNum);
    });
}
RegisterShipInitFunc init(RegisterMarket);
} // namespace

extern "C" COMBO_EXPORT void MM_SetBlackMarketSeed(const char* text, int slot, uint16_t clears) try {
    auto value = nlohmann::json::parse(text ? text : "null");
    sSession.Bind(value, slot);
    sClears = clears & 0xFFF;
} catch (...) {
    sSession.Bind(nullptr, -1);
    sClears = 0;
}
extern "C" COMBO_EXPORT void MM_SetBlackMarketClearCallback(void (*callback)(int, int)) {
    sReportClear = callback;
}
extern "C" int ComboBlackMarket_IsInterior(void) {
    return Interior();
}
extern "C" void ComboBlackMarket_ResetState(void) {
    sSession.state = {};
    sResumeFromMarket = false;
}
void ComboBlackMarket_SaveState(nlohmann::json& save, int slot) {
    // Native file-select copies/imports write full documents without loading
    // their market session. Preserve their existing stock and bits verbatim.
    if (gComboGameMode == COMBO_GAME_MM &&
        (!gPlayState || gGameState != &gPlayState->state || !gPlayState->state.running))
        return;
    if (sSession.Active(slot) && slot == gSaveContext.fileNum) {
        save["blackMarket"] = sSession.Save(slot);
        if (gComboGameMode == COMBO_GAME_MM) {
            save["blackMarket"]["clears"] = sClears;
            save["blackMarket"]["return"] = { { "entrance", sSoloReturnEntrance },
                                              { "x", sSoloReturn.x },
                                              { "y", sSoloReturn.y },
                                              { "z", sSoloReturn.z },
                                              { "yaw", sSoloReturnYaw } };
        }
        save["blackMarket"]["savedInside"] = Interior() && gSaveContext.save.entrance == COMBO_BLACK_MARKET_ENTRANCE;
    }
}
void ComboBlackMarket_LoadState(const nlohmann::json& save, int slot) {
    const auto value = save.is_object() ? save.value("blackMarket", nlohmann::json()) : nlohmann::json();
    if (gComboGameMode == COMBO_GAME_MM) {
        sEntering = sReturnPending = sReturnStaged = sResumeFromMarket = false;
        const auto market = value.is_object() ? value.value("market", nlohmann::json()) : nlohmann::json();
        // A native file owns its frozen stock; never accept a combined or OoT
        // payload in solo MM, even if it happens to use the same seed number.
        const bool valid = Validate(market) && MatchesGames(market, Games::Mm) &&
                           market["seed"] == gSaveContext.save.shipSaveInfo.rando.finalSeed;
        sSession.Bind(valid ? market : nlohmann::json(), slot);
        const auto clears = value.is_object() ? value.value("clears", nlohmann::json(0)) : nlohmann::json();
        sClears = valid && UnsignedInRange(clears, 0xF00) ? clears.get<uint16_t>() & 0xF00 : 0;
        sSoloReturn = { 184, 0, -300 };
        sSoloReturnYaw = -16384;
        sSoloReturnEntrance = COMBO_SOLO_FOREST_MM_ENTRANCE;
        const auto point = value.is_object() ? value.value("return", nlohmann::json()) : nlohmann::json();
        if (point.is_object())
            try {
                const auto entrance = point.at("entrance").get<int>();
                const Vec3f pos = { point.at("x").get<float>(), point.at("y").get<float>(),
                                    point.at("z").get<float>() };
                const auto yaw = point.at("yaw").get<int>();
                if (entrance >= 0 && entrance == COMBO_SOLO_FOREST_MM_ENTRANCE && std::isfinite(pos.x) &&
                    std::isfinite(pos.y) && std::isfinite(pos.z) && std::abs(pos.x) < 4096 && std::abs(pos.y) < 4096 &&
                    std::abs(pos.z) < 4096 && yaw >= INT16_MIN && yaw <= INT16_MAX) {
                    sSoloReturn = pos;
                    sSoloReturnYaw = yaw;
                    sSoloReturnEntrance = entrance;
                }
            } catch (const nlohmann::json::exception&) {}
    }
    sSession.Load(value, slot);
    sResumeFromMarket = sSession.Active(slot) && value.is_object() &&
                        value.value("market", nlohmann::json()) == sSession.market &&
                        value.value("savedInside", nlohmann::json()) == true;
    if (gComboGameMode == COMBO_GAME_MM && sResumeFromMarket)
        RememberTravel();
}
extern "C" void ComboBlackMarket_ApplyResume(int resume) {
    const bool fromMarket = sResumeFromMarket;
    sResumeFromMarket = false;
    if (!resume || !fromMarket || !sSession.Active(gSaveContext.fileNum))
        return;
    sReturnPending = true;
    RememberTravel();
    gSaveContext.save.entrance = ReturnEntrance();
    gSaveContext.save.cutsceneIndex = 0;
    gSaveContext.respawnFlag = 0;
    StageSoloReturn();
}
extern "C" void ComboBlackMarket_NormalizeEntry(void) {
    if (gComboGameMode == COMBO_GAME_OOT)
        return;
    if (gComboGameMode == COMBO_GAME_MM && sResumeFromMarket) {
        sResumeFromMarket = false;
        if (MatchesTravel() && sSession.Active(gSaveContext.fileNum)) {
            sEntering = false;
            sReturnPending = true;
            gSaveContext.save.entrance = ReturnEntrance();
            gSaveContext.save.cutsceneIndex = 0;
            gSaveContext.respawnFlag = 0;
            StageSoloReturn();
            return;
        }
    }
    if (sReturnPending && MatchesTravel() && gSaveContext.save.entrance == ReturnEntrance()) {
        StageSoloReturn();
        return;
    }
    if (gSaveContext.save.entrance != COMBO_BLACK_MARKET_ENTRANCE)
        return;
    if (sEntering && MatchesTravel() && sSession.Active(gSaveContext.fileNum) && PackAvailable() &&
        gSaveContext.respawnFlag == 0)
        return;
    // Save reload, void/death and missing-pack recovery use the ordinary MM
    // forest spawn. No portal callback, shared arrival flag or TOP respawn write.
    sEntering = false;
    sReturnPending = true;
    RememberTravel();
    gSaveContext.save.entrance = ReturnEntrance();
    gSaveContext.save.cutsceneIndex = 0;
    gSaveContext.respawnFlag = 0;
    StageSoloReturn();
}
#endif
