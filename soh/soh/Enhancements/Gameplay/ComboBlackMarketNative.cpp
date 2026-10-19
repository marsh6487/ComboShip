#ifdef COMBO_BUILD
#include "ComboBlackMarketNative.h"
#include "ComboBlackMarketItems.h"
#include "ComboGameMode.h"
#include "entrances/BlackMarketEntrance.h"
#include "entrances/SoloForest.h"
#include "BlackMarketLock.h"
#include "BlackMarketKeeper.h"
#include "BlackMarketGrannyResources.h"
#include "BlackMarketBrowsing.h"
#include "entrances/LostWoodsEntrance.h"
#include "soh/SaveManager.h"
#include "soh/ActorDB.h"
#include "soh/ShipInit.hpp"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/Enhancements/randomizer/SeedContext.h"
#include "soh/Enhancements/randomizer/static_data.h"
#include "soh/Enhancements/randomizer/NeiGiPresentation.h"
#include "../../../../mm/2s2h/Enhancements/Gameplay/BlackMarketResources.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <algorithm>
#include <array>
#include <cmath>
extern "C" {
#include "global.h"
u8 ResourceMgr_FileExists(const char*);
const char* SOH_ReadBlackMarketOptions();
}
#include "BlackMarketShelvesNative.h"

namespace {
using namespace ComboRando::BlackMarket;
Session sSession;
nlohmann::json sPendingSave;
bool sFileLoadPending = true;
u16 sClears = 0;
bool sEntering = false, sReturnPending = false, sReturnStaged = false, sResumeFromMarket = false;
bool sDoorArmed = false, sTalking = false, sTitlePending = false;
bool sKeeperSpawned = false;
int sTravelSlot = -1, sSelected = 0;
float sDoorFloor = 0;
u32 sTravelSeed = 0;
Vec3f sReturnPos = { 184, 0, -300 };
s16 sReturnYaw = -16384;
u16 sReturnEntrance = ENTR_SKIEJER_FOREST;
// Movement updates only this main-thread candidate. The serialized return
// point is published once at entry, after prior native saves have finished.
Vec3f sGroundedReturnPos = { 184, 0, -300 };
s16 sGroundedReturnYaw = 0;
u16 sGroundedReturnEntrance = ENTR_SKIEJER_FOREST;
RespawnData sPreservedReturn;
std::string sStatus;
constexpr u16 kBrowseText = 0x8F10, kResultText = 0x8F11, kWelcomeText = 0x8F12, kConfirmText = 0x8F13;
int sKeeperActorId = -1;
constexpr int kLimbCount = 24, kHeadLimb = 5;
alignas(2) constexpr char kKeeper[] = "__OTR__custom/skiejer_market/keeper/gTreasureChestShopGalSkel";
alignas(2) constexpr char kCard[] = "__OTR__custom/skiejer_market/title";
alignas(2) constexpr char kExterior[] = "__OTR__custom/skiejer_market/entrance";
struct KeeperState {
    SkelAnime skel;
    Vec3s joints[kLimbCount], morphs[kLimbCount];
    int eye, timer;
} sKeeper;
BlackMarketShelvesNative::Fixture sShelfFixture;
constexpr Vec3f kShelves[8] = {
    // Two framed shelf banks; keep all offers on one frontal plane.
    // Upper anchors clear the physical middle board and selected-item seals.
    { 35, 58, 110 },  { 35, 110, 110 },  { 70, 58, 110 },  { 70, 110, 110 },
    { -35, 58, 110 }, { -35, 110, 110 }, { -70, 58, 110 }, { -70, 110, 110 }
};

u32 Seed() {
    return IS_RANDO ? Rando::Context::GetInstance()->GetSeed() : 0;
}
bool PackAvailable() {
    return std::all_of(std::begin(kBlackMarketResources), std::end(kBlackMarketResources),
                       [](const char* path) { return ResourceMgr_FileExists(path); });
}
bool Interior() {
    return gComboGameMode == COMBO_GAME_OOT && gPlayState && gPlayState->sceneNum == SCENE_SKIEJER_MARKET;
}
bool Exterior(const PlayState* play) {
    return play && play->roomCtx.curRoom.num == 0 &&
           ComboSoloForest_IsOotVisit(play->sceneNum, play->roomCtx.curRoom.num, gSaveContext.gameMode,
                                      gSaveContext.fileNum, gSaveContext.sceneLayer, gSaveContext.cutsceneIndex);
}
bool Active() {
    return Interior() && sSession.Active(gSaveContext.fileNum);
}

BlackMarketGranny::Resources sGrannyResources;
bool sGrannyPending = false, sGrannyActive = false;
MeshHeader sGrannyShape{};
PolygonDlist sGrannyEntry{};
std::vector<EnvLightSettings> sGrannyLights;
void GrannySceneBegin() {
    sGrannyActive = false;
    sGrannyPending = Interior() && CVarGetInteger(BlackMarketGranny::kCvar, 0) != 0;
}
void ApplyGrannyInterior() {
    auto* play = gPlayState;
    if (!sGrannyPending || !Interior() || play->roomCtx.curRoom.num != 0 || !play->roomCtx.curRoom.meshHeader)
        return;
    sGrannyPending = false;
    if (!sGrannyResources.Prepare("oot"))
        return;
    std::array<Actor*, 2> flames{};
    size_t index = 0;
    for (const auto& actor : sGrannyResources.Decorations()) {
        auto* spawned = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_LIGHT, actor.x, actor.y, actor.z, actor.rx,
                                    actor.ry, actor.rz, actor.params);
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
    play->envCtx.blendIndoorLights = 0;
    play->envCtx.unk_BD = play->envCtx.unk_BE = 0;
    std::memcpy(&play->envCtx.lightSettings, sGrannyLights.data(), sizeof(EnvLightSettings));
    sGrannyShape.polygon0 = { { 0 }, 1, &sGrannyEntry, &sGrannyEntry + 1 };
    play->roomCtx.curRoom.meshHeader = &sGrannyShape;
    sGrannyActive = true;
}
void DrawGrannyRoom() {
    ApplyGrannyInterior();
    if (!sGrannyActive || !Interior())
        return;
    auto* play = gPlayState;
    auto* frame = static_cast<Gfx*>(Graph_Alloc(play->state.gfxCtx, 12 * sizeof(Gfx)));
    const auto tick = play->gameplayFrames;
    BlackMarketGranny::Wrap(frame, frame + 5, sGrannyResources.Opaque(), sGrannyResources.Translucent(),
                            Gfx_TexScrollEx(play->state.gfxCtx, 0, (tick * 3) % 128, 32, 32, 0, 3),
                            Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, 0, 1023 - (tick * 3) % 1024, 16, 256, 1, 0,
                                               1023 - (tick * 6) % 1024, 16, 256, 0, -3, 0, -6),
                            sGrannyResources.Route());
    sGrannyEntry = { frame, frame + 5 };
}
bool Safe(PlayState* play) {
    return play && play->state.running && gSaveContext.gameMode == GAMEMODE_NORMAL &&
           ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) && play->transitionTrigger == TRANS_TRIGGER_OFF &&
           play->transitionMode == TRANS_MODE_OFF && play->pauseCtx.state == 0 &&
           play->gameOverCtx.state == GAMEOVER_INACTIVE && play->csCtx.state == CS_STATE_IDLE &&
           !gSaveContext.cutsceneTrigger;
}
void RememberTravel() {
    sTravelSlot = gSaveContext.fileNum;
    sTravelSeed = Seed();
}
bool MatchesTravel() {
    return sTravelSlot == gSaveContext.fileNum && sTravelSeed == Seed();
}
void StageReturn() {
    if (sReturnStaged)
        return;
    auto& arrival = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    sPreservedReturn = arrival;
    sReturnStaged = true;
    arrival = {};
    arrival.entranceIndex = sReturnEntrance;
    arrival.roomIndex = 0;
    arrival.pos = sReturnPos;
    arrival.yaw = sReturnYaw;
    arrival.playerParams = (PLAYER_START_MODE_IDLE << 8) | 0xFF;
    gSaveContext.respawnFlag = 2;
}
void Transition(PlayState* play, u16 entrance) {
    // Direct native routes never call the entrance randomizer or the launcher.
    play->nextEntranceIndex = entrance;
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
                       gSaveContext.rupees + std::min<int>(0, gSaveContext.rupeeAccumulator), ClearCount(sClears),
                       ComboBlackMarketOotItems::Owned(*item), ComboBlackMarketOotItems::Available(*item));
}
void Purchase() {
    if (Status(sSelected) != PurchaseResult::Ready) {
        switch (Status(sSelected)) {
            case PurchaseResult::SoldOut:
                sStatus = "That one has already been sold.";
                break;
            case PurchaseResult::Owned:
                sStatus = "You already own that power.";
                break;
            case PurchaseResult::Locked:
                sStatus = "The seal needs more unique dungeon clears.";
                break;
            case PurchaseResult::NeedRupees:
                sStatus = "You need more rupees.";
                break;
            default:
                sStatus = "You need its prerequisite first.";
                break;
        }
        return;
    }
    SaveManager::Instance->ThreadPoolWait();
    const auto* item = Offer(sSelected);
    if (!item || !ComboBlackMarketOotItems::Grant(*item)) {
        sStatus = "That offer cannot be delivered yet.";
        return;
    }
    gSaveContext.rupees -= sSession.market["stock"][sSelected]["price"].get<int>();
    CommitPurchase(sSession.state, sSelected);
    // NEI, rupees and market bits share the same synchronous atomic native save.
    SaveManager::Instance->SaveSection(gSaveContext.fileNum, SECTION_ID_BASE, false);
    sStatus = std::string("Yours: ") + item->label + ".";
}

