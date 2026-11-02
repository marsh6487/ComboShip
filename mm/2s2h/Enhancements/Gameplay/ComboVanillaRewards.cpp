#ifdef COMBO_BUILD
#include "global.h"
#include "ComboExport.h"
#include "vanilla/ComboVanillaMode.h"
#include "vanilla/ComboVanillaRewards.h"
#include "entrances/NativeForest.h"
#include "entrances/NativeForestResourceData.h"
#include "2s2h/CustomItem/CustomItem.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/FleetShipCombo/FleetComboIds.h"
#include "2s2h/Rando/Rando.h"
#include "2s2h/SaveManager/SaveManager.h"
#include "2s2h/ShipInit.hpp"
#include "2s2h/resource/type/Scene.h"
#include "2s2h/resource/type/scenecommand/SetAlternateHeaders.h"
#include "2s2h/resource/type/scenecommand/SetEntranceList.h"
#include "2s2h/resource/type/scenecommand/SetStartPositionList.h"
#include "mods/extended_inventory.h"
#include "mods/extended_equipment.h"
#include "mods/items/custom_bottles.h"
#include "mods/items/helpers/equip_helper.h"
#include "mods/items/logic/twilight_upgrade.h"
#include "../../../../soh/soh/Enhancements/randomizer/NeiGiRender.h"
#include <array>
#include <cmath>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <spdlog/spdlog.h>

extern "C" {
u8 Cane_GiveSkill(u8 skill);
void CustomItem00_Update(Actor* actor, PlayState* play);
void CustomItem00_Destroy(Actor* actor, PlayState* play);
u8 ResourceMgr_FileExists(const char* path);
}