#define BLACK_MARKET_BROWSING_OOT
#include "BlackMarketBrowsingNative.h"
#undef BLACK_MARKET_BROWSING_OOT

void KeeperUpdate(Actor* actor, PlayState* play) {
    SkelAnime_Update(&sKeeper.skel);
    if (sKeeper.timer > 0)
        --sKeeper.timer;
    if (!sKeeper.timer) {
        sKeeper.eye = (sKeeper.eye + 1) % 4;
        sKeeper.timer = sKeeper.eye ? 2 : 80 + play->gameplayFrames % 53;
    }
    Actor_SetFocus(actor, 55);
    if (!Active()) {
        EndBrowsing(play);
        return;
    }
    if (Actor_ProcessTalkRequest(actor, play)) {
        if (!BeginBrowsing(play)) {
            Message_CloseTextbox(play);
            return;
        }
        Message_ContinueTextbox(play, kWelcomeText);
    } else if (!sTalking) {
        actor->textId = kWelcomeText;
        Actor_OfferTalk(actor, play, 150);
    }
    if (!sTalking)
        return;
    BrowseInput(play);
}
s32 KeeperLimb(PlayState*, s32 limb, Gfx** display, Vec3f*, Vec3s*, void*) {
    if (limb == kHeadLimb) {
        static const char* heads[] = { "__OTR__custom/skiejer_market/keeper/ShopGalMMDBlinkHead0DL",
                                       "__OTR__custom/skiejer_market/keeper/ShopGalMMDBlinkHead1DL",
                                       "__OTR__custom/skiejer_market/keeper/ShopGalMMDBlinkHead2DL",
                                       "__OTR__custom/skiejer_market/keeper/ShopGalMMDBlinkHead1DL" };
        *display = (Gfx*)heads[sKeeper.eye];
    }
    return false;
}
void KeeperDraw(Actor* actor, PlayState* play) {
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    SkelAnime_DrawFlexOpa(play, sKeeper.skel.skeleton, sKeeper.skel.jointTable, sKeeper.skel.dListCount, KeeperLimb,
                          nullptr, actor);
}
void KeeperInit(Actor* actor, PlayState* play) {
    if (!Interior() || !PackAvailable()) {
        Actor_Kill(actor);
        return;
    }
    sKeeper = {};
    ActorShape_Init(&actor->shape, 0, nullptr, 0);
    Actor_SetScale(actor, BlackMarketKeeper::kScale);
    actor->flags = ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_FRIENDLY | ACTOR_FLAG_UPDATE_CULLING_DISABLED;
    SkelAnime_InitFlex(play, &sKeeper.skel, (FlexSkeletonHeader*)kKeeper,
                       (AnimationHeader*)BlackMarketKeeper::kAnimation, sKeeper.joints, sKeeper.morphs, kLimbCount);
    actor->update = KeeperUpdate;
    actor->draw = KeeperDraw;
    actor->destroy = nullptr;
    sKeeper.timer = 90;
}
void RegisterKeeperActor() {
    if (sKeeperActorId >= 0)
        return;
    ActorDBInit profile;
    profile.name = "En_SkiejerMarketKeeper";
    profile.desc = "Skiejer's Black Market Goth Shop Girl";
    profile.category = ACTORCAT_NPC;
    profile.flags = ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_FRIENDLY | ACTOR_FLAG_UPDATE_CULLING_DISABLED |
                    ACTOR_FLAG_DRAW_CULLING_DISABLED;
    profile.objectId = OBJECT_GAMEPLAY_KEEP;
    profile.instanceSize = sizeof(Actor);
    profile.init = KeeperInit;
    profile.update = KeeperUpdate;
    profile.draw = KeeperDraw;
    sKeeperActorId = ActorDB::Instance->AddEntry(profile).entry.id;
}
void MarketText(u16* textId, bool* fromTable) {
    if (!Active() ||
        (*textId != kBrowseText && *textId != kResultText && *textId != kWelcomeText && *textId != kConfirmText))
        return;
    std::string message;
    if (*textId == kWelcomeText)
        message = "Welcome to the Black Market.&Left / Right: browse items&A: Browse   B: Leave\x0B";
    else if (*textId == kConfirmText) {
        const auto* item = Offer(sSelected);
        if (!item)
            return;
        message = std::string(item->label) + " - " +
                  std::to_string(sSession.market["stock"][sSelected]["price"].get<int>()) + " rupees^" +
                  CustomMessage::TWO_WAY_CHOICE() + "%gBuy&Cancel%w";
    } else if (*textId == kResultText)
        message = sStatus + "\x0B";
    else {
        const auto* item = Offer(sSelected);
        if (!item)
            return;
        const auto& offer = sSession.market["stock"][sSelected];
        std::string label = "Tier " + std::to_string(item->tier);
        switch (Status(sSelected)) {
            case PurchaseResult::SoldOut:
                label = "Sold out";
                break;
            case PurchaseResult::Owned:
                label = "Already owned";
                break;
            case PurchaseResult::Locked:
                label = "Sealed: " + std::to_string(offer["clears"].get<int>()) + " dungeon clears";
                break;
            case PurchaseResult::Unavailable:
                label = "Prerequisite required";
                break;
            default:
                break;
        }
        message = std::string(item->label) + " - " + std::to_string(offer["price"].get<int>()) + " rupees&" + label +
                  " (" + std::to_string(sSelected + 1) + "/8)&A: Select   B: Back\x0B";
    }
    CustomMessage entry(message);
    entry.Format();
    entry.LoadIntoFont();
    *fromTable = false;
}
void DrawSeal(PlayState* play, const BlackMarketVisual::Frame& frame, bool selected) {
    using namespace BlackMarketVisual;
    const auto mesh = BuildLock(frame, play->gameplayFrames, selected);
    if (!mesh.valid)
        return;
    auto* vertices = (Vtx*)Graph_Alloc(play->state.gfxCtx, kLockVertices * sizeof(Vtx));
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
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    Matrix_Push();
    Matrix_Translate(mesh.center.x, mesh.center.y, mesh.center.z, MTXMODE_NEW);
    Matrix_Scale(1.f / kLockVertexScale, 1.f / kLockVertexScale, 1.f / kLockVertexScale, MTXMODE_APPLY);
    gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
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
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
}

void PresentTitle() {
    // TitleCard_InitPlaceName rejects new scene IDs and rewrites language
    // suffixes. Initialize its ordinary native context with our private card;
    // native timing, HUD cosmetics and OnPresentTitleCard still own display.
    auto& card = gPlayState->actorCtx.titleCtx;
    card = {};
    card.texture = (void*)kCard;
    card.isBossCard = false;
    card.hasTranslation = false;
    card.x = 160;
    card.y = 180;
    card.width = 192;
    card.height = 24;
    card.durationTimer = 80;
    card.delayTimer = 20;
}

void DrawOffer(PlayState* play, const Item& item) {
    const auto entry = Rando::StaticData::RetrieveItem(ComboBlackMarketOotItems::Id(item)).GetGIEntryUnresolved();
    if (entry && !NeiGi_DrawShop(play, entry.get()))
        GetItemEntry_Draw(play, *entry);
}

void DrawShelves(PlayState* play) {
    if (!sShelfFixture.Ensure(play))
        return;
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    Matrix_Push();
    Matrix_Translate(0, 0, 0, MTXMODE_NEW);
    gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    const auto color =
        CVarGetColor("gCosmetics.World.BlackMarketShelves.Value",
                     Color_RGBA8{ BlackMarketShelves::kDefaultTint[0], BlackMarketShelves::kDefaultTint[1],
                                  BlackMarketShelves::kDefaultTint[2], 255 });
    sShelfFixture.Draw(POLY_OPA_DISP, sGrannyActive, sGrannyResources.ShelfTextures(),
                       BlackMarketShelves::Tint(CVarGetInteger("gCosmetics.World.BlackMarketShelves.Changed", 0) != 0,
                                                { color.r, color.g, color.b, 255 }));
    Matrix_Pop();
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
}
void DrawMarket() {
    auto* play = gPlayState;
    if (!Active() || !play->state.running)
        return;
    DrawShelves(play);
    for (int slot = 0; slot < 8; ++slot) {
        const auto* item = Offer(slot);
        if (!item)
            continue;
        const bool locked = Status(slot) == PurchaseResult::Locked;
        BlackMarketVisual::Capture capture(locked);
        Matrix_Push();
        const auto pos = kShelves[slot];
        Matrix_Translate(pos.x, pos.y + 2 * std::sin((play->gameplayFrames + slot * 9) * .035f), pos.z, MTXMODE_NEW);
        Matrix_RotateY(play->gameplayFrames * .018f, MTXMODE_APPLY);
        const float scale = sTalking && slot == sSelected ? .32f : .25f;
        Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
        DrawOffer(play, *item);
        Matrix_Pop();
        if (locked && CVarGetInteger(BlackMarketVisual::kSealsCvar, 1))
            DrawSeal(play, capture.frame, sTalking && slot == sSelected);
    }
    if (sTitlePending && Safe(play)) {
        PresentTitle();
        sTitlePending = false;
    }
}
void DrawExterior() {
    auto* play = gPlayState;
    if (!Exterior(play) || !sSession.Active(gSaveContext.fileNum) || !PackAvailable())
        return;
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    Matrix_Push();
    Matrix_Translate(COMBO_BLACK_MARKET_DOOR_X, sDoorFloor, COMBO_BLACK_MARKET_DOOR_Z, MTXMODE_NEW);
    Matrix_RotateY(COMBO_BLACK_MARKET_DOOR_YAW * (6.28318530718f / 65536.0f), MTXMODE_APPLY);
    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)kExterior);
    Matrix_Pop();
    CLOSE_DISPS(play->state.gfxCtx);
}
void PlayerUpdate() {
    auto* play = gPlayState;
    if (gComboGameMode != COMBO_GAME_OOT || !Safe(play) || Player_InCsMode(play))
        return;
    auto* player = GET_PLAYER(play);
    if (!player)
        return;
    if (sReturnStaged) {
        gSaveContext.respawn[RESPAWN_MODE_RETURN] = sPreservedReturn;
        sReturnStaged = false;
    }
    if (sReturnPending && MatchesTravel() && Exterior(play)) {
        sReturnPending = false;
        Vec3f pos = sReturnPos;
        pos.y += 120;
        CollisionPoly* floor = nullptr;
        s32 bg = BGCHECK_SCENE;
        const auto y = BgCheck_EntityRaycastFloor5(play, &play->colCtx, &floor, &bg, &player->actor, &pos);
        if (y != BGCHECK_Y_MIN && floor && floor->normal.y >= 23170) {
            pos.y = y;
            player->actor.world.pos = player->actor.prevPos = player->actor.home.pos = pos;
            player->actor.world.rot.y = player->actor.shape.rot.y = player->yaw = sReturnYaw;
            player->actor.velocity = {};
            player->actor.speedXZ = 0;
            player->actor.floorHeight = y;
            player->actor.floorPoly = floor;
            player->actor.floorBgId = bg;
        }
        sDoorArmed = false;
        return;
    }
    if (Interior()) {
        // Empty custom rooms do not fire OnSceneSpawnActors in SoH. Wait for
        // the native player and object context, then create the one keeper.
        if (!sKeeperSpawned && PackAvailable())
            sKeeperSpawned =
                Actor_Spawn(&play->actorCtx, play, sKeeperActorId, BlackMarketKeeper::kX, BlackMarketKeeper::kY,
                            BlackMarketKeeper::kZ, 0, BlackMarketKeeper::kYaw, 0, 0) != nullptr;
        if (!Active() ||
            ComboBlackMarket_AtExit(player->actor.world.pos.x, player->actor.world.pos.y, player->actor.world.pos.z)) {
            RememberTravel();
            sReturnPending = true;
            Transition(play, sReturnEntrance);
        }
        return;
    }
    if (!Exterior(play) || !sSession.Active(gSaveContext.fileNum) || !PackAvailable())
        return;
    const auto pos = player->actor.world.pos;
    Vec3f probe = { COMBO_BLACK_MARKET_DOOR_X, 120, COMBO_BLACK_MARKET_DOOR_Z };
    CollisionPoly* floor = nullptr;
    s32 bg = BGCHECK_SCENE;
    const float height = BgCheck_EntityRaycastFloor5(play, &play->colCtx, &floor, &bg, &player->actor, &probe);
    if (height == BGCHECK_Y_MIN || !floor || floor->normal.y < 23170)
        return;
    sDoorFloor = height;
    const float dx = pos.x - COMBO_BLACK_MARKET_DOOR_X, dz = pos.z - COMBO_BLACK_MARKET_DOOR_Z;
    if (dx * dx + dz * dz > 14400) {
        sDoorArmed = true;
        if (player->actor.floorPoly && player->actor.floorPoly->normal.y >= 23170 &&
            std::isfinite(player->actor.floorHeight) && std::abs(pos.y - player->actor.floorHeight) < 35.0f) {
            sGroundedReturnPos = pos;
            sGroundedReturnYaw = player->actor.shape.rot.y;
            sGroundedReturnEntrance = gSaveContext.entranceIndex;
        }
    }
    if (sDoorArmed && ComboBlackMarket_InDoor(pos.x, pos.y, pos.z, height)) {
        SaveManager::Instance->ThreadPoolWait();
        sReturnPos = sGroundedReturnPos;
        sReturnYaw = sGroundedReturnYaw;
        sReturnEntrance = sGroundedReturnEntrance;
        sDoorArmed = false;
        sEntering = true;
        RememberTravel();
        Transition(play, ENTR_SKIEJER_MARKET);
    }
}
void BindSolo(int slot) {
    if (gComboGameMode != COMBO_GAME_OOT || !ComboLostWoods_IsPlayableSlot(slot))
        return;
    if (!sFileLoadPending && sSession.Active(slot) && sSession.market["seed"] == Seed())
        return;
    sFileLoadPending = false;
    sResumeFromMarket = false;
    const bool create = sPendingSave.is_null();
    auto market = sPendingSave.is_object() ? sPendingSave.value("market", nlohmann::json()) : nlohmann::json();
    if (sPendingSave.is_null()) {
        auto options = OptionsFromJson(nlohmann::json::parse(SOH_ReadBlackMarketOptions()));
        options.games = Games::Oot;
        market = Generate(Seed(), options);
    }
    if (!Validate(market) || !MatchesGames(market, Games::Oot) || market["seed"] != Seed()) {
        sSession.Bind(nullptr, -1);
        sClears = 0;
        return;
    }
    sSession.Bind(market, slot);
    sSession.Load(sPendingSave, slot);
    const auto clears = sPendingSave.is_object() ? sPendingSave.value("clears", nlohmann::json(0)) : nlohmann::json();
    sClears = UnsignedInRange(clears, 0xFF) ? clears.get<u16>() : 0;
    sReturnEntrance = ENTR_SKIEJER_FOREST;
    sReturnPos = { 184, 0, -300 };
    sReturnYaw = -16384;
    const auto point = sPendingSave.is_object() ? sPendingSave.value("return", nlohmann::json()) : nlohmann::json();
    if (point.is_object())
        try {
            const int entrance = point.at("entrance").get<int>(), yaw = point.at("yaw").get<int>();
            const Vec3f pos = { point.at("x").get<float>(), point.at("y").get<float>(), point.at("z").get<float>() };
            if (entrance >= 0 && entrance < ENTR_MAX && gEntranceTable[entrance].scene == SCENE_SKIEJER_FOREST &&
                std::isfinite(pos.x) && std::isfinite(pos.y) && std::isfinite(pos.z) && std::abs(pos.x) < 4096 &&
                std::abs(pos.y) < 4096 && std::abs(pos.z) < 4096 && yaw >= INT16_MIN && yaw <= INT16_MAX) {
                sReturnEntrance = entrance;
                sReturnPos = pos;
                sReturnYaw = yaw;
            }
        } catch (const nlohmann::json::exception&) {}
    sGroundedReturnPos = sReturnPos;
    sGroundedReturnYaw = sReturnYaw;
    sGroundedReturnEntrance = sReturnEntrance;
    sResumeFromMarket = sPendingSave.is_object() && sPendingSave.value("savedInside", nlohmann::json()) == true;
    if (sResumeFromMarket) {
        // OnLoadGame is unordered; native entrance-rando hooks may still
        // choose a savewarp. Apply our local recovery only at Play_Init.
        RememberTravel();
    }
    if (create) {
        SaveManager::Instance->ThreadPoolWait();
        SaveManager::Instance->SaveSection(slot, SECTION_ID_BASE, false);
    }
}
void SaveMarket(const SaveContext& snapshot, int, bool) {
    if (gComboGameMode != COMBO_GAME_OOT || !sSession.Active(snapshot.fileNum))
        return;
    auto value = sSession.Save(snapshot.fileNum);
    value["clears"] = sClears;
    value["savedInside"] =
        snapshot.entranceIndex >= ENTR_SKIEJER_MARKET && snapshot.entranceIndex <= ENTR_SKIEJER_MARKET_3;
    value["return"] = { { "entrance", sReturnEntrance },
                        { "x", sReturnPos.x },
                        { "y", sReturnPos.y },
                        { "z", sReturnPos.z },
                        { "yaw", sReturnYaw } };
    SaveManager::Instance->SaveData("native", value);
}
void RegisterNativeMarket() {
    RegisterKeeperActor();
    SaveManager::Instance->AddInitFunction([](bool) {
        sPendingSave = nullptr;
        sSession = {};
        sClears = 0;
        sFileLoadPending = true;
        sEntering = sReturnPending = sReturnStaged = sResumeFromMarket = false;
    });
    SaveManager::Instance->AddLoadFunction("blackMarket", 1, [] {
        SaveManager::Instance->LoadData("native", sPendingSave);
        sFileLoadPending = true;
    });
    SaveManager::Instance->AddSaveFunction("blackMarket", 1, SaveMarket, true, SECTION_PARENT_NONE);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>(BindSolo);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](s16 scene) {
        sShelfFixture.Reset();
        EndBrowsing(gPlayState, true);
        GrannySceneBegin();
        sEntering = sDoorArmed = sTalking = false;
        sKeeperSpawned = false;
        sTitlePending = Interior() && sSession.Active(gSaveContext.fileNum) && PackAvailable();
        if (scene == SCENE_SKIEJER_MARKET)
            sSelected = 0;
    });
    Ship::CrossRMRegistry::RegisterTeardownListener([] { sGrannyResources.Clear(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawBegin>(DrawGrannyRoom);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(ApplyGrannyInterior);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(PlayerUpdate);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnOpenText>(MarketText);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>(MonitorBrowsing);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawEnd>([] {
        DrawMarket();
        DrawBrowseCursor();
        DrawExterior();
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([] {
        sShelfFixture.Reset();
        EndBrowsing(gPlayState, true);
        sTitlePending = sTalking = sDoorArmed = sGrannyActive = sGrannyPending = false;
    });
}
RegisterShipInitFunc init(RegisterNativeMarket);
} // namespace

extern "C" void ComboBlackMarketOot_NormalizeEntry(void) {
    if (gComboGameMode != COMBO_GAME_OOT)
        return;
    if (sResumeFromMarket) {
        sResumeFromMarket = false;
        if (MatchesTravel() && sSession.Active(gSaveContext.fileNum)) {
            sEntering = false;
            sReturnPending = true;
            gSaveContext.entranceIndex = sReturnEntrance;
            gSaveContext.cutsceneIndex = 0;
            gSaveContext.respawnFlag = 0;
            StageReturn();
            return;
        }
    }
    if (sReturnPending && MatchesTravel() && gSaveContext.entranceIndex == sReturnEntrance) {
        StageReturn();
        return;
    }
    if (gSaveContext.entranceIndex < ENTR_SKIEJER_MARKET || gSaveContext.entranceIndex > ENTR_SKIEJER_MARKET_3)
        return;
    if (sEntering && MatchesTravel() && sSession.Active(gSaveContext.fileNum) && PackAvailable() &&
        gSaveContext.respawnFlag == 0)
        return;
    sEntering = false;
    sReturnPending = true;
    RememberTravel();
    gSaveContext.entranceIndex = sReturnEntrance;
    gSaveContext.cutsceneIndex = 0;
    gSaveContext.respawnFlag = 0;
    StageReturn();
}
extern "C" void ComboBlackMarketOot_ReportClear(int boss, int slot) {
    if (gComboGameMode != COMBO_GAME_OOT || boss < 0 || boss >= 8 || slot != gSaveContext.fileNum ||
        !sSession.Active(slot))
        return;
    const u16 next = sClears | (1u << boss);
    if (next == sClears)
        return;
    SaveManager::Instance->ThreadPoolWait();
    sClears = next;
    SaveManager::Instance->SaveSection(slot, SECTION_ID_BASE, false);
}
#endif