namespace {
using namespace ComboVanilla;
constexpr u16 kOfferText = 0x8E80;
constexpr float kAngle = 6.28318530718f / 65536.0f;
RewardHost sHost = nullptr;

struct Binding {
    const char* location;
    s16 scene;
    s8 room;
    u8 entrance;
    float right;
    float forward;
};
// These are additions beside native entrance landings. We read the active
// header's coordinates, rather than carrying stock world coordinates into an
// authored scene. The complete apron and a short approach must pass collision
// checks before anything is spawned. Unverified/changed geometry fails closed.
constexpr Binding kBindings[] = {
    { "swamp_tourist", SCENE_20SICHITAI, 0, 0, 55, 85 },
    { "swamp_tourist", SCENE_20SICHITAI2, 0, 0, 55, 85 },
    { "deku_palace_gardens", SCENE_22DEKUCITY, 0, 0, 55, 85 },
    { "woodfall_approach", SCENE_21MITURINMAE, 0, 0, -55, 45 },
    { "woodfall_return", SCENE_21MITURINMAE, 0, 0, -55, 125 },
    { "mountain_village_thawed", SCENE_10YUKIYAMANOMURA2, 0, 0, 55, 85 },
    { "pirates_fortress_exterior", SCENE_TORIDE, 0, 0, 55, 85 },
    { "milk_road", SCENE_ROMANYMAE, 0, 0, 55, 85 },
    { "clock_tower_base", SCENE_CLOCKTOWER, 0, 0, 55, 85 },
    { "snowhead_approach", SCENE_14YUKIDAMANOMITI, 0, 0, 55, 85 },
    { "ikana_castle_exit", SCENE_CASTLE, 0, 0, 55, 165 },
    { "swamp_potion_shop", SCENE_WITCH_SHOP, 0, 0, 45, 65 },
    { "ikana_castle_courtyard", SCENE_CASTLE, 0, 0, 55, 85 },
    { "great_bay_return", SCENE_30GYOSON, 0, 0, -55, 85 },
    { "ikana_graveyard", SCENE_BOTI, 0, 0, 55, 85 },
    { "curiosity_shop", SCENE_AYASHIISHOP, 0, 0, 45, 65 },
    { "stone_tower_approach", SCENE_F40, 0, 0, 55, 85 },
    { "mountain_smithy", SCENE_10YUKIYAMANOMURA2, 0, 1, 55, 85 },
    { "romani_ranch", SCENE_F01, 0, 0, 55, 85 },
    { "great_bay_coast", SCENE_30GYOSON, 0, 0, 55, 85 },
    { "ikana_canyon", SCENE_IKANA, 0, 0, 55, 85 },
    { "zora_cape", SCENE_31MISAKI, 0, 0, 55, 85 },
    { "mirror_source", SCENE_20SICHITAI, 0, 0, -55, 85 },
    { "mirror_source", SCENE_20SICHITAI2, 0, 0, -55, 85 },
};

bool NativePrerequisite(std::string_view name) {
    if (name == "first_cycle")
        return true;
    if (name == "deku")
        return INV_CONTENT(ITEM_MASK_DEKU) == ITEM_MASK_DEKU;
    if (name == "goron")
        return INV_CONTENT(ITEM_MASK_GORON) == ITEM_MASK_GORON;
    if (name == "zora")
        return INV_CONTENT(ITEM_MASK_ZORA) == ITEM_MASK_ZORA;
    if (name == "zora_hookshot")
        return INV_CONTENT(ITEM_MASK_ZORA) == ITEM_MASK_ZORA && INV_CONTENT(ITEM_HOOKSHOT) == ITEM_HOOKSHOT;
    if (name == "epona")
        return CHECK_QUEST_ITEM(QUEST_SONG_EPONA);
    if (name == "odolwa")
        return CHECK_QUEST_ITEM(QUEST_REMAINS_ODOLWA);
    if (name == "goht")
        return CHECK_QUEST_ITEM(QUEST_REMAINS_GOHT);
    if (name == "gyorg")
        return CHECK_QUEST_ITEM(QUEST_REMAINS_GYORG);
    if (name == "twinmold")
        return CHECK_QUEST_ITEM(QUEST_REMAINS_TWINMOLD);
    if (name == "elegy")
        return CHECK_QUEST_ITEM(QUEST_SONG_ELEGY);
    if (name == "elegy_hookshot")
        return CHECK_QUEST_ITEM(QUEST_SONG_ELEGY) && INV_CONTENT(ITEM_HOOKSHOT) == ITEM_HOOKSHOT;
    if (name == "ikana")
        return INV_CONTENT(ITEM_MASK_GIBDO) == ITEM_MASK_GIBDO && INV_CONTENT(ITEM_HOOKSHOT) == ITEM_HOOKSHOT;
    if (name == "captains_hat")
        return INV_CONTENT(ITEM_MASK_CAPTAIN) == ITEM_MASK_CAPTAIN;
    if (name == "koume")
        return CHECK_WEEKEVENTREG(WEEKEVENTREG_SAVED_KOUME);
    if (name == "night_shop")
        return gSaveContext.save.isNight != 0;
    return false;
}

RandoItemId DrawIdentity(const Reward& reward) {
    switch (reward.kind) {
        case RewardKind::Item:
            switch (reward.payload) {
                case 0x9F:
                    return RI_OOT_NEI_ROCS_CAPE;
                case 0xA2:
                    return RI_OOT_NEI_ZONAI_PERMAFROST;
                case 0xA3:
                    return RI_OOT_NEI_DEMISE_DESTRUCTION;
                case 0xA5:
                    return RI_OOT_NEI_SWITCH_HOOK;
                case 0xA7:
                    return RI_OOT_NEI_GUST_JAR;
                case 0xA8:
                    return RI_OOT_NEI_BALL_AND_CHAIN;
                case 0xA9:
                    return RI_OOT_NEI_WHIP;
                case 0xAC:
                    return RI_OOT_NEI_DOMINION_ROD;
                case 0xB0:
                    return RI_OOT_NEI_ICE_ROD;
                case 0xB7:
                    return RI_OOT_NEI_POKE_BALL;
                case 0x221:
                    return RI_OOT_NEI_PHANTOM_HOURGLASS;
                default:
                    return RI_UNKNOWN;
            }
        case RewardKind::Cane:
            switch (reward.payload) {
                case 1:
                    return RI_OOT_NEI_CANE_SOMARIA_BLOCK;
                case 3:
                    return RI_OOT_NEI_CANE_PACCI_FLIP;
                case 5:
                    return RI_OOT_NEI_CANE_PACCI_ULTRAHAND;
                default:
                    return RI_UNKNOWN;
            }
        case RewardKind::Wand:
            switch (reward.payload) {
                case 1:
                    return RI_OOT_NEI_WAND_TORNADO_ROD;
                case 4:
                    return RI_OOT_NEI_WAND_STORM_ROD;
                case 5:
                    return RI_OOT_NEI_WAND_SHADOW_SCEPTER;
                default:
                    return RI_UNKNOWN;
            }
        case RewardKind::Rune:
            if (reward.payload == 1)
                return RI_OOT_NEI_SLATE_RUNE_STASIS;
            if (reward.payload == 3)
                return RI_OOT_NEI_SLATE_RUNE_MASTER_CYCLE;
            return RI_UNKNOWN;
        case RewardKind::Season:
            if (reward.payload == 1)
                return RI_OOT_NEI_SEASON_SUMMER;
            if (reward.payload == 2)
                return RI_OOT_NEI_SEASON_AUTUMN;
            return RI_UNKNOWN;
        case RewardKind::Equipment:
            switch (reward.payload) {
                case 0x001:
                    return RI_OOT_EXT_CANE_OF_BYRNA;
                case 0x003:
                    return RI_OOT_EXT_TRIDENT;
                case 0x201:
                    return RI_OOT_EXT_CHAMPIONS_TUNIC;
                case 0x302:
                    return RI_OOT_EXT_CLIMB_BOOTS;
                default:
                    return RI_UNKNOWN;
            }
        case RewardKind::Upgrade:
            switch (static_cast<Upgrade>(reward.payload)) {
                case Upgrade::Net:
                    return RI_NET;
                case Upgrade::BottomlessBottle:
                    return RI_BOTTOMLESS_BOTTLE;
                case Upgrade::MagicCape:
                    return RI_OOT_EXT_MAGIC_CAPE;
                default:
                    return RI_UNKNOWN;
            }
        case RewardKind::Retained:
            if (reward.payload == 0x224)
                return RI_OOT_NEI_MAGIC_MIRROR;
            return RI_UNKNOWN;
        default:
            return RI_UNKNOWN;
    }
}

bool HasPresentation(const Reward& reward) {
    return DrawIdentity(reward) != RI_UNKNOWN ||
           (reward.kind == RewardKind::Upgrade && reward.payload == static_cast<u16>(Upgrade::Twilight));
}
void DrawReward(const Reward& reward, Actor* actor, PlayState* play) {
    if (reward.kind == RewardKind::Upgrade && reward.payload == static_cast<u16>(Upgrade::Twilight)) {
        // The upgrade has no standalone item model. Its own dark-energy orb is
        // a presentation of this power, rather than a mislabeled native GI.
        const auto camera = NeiGi_CameraBasis(play);
        NeiGi_DrawMesh(play, NeiGi::SampleOrb(NeiGi::Kind::Shadow, camera), NeiGi::Kind::Shadow);
        NeiGi_DrawMesh(play, NeiGi::SampleEnergy(NeiGi::Kind::Shadow, play->gameplayFrames, camera));
    } else {
        Rando::DrawResolvedItem(DrawIdentity(reward), RC_UNKNOWN, actor);
    }
}

int ItemSlot(u16 canonical) {
    switch (canonical) {
        case 0x9F:
            return SLOT_ROCS;
        case 0xA2:
            return SLOT_ZONAI_PERMAFROST;
        case 0xA3:
            return SLOT_DEMISE_DESTRUCTION;
        case 0xA5:
            return SLOT_SWITCH_HOOK;
        case 0xA7:
            return SLOT_GUST_JAR;
        case 0xA8:
            return SLOT_BALL_AND_CHAIN;
        case 0xA9:
            return SLOT_WHIP;
        case 0xAC:
            return SLOT_SHOVEL;
        case 0xB0:
            return SLOT_ICE_ROD;
        case 0x221:
            return SLOT_PHANTOM_HOURGLASS;
        case 0x224:
            return SLOT_MAGIC_MIRROR;
        default:
            return -1;
    }
}

bool Owns(const Reward& reward) {
    const auto* nei = Nei_Save();
    switch (reward.kind) {
        case RewardKind::Item:
        case RewardKind::Retained:
            if (reward.payload == 0xB7)
                return nei->pokeballOwned;
            if (reward.payload == 0xAC)
                return nei->dominionOwned;
            if (reward.payload == 0x221)
                return nei->phantomHourglassOwned;
            if (reward.payload == 0x224)
                return nei->magicMirrorOwned;
            return ItemSlot(reward.payload) >= 0 &&
                   Nei_GetOwnedItem(ItemSlot(reward.payload)) == FcEquip_OotToMm16(reward.payload);
        case RewardKind::Cane:
            return (nei->caneSkills & (1u << reward.payload)) != 0;
        case RewardKind::Wand:
            return (nei->wandRodsOwned & (1u << reward.payload)) != 0;
        case RewardKind::Rune:
            return (nei->slateRunesOwned & (1u << reward.payload)) != 0;
        case RewardKind::Season:
            return (nei->seasonsOwned & (1u << reward.payload)) != 0;
        case RewardKind::Equipment:
            return ExtEquip_HasItem(reward.payload >> 8, reward.payload & 255);
        case RewardKind::Upgrade:
            switch (static_cast<Upgrade>(reward.payload)) {
                case Upgrade::Net:
                    return Bottle_NetOwned();
                case Upgrade::BottomlessBottle:
                    return Bottle_BottomlessOwned();
                case Upgrade::MagicCape:
                    return ExtEquip_CapeOwned();
                case Upgrade::Twilight:
                    return TwilightUpgrade_IsFullyObtained();
                default:
                    return false;
            }
        default:
            return false;
    }
}

bool Grant(const Reward& reward) {
    if (Owns(reward))
        return true;
    switch (reward.kind) {
        case RewardKind::Item:
        case RewardKind::Retained: {
            if (reward.payload == 0xB7) {
                Nei_Save()->pokeballOwned = 1;
                break;
            }
            if (reward.payload == 0xAC) {
                Nei_Save()->dominionOwned = 1;
                if (Nei_GetOwnedItem(SLOT_SHOVEL) == ITEM_NONE)
                    ExtInv_GiveItem(SLOT_SHOVEL, ITEM_DOMINION_ROD);
                break;
            }
            const int slot = ItemSlot(reward.payload);
            const u16 item = FcEquip_OotToMm16(reward.payload);
            if (slot < 0 || item == ITEM_NONE)
                return false;
            ExtInv_GiveItem(slot, item);
            break;
        }
        case RewardKind::Cane:
            Cane_GiveSkill(reward.payload);
            break;
        case RewardKind::Wand:
            Wand_GrantMode(reward.payload);
            break;
        case RewardKind::Rune:
            Slate_GrantRune(reward.payload);
            break;
        case RewardKind::Season:
            Seasons_GrantSeason(reward.payload);
            break;
        case RewardKind::Equipment:
            ExtEquip_GiveItem(reward.payload >> 8, reward.payload & 255);
            break;
        case RewardKind::Upgrade:
            switch (static_cast<Upgrade>(reward.payload)) {
                case Upgrade::Net:
                    Bottle_SetNetOwned(1);
                    break;
                case Upgrade::BottomlessBottle:
                    Bottle_SetBottomlessOwned(1);
                    Bottle_BottomlessEmpty();
                    break;
                case Upgrade::MagicCape:
                    ExtEquip_GiveCape();
                    break;
                case Upgrade::Twilight:
                    TwilightUpgrade_Grant();
                    break;
                default:
                    return false;
            }
            break;
        default:
            return false;
    }
    return Owns(reward);
}

// A resource-specific native source is retained whenever the Combo forest is
// present. A stock fallback uses the same collection ID, never another copy.
bool AuthoredMirrorSourceAvailable() {
    return NativeForest::SceneAvailable(NativeForest::kCombinedScene, ResourceMgr_FileExists, ResourceMgr_FileExists,
                                        NativeForest::LoadDependencies);
}

bool Active() {
    return Combo_IsVanilla() && gSaveContext.fileNum <= 2 &&
           gSaveContext.save.saveInfo.playerData.threeDayResetCount > 0;
}
bool Safe(PlayState* play, bool ownMessage = false) {
    if (!Active() || !play || gGameState != &play->state || !play->state.running || !GET_PLAYER(play) ||
        gSaveContext.gameMode != GAMEMODE_NORMAL || play->transitionTrigger != TRANS_TRIGGER_OFF ||
        play->transitionMode != TRANS_MODE_OFF || play->pauseCtx.state != 0 ||
        play->gameOverCtx.state != GAMEOVER_INACTIVE || play->csCtx.state != CS_STATE_IDLE ||
        gSaveContext.cutsceneTrigger || play->soaringCsOrSoTCsPlaying)
        return false;
    const auto* player = GET_PLAYER(play);
    if (!(player->actor.bgCheckFlags & BGCHECKFLAG_GROUND) || player->stateFlags1 & PLAYER_STATE1_DEAD)
        return false;
    if (!ownMessage && (play->msgCtx.msgMode != MSGMODE_NONE || ItemInput_IsBlocked(GET_PLAYER(play), play)))
        return false;
    for (const auto state : gSaveContext.timerStates)
        if (state != TIMER_STATE_OFF)
            return false;
    return true;
}

bool ReadNativeAnchors(SOH::Scene* scene, const SOH::SetEntranceList*& entrances,
                       const SOH::SetStartPositionList*& starts, unsigned depth = 0) {
    if (!scene || depth >= 8)
        return false;
    // Execute the same list selection as the native importer. A selected
    // alternate inherits earlier base commands and ends the remaining base.
    for (const auto& command : scene->commands) {
        if (!command)
            return false;
        if (command->cmdId == SOH::SceneCommandID::EndMarker)
            break;
        if (command->cmdId == SOH::SceneCommandID::SetEntranceList)
            entrances = static_cast<SOH::SetEntranceList*>(command.get());
        if (command->cmdId == SOH::SceneCommandID::SetStartPositionList)
            starts = static_cast<SOH::SetStartPositionList*>(command.get());
        if (command->cmdId == SOH::SceneCommandID::SetAlternateHeaders && gSaveContext.sceneLayer > 0) {
            const auto* list = static_cast<SOH::SetAlternateHeaders*>(command.get());
            const unsigned index = gSaveContext.sceneLayer - 1;
            if (index >= list->headers.size())
                return false;
            if (list->headers[index])
                return ReadNativeAnchors(list->headers[index].get(), entrances, starts, depth + 1);
        }
    }
    return true;
}

bool NativeAnchor(PlayState* play, const Binding& binding, ActorEntry& anchor) {
    const SOH::SetEntranceList* entrances = nullptr;
    const SOH::SetStartPositionList* starts = nullptr;
    if (!ReadNativeAnchors(static_cast<SOH::Scene*>(play->sceneSegment), entrances, starts))
        return false;
    if (!entrances || !starts || binding.entrance >= entrances->entrances.size())
        return false;
    const auto native = entrances->entrances[binding.entrance];
    if (native.room != binding.room || native.spawn >= starts->startPositions.size())
        return false;
    const auto& source = starts->startPositions[native.spawn];
    anchor = ActorEntry{ source.id, source.pos, source.rot, source.params };
    return PLAYER_GET_START_MODE(&anchor) != PLAYER_START_MODE_TELESCOPE;
}

bool DryFloor(PlayState* play, Vec3f point, float expected, float& height) {
    point.y = expected + 90;
    CollisionPoly* floor = nullptr;
    s32 bg = BGCHECK_SCENE;
    height = BgCheck_EntityRaycastFloor5_3(play, &play->colCtx, &floor, &bg, &GET_PLAYER(play)->actor, &point);
    if (!floor || bg != BGCHECK_SCENE || !std::isfinite(height) || height == BGCHECK_Y_MIN ||
        std::abs(height - expected) > 35 || floor->normal.y < 30000 ||
        SurfaceType_GetSceneExitIndex(&play->colCtx, floor, bg) != 0)
        return false;
    WaterBox* water = nullptr;
    float surface = 0;
    return !WaterBox_GetSurface1_2(play, &play->colCtx, point.x, point.z, &surface, &water) || surface < height - 0.5f;
}

bool ClearApproach(PlayState* play, Vec3f start, Vec3f end) {
    CollisionPoly* wall = nullptr;
    Vec3f hit{};
    s32 bg = BGCHECK_SCENE;
    for (float elevation : { 20.0f, 70.0f }) {
        Vec3f from{ start.x, start.y + elevation, start.z }, to{ end.x, end.y + elevation, end.z };
        if (BgCheck_EntityLineTest1(&play->colCtx, &from, &to, &hit, &wall, true, true, true, true, &bg))
            return false;
    }
    return true;
}

bool Placement(PlayState* play, const Binding& binding, unsigned ordinal, Vec3f& point) {
    ActorEntry entry{};
    if (!NativeAnchor(play, binding, entry))
        return false;
    // Same named cache family, distinct adjacent physical checks. Slots do not
    // move when another reward is collected, nor depend on a player's entrance.
    const float right = binding.right + static_cast<float>(ordinal) * 80;
    // Scene entries pack whole degrees above a seven-bit actor cutscene ID.
    // Match Actor_SpawnEntry, including its explicit degree-mode actor flag.
    s16 yaw = (entry.rot.y >> 7) & 0x1FF;
    if (!(entry.id & 0x8000))
        yaw = DEG_TO_BINANG(yaw);
    else if (yaw > 180)
        yaw -= 360;
    const float sine = std::sin(yaw * kAngle), cosine = std::cos(yaw * kAngle);
    point = { entry.pos.x + right * cosine + binding.forward * sine, static_cast<float>(entry.pos.y),
              entry.pos.z - right * sine + binding.forward * cosine };
    float height;
    if (!DryFloor(play, point, entry.pos.y, height))
        return false;
    point.y = height;
    for (const auto& edge :
         std::array<Vec3f, 4>{ Vec3f{ -22, 0, 0 }, Vec3f{ 22, 0, 0 }, Vec3f{ 0, 0, -22 }, Vec3f{ 0, 0, 22 } }) {
        Vec3f sample{ point.x + edge.x, point.y, point.z + edge.z };
        float support;
        if (!DryFloor(play, sample, point.y, support) || std::abs(support - point.y) > 5 ||
            !ClearApproach(play, point, sample))
            return false;
    }
    const Vec3f native{ static_cast<float>(entry.pos.x), static_cast<float>(entry.pos.y),
                        static_cast<float>(entry.pos.z) };
    for (unsigned i = 1; i <= 5; ++i) {
        const float fraction = i / 5.0f;
        Vec3f sample{ native.x + (point.x - native.x) * fraction, native.y,
                      native.z + (point.z - native.z) * fraction };
        float support;
        if (!DryFloor(play, sample, native.y, support) || !ClearApproach(play, native, point))
            return false;
    }
    return true;
}

struct Context {
    PlayState* play = nullptr;
    int slot = -1;
    int scene = -1;
    int room = -1;
    unsigned generation = 0;
} sContext;
struct Pickup {
    unsigned reward;
    unsigned generation;
    bool talking = false;
    bool accepted = false;
};
std::unordered_map<Actor*, Pickup> sPickups;
std::unordered_set<unsigned> sAttempted;
bool sMirrorBackfillAttempted = false;
struct ReceiptState {
    unsigned reward = 0;
    int slot = -1;
    s16 token = 0;
    unsigned generation = 0;
    Actor* actor = nullptr;
    bool finished = false;
} sReceipt;
unsigned sSerial = 0;
Actor* sTalking = nullptr;

bool ContextMatches(PlayState* play) {
    return play && sContext.play == play && sContext.slot == gSaveContext.fileNum && sContext.scene == play->sceneId &&
           sContext.room == play->roomCtx.curRoom.num;
}
int Host(RewardAction action, const Reward& reward, int slot) {
    return sHost ? sHost(static_cast<int>(action), static_cast<int>(Game::Mm), slot, reward.checkId, nullptr) : -1;
}
bool Transact(const Reward& reward, int slot) {
    if (!Active() || slot != gSaveContext.fileNum)
        return false;
    const int status = Host(RewardAction::Query, reward, slot);
    if (status == 1)
        return Owns(reward);
    if (status != 0)
        return false;
    const bool alreadyOwned = Owns(reward);
    const unsigned price = alreadyOwned ? 0 : reward.price;
    if (gSaveContext.save.saveInfo.playerData.rupees < price)
        return false;
    auto backup = std::make_unique<SaveContext>(gSaveContext);
    if (Host(alreadyOwned ? RewardAction::BeginOwned : RewardAction::Begin, reward, slot) != 1)
        return false;
    bool success = false;
    try {
        if (alreadyOwned || Grant(reward)) {
            gSaveContext.save.saveInfo.playerData.rupees -= price;
            if (Host(RewardAction::PrepareCapture, reward, slot) == 1) {
                SaveManager_SaveCurrentForCombo(true);
                success = Host(RewardAction::Commit, reward, slot) == 1;
            }
        }
    } catch (const std::exception& error) {
        SPDLOG_ERROR("[Combo Vanilla] MM receipt {}: {}", reward.checkId, error.what());
    }
    if (!success) {
        gSaveContext = *backup;
        ExtEquip_ResyncFromSave();
        Host(RewardAction::Abort, reward, slot);
    }
    return success;
}
void GiveReceipt(Actor* actor, PlayState* play);
void DrawReceipt(Actor* actor, PlayState* play);
void ReleaseReceiptMarker(s16 token) {
    auto& current = GameInteractor::Instance->currentEvent;
    const auto* event = std::get_if<GIEventGiveItem>(&current);
    if (event && event->giveItem == GiveReceipt && event->param == token)
        current = GIEventNone{};
}
void CancelReceipt() {
    if (sReceipt.token) {
        if (!sReceipt.finished)
            Host(RewardAction::Abort, kRewards[sReceipt.reward], sReceipt.slot);
        ReleaseReceiptMarker(sReceipt.token);
    }
    sReceipt = {};
}
void Reset() {
    CancelReceipt();
    sPickups.clear();
    sAttempted.clear();
    sMirrorBackfillAttempted = false;
    sTalking = nullptr;
    const unsigned generation = sContext.generation + 1;
    sContext = {};
    sContext.generation = generation;
}

void ReceiptDestroyed(Actor* actor, PlayState* play) {
    if (actor == sReceipt.actor)
        CancelReceipt();
    CustomItem00_Destroy(actor, play);
}
bool ReceiptMatches(Actor* actor, PlayState* play) {
    return Active() && ContextMatches(play) && actor == sReceipt.actor && sReceipt.token &&
           CUSTOM_ITEM_PARAM == sReceipt.token && sReceipt.slot == gSaveContext.fileNum &&
           sReceipt.generation == sContext.generation;
}
void GiveReceipt(Actor* actor, PlayState* play) {
    if (!ReceiptMatches(actor, play) || sReceipt.finished)
        return;
    const auto& reward = kRewards[sReceipt.reward];
    const bool success = Transact(reward, sReceipt.slot);
    CustomMessage::Entry message;
    message.textboxType = TEXTBOX_TYPE_2;
    message.msg = success ? std::string("You found the %g") + reward.label + "%w!"
                          : "The reward could not be saved.\nPlease try again.";
    if (CUSTOM_ITEM_FLAGS & CustomItem::GIVE_ITEM_CUTSCENE)
        CustomMessage::SetActiveCustomMessage(message.msg, message);
    else
        CustomMessage::StartTextbox(message.msg, message);
    sReceipt.finished = true;
}
void DrawReceipt(Actor* actor, PlayState* play) {
    if (!ReceiptMatches(actor, play))
        return;
    Matrix_Scale(30.0f, 30.0f, 30.0f, MTXMODE_APPLY);
    DrawReward(kRewards[sReceipt.reward], actor, play);
}
bool OfferReceipt(unsigned index, PlayState* play) {
    if (sReceipt.token || !Safe(play) || !GameInteractor::Instance->events.empty() ||
        !std::holds_alternative<GIEventNone>(GameInteractor::Instance->currentEvent))
        return false;
    const auto& reward = kRewards[index];
    if (Host(RewardAction::Query, reward, gSaveContext.fileNum) != 0)
        return false;
    const s16 token = static_cast<s16>(sSerial = sSerial % 32766 + 1);
    auto* player = GET_PLAYER(play);
    const s16 flags = CustomItem::HIDE_TILL_OVERHEAD | CustomItem::KEEP_ON_PLAYER | CustomItem::GIVE_ITEM_CUTSCENE;
    auto* actor = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_ITEM00, player->actor.world.pos.x,
                              player->actor.world.pos.y, player->actor.world.pos.z, flags, 0, token, ITEM00_NOTHING);
    if (!actor)
        return false;
    reinterpret_cast<EnItem00*>(actor)->actionFunc = reinterpret_cast<EnItem00ActionFunc>(GiveReceipt);
    actor->draw = DrawReceipt;
    actor->destroy = ReceiptDestroyed;
    sReceipt = { index, gSaveContext.fileNum, token, sContext.generation, actor };
    GameInteractor::Instance->currentEvent = GIEventGiveItem{ true, token, GiveReceipt, DrawReceipt };
    return true;
}

void PickupDestroyed(Actor* actor, PlayState* play) {
    const auto found = sPickups.find(actor);
    if (found != sPickups.end() && sTalking == actor && ContextMatches(play) &&
        play->msgCtx.currentTextId == kOfferText + found->second.reward)
        Message_CloseTextbox(play);
    sPickups.erase(actor);
    if (sTalking == actor)
        sTalking = nullptr;
    CustomItem00_Destroy(actor, play);
}
void PickupDraw(Actor* actor, PlayState* play) {
    const auto found = sPickups.find(actor);
    if (found == sPickups.end() || !ContextMatches(play) || found->second.generation != sContext.generation)
        return;
    const auto& reward = kRewards[found->second.reward];
    Matrix_Scale(30.0f, 30.0f, 30.0f, MTXMODE_APPLY);
    DrawReward(reward, actor, play);
}
void PickupUpdate(Actor* actor, PlayState* play) {
    const auto found = sPickups.find(actor);
    if (found == sPickups.end() || !Active() || !ContextMatches(play) ||
        found->second.generation != sContext.generation) {
        Actor_Kill(actor);
        return;
    }
    auto& state = found->second;
    const auto& reward = kRewards[state.reward];
    if (Host(RewardAction::Query, reward, gSaveContext.fileNum) == 1) {
        Actor_Kill(actor);
        return;
    }
    actor->shape.rot.y += 600;
    actor->shape.yOffset = 1400;
    Actor_SetFocus(actor, 35);
    if (state.talking) {
        const auto message = Message_GetState(&play->msgCtx);
        if (message != TEXT_STATE_NONE && play->msgCtx.currentTextId != kOfferText + state.reward &&
            play->msgCtx.currentTextId != 0) {
            state.talking = state.accepted = false;
            if (sTalking == actor)
                sTalking = nullptr;
            return;
        }
        if (message == TEXT_STATE_CHOICE && Message_ShouldAdvance(play)) {
            state.accepted = play->msgCtx.choiceIndex == 0;
            Message_CloseTextbox(play);
        } else if (message == TEXT_STATE_EVENT && Message_ShouldAdvance(play)) {
            Message_CloseTextbox(play);
        } else if (message == TEXT_STATE_NONE) {
            state.talking = false;
            if (sTalking == actor)
                sTalking = nullptr;
        }
        return;
    }
    if (state.accepted) {
        if (!NativePrerequisite(reward.prerequisite)) {
            state.accepted = false;
        } else if (Safe(play) && !sTalking && !sReceipt.token) {
            if (!Owns(reward) && gSaveContext.save.saveInfo.playerData.rupees < reward.price) {
                state.accepted = false;
                CustomMessage::StartTextbox("You need more rupees.");
            } else if (OfferReceipt(state.reward, play)) {
                state.accepted = false;
            }
        }
        return;
    }
    actor->textId = kOfferText + state.reward;
    // The player opens this textbox and sets TALKING before the actor consumes
    // ACTOR_FLAG_TALK. Acceptance therefore precedes the idle offer guard.
    if (Actor_TalkOfferAccepted(actor, &play->state)) {
        if (!sTalking && !sReceipt.token && play->msgCtx.currentTextId == actor->textId &&
            play->msgCtx.talkActor == actor) {
            state.talking = true;
            state.accepted = false;
            sTalking = actor;
        }
        return;
    }
    if (sTalking || sReceipt.token || !Safe(play) || !NativePrerequisite(reward.prerequisite))
        return;
    Actor_OfferTalk(actor, play, 60);
}
void OpenText(u16* text, bool* fromTable) {
    if (!text || !fromTable || *text < kOfferText || *text >= kOfferText + std::size(kRewards) || !Active() ||
        !ContextMatches(gPlayState))
        return;
    const unsigned index = *text - kOfferText;
    const bool exists = std::any_of(sPickups.begin(), sPickups.end(), [index](const auto& entry) {
        return entry.second.reward == index && entry.second.generation == sContext.generation;
    });
    if (!exists)
        return;
    const auto& reward = kRewards[index];
    CustomMessage::Entry message;
    const unsigned price = Owns(reward) ? 0 : reward.price;
    message.msg = (price ? "Buy the %g" : "Take the %g") + std::string(reward.label) + "%w";
    if (price)
        message.msg += " for %r" + std::to_string(price) + " rupees%w";
    message.msg += "?\002\021\302Yes\021No";
    CustomMessage::LoadCustomMessageIntoFont(message);
    *fromTable = false;
}

void Update() {
    auto* play = gPlayState;
    if (!Active() || !play || !sHost || gGameState != &play->state || !play->state.running)
        return;
    if (!ContextMatches(play)) {
        Reset();
        sContext.play = play;
        sContext.slot = gSaveContext.fileNum;
        sContext.scene = play->sceneId;
        sContext.room = play->roomCtx.curRoom.num;
    }
    if (!Safe(play) || sReceipt.token)
        return;
    if (!sMirrorBackfillAttempted) {
        sMirrorBackfillAttempted = true;
        const auto* mirror = FindReward("vanilla.nei.magic_mirror");
        if (mirror && MagicMirror_IsOwned() && Host(RewardAction::Query, *mirror, gSaveContext.fileNum) == 0)
            Transact(*mirror, gSaveContext.fileNum);
    }
    for (unsigned index = 0; index < std::size(kRewards); ++index) {
        const auto& reward = kRewards[index];
        if (reward.game != Game::Mm || sAttempted.contains(index) || !NativePrerequisite(reward.prerequisite))
            continue;
        // A missing exact rendering/grant identity is deliberately unavailable;
        // it must be repaired/audited, never silently replaced with another item.
        if (!HasPresentation(reward))
            continue;
        for (const auto& binding : kBindings) {
            if (reward.location != std::string_view(binding.location) || binding.scene != play->sceneId ||
                binding.room != play->roomCtx.curRoom.num)
                continue;
            if (reward.kind == RewardKind::Retained && AuthoredMirrorSourceAvailable())
                continue;
            if (Host(RewardAction::Query, reward, gSaveContext.fileNum) != 0)
                continue;
            unsigned ordinal = 0;
            for (unsigned previous = 0; previous < index; ++previous)
                if (kRewards[previous].game == Game::Mm &&
                    std::string_view(kRewards[previous].location) == reward.location)
                    ++ordinal;
            Vec3f position{};
            sAttempted.insert(index);
            if (!Placement(play, binding, ordinal, position)) {
                SPDLOG_WARN("[Combo Vanilla] {} has no validated native landing in scene {} room {}", reward.checkId,
                            play->sceneId, play->roomCtx.curRoom.num);
                continue;
            }
            auto* actor = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_ITEM00, position.x, position.y, position.z,
                                      CustomItem::STOP_BOBBING, 0, static_cast<s16>(index), ITEM00_NOTHING);
            if (!actor)
                continue;
            actor->update = PickupUpdate;
            actor->draw = PickupDraw;
            actor->destroy = PickupDestroyed;
            actor->flags |= ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_FRIENDLY | ACTOR_FLAG_LOCK_ON_DISABLED;
            actor->textId = kOfferText + index;
            sPickups.emplace(actor, Pickup{ index, sContext.generation });
        }
    }
}
void Register() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameStateMainStart>(Update);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnOpenText>(OpenText);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](s8, s8) { Reset(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>(Reset);
}
RegisterShipInitFunc init(Register, {});
} // namespace

extern "C" COMBO_EXPORT void MM_SetComboVanillaRewardCallback(ComboVanilla::RewardHost callback) {
    sHost = callback;
}

extern "C" int MM_VanillaMirrorUsesAuthoredSource(void) {
    return AuthoredMirrorSourceAvailable();
}

extern "C" int MM_VanillaClaimMirror(void) {
    const auto* reward = ComboVanilla::FindReward("vanilla.nei.magic_mirror");
    return reward && Transact(*reward, gSaveContext.fileNum);
}
#endif
