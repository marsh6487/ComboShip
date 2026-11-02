#ifdef COMBO_BUILD
#include "ComboExport.h"
#include "vanilla/ComboVanillaMode.h"
#include "vanilla/ComboVanillaRewards.h"
#include "soh/ActorDB.h"
#include "soh/SaveManager.h"
#include "soh/ShipInit.hpp"
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/randomizer/static_data.h"
#include <array>
#include <cmath>
#include <string_view>
#include <spdlog/spdlog.h>
extern "C" {
#include "global.h"
#include "mods/nei_save.h"
#include "mods/extended_inventory.h"
#include "mods/extended_equipment.h"
#include "mods/items/logic/weapon_upgrades.h"
#include "mods/items/logic/twilight_upgrade.h"
#include "mods/transformation_masks/custom_forms.h"
#include "soh/Enhancements/randomizer/RocsFeatherCycle.h"
u8 Cane_GiveSkill(u8 skill);
extern PlayState* gPlayState;
}

namespace {
using namespace ComboVanilla;

// BEGIN TESTED NATIVE REWARD HELPERS
struct NativeRewardBinding {
    int scene = -1;
    int entrance = -1;
    int alternateEntrance = -1;
    int actor = -1;
};

// Entrance anchors are taken from the loaded native scene, after the native
// arrival finishes. Return entrances are repeatable; no cutscene-only position
// or unverified rupee/chest identity is used for a supplementary reward.
static NativeRewardBinding NativeBinding(std::string_view location) {
    if (location == "market_bazaar")
        return { SCENE_BAZAAR, ENTR_BAZAAR_0 };
    if (location == "kakariko_bazaar")
        return { SCENE_BAZAAR, ENTR_BAZAAR_1 };
    if (location == "kokiri_shop")
        return { SCENE_KOKIRI_SHOP, ENTR_KOKIRI_SHOP_0 };
    if (location == "graveyard_dampe")
        return { SCENE_GRAVEYARD, ENTR_GRAVEYARD_OUTSIDE_DAMPES_HUT };
    // Leaf's fixed source is the south-entry ground approach before the maze.
    // A live Saria actor would silently add a Zelda's Letter gate. Four Sword
    // uses the separate native temple-return variants in Anchor below.
    if (location == "sacred_forest_meadow")
        return { SCENE_SACRED_FOREST_MEADOW, ENTR_SACRED_FOREST_MEADOW_SOUTH_EXIT };
    if (location == "goron_city")
        return { SCENE_GORON_CITY, ENTR_GORON_CITY_UPPER_EXIT };
    if (location == "gerudo_fortress")
        return { SCENE_GERUDOS_FORTRESS, ENTR_GERUDOS_FORTRESS_EAST_EXIT };
    if (location == "gerudo_training_entry")
        return { SCENE_GERUDO_TRAINING_GROUND, ENTR_GERUDO_TRAINING_GROUND_ENTRANCE };
    if (location == "links_house")
        return { SCENE_LINKS_HOUSE, ENTR_LINKS_HOUSE_CHILD_SPAWN };
    if (location == "lake_fishing_approach")
        return { SCENE_LAKE_HYLIA, ENTR_LAKE_HYLIA_OUTSIDE_FISHING_POND };
    if (location == "death_mountain_fairy")
        return { SCENE_DEATH_MOUNTAIN_TRAIL, ENTR_DEATH_MOUNTAIN_TRAIL_GREAT_FAIRY_EXIT };
    if (location == "kokiri_forest")
        return { SCENE_KOKIRI_FOREST, ENTR_KOKIRI_FOREST_OUTSIDE_LINKS_HOUSE };
    if (location == "graveyard_royal_tomb")
        return { SCENE_GRAVEYARD, ENTR_GRAVEYARD_ROYAL_TOMB_EXIT };
    if (location == "temple_of_time")
        return { SCENE_TEMPLE_OF_TIME, -1, -1, ACTOR_BG_TOKI_SWD };
    if (location == "dodongo_return")
        return { SCENE_DEATH_MOUNTAIN_TRAIL, ENTR_DEATH_MOUNTAIN_TRAIL_OUTSIDE_DODONGOS_CAVERN,
                 ENTR_DEATH_MOUNTAIN_TRAIL_DODONGO_BLUE_WARP };
    if (location == "desert_colossus_fairy")
        return { SCENE_DESERT_COLOSSUS, ENTR_DESERT_COLOSSUS_GREAT_FAIRY_EXIT };
    if (location == "desert_colossus_spirit")
        return { SCENE_DESERT_COLOSSUS, ENTR_DESERT_COLOSSUS_OUTSIDE_TEMPLE,
                 ENTR_DESERT_COLOSSUS_SPIRIT_TEMPLE_BLUE_WARP };
    if (location == "zoras_fountain")
        return { SCENE_ZORAS_FOUNTAIN, ENTR_ZORAS_FOUNTAIN_TUNNEL_EXIT };
    if (location == "fire_temple_return")
        return { SCENE_DEATH_MOUNTAIN_CRATER, ENTR_DEATH_MOUNTAIN_CRATER_OUTSIDE_TEMPLE,
                 ENTR_DEATH_MOUNTAIN_CRATER_FIRE_TEMPLE_BLUE_WARP };
    if (location == "lake_hylia")
        return { SCENE_LAKE_HYLIA, ENTR_LAKE_HYLIA_OUTSIDE_TEMPLE, ENTR_LAKE_HYLIA_WARP_PAD };
    if (location == "bombchu_bowling")
        return { SCENE_BOMBCHU_BOWLING_ALLEY, ENTR_BOMBCHU_BOWLING_ALLEY_0 };
    if (location == "ice_cavern_exit" || location == "ice_cavern_approach")
        return { SCENE_ZORAS_FOUNTAIN, ENTR_ZORAS_FOUNTAIN_OUTSIDE_ICE_CAVERN };
    if (location == "lost_woods_kokiri")
        return { SCENE_LOST_WOODS, ENTR_LOST_WOODS_SOUTH_EXIT };
    if (location == "zoras_river")
        return { SCENE_ZORAS_RIVER, ENTR_ZORAS_RIVER_WEST_EXIT };
    if (location == "hyrule_castle_fairy")
        return { SCENE_HYRULE_CASTLE, ENTR_CASTLE_GROUNDS_GREAT_FAIRY_EXIT };
    if (location == "lon_lon_ranch")
        return { SCENE_LON_LON_RANCH, ENTR_LON_LON_RANCH_OUTSIDE_TALONS_HOUSE };
    if (location == "gerudo_valley")
        return { SCENE_GERUDO_VALLEY, ENTR_GERUDO_VALLEY_WEST_EXIT };
    if (location == "skulltula_house")
        return { SCENE_HOUSE_OF_SKULLTULA, ENTR_HOUSE_OF_SKULLTULA_0 };
    return {};
}

static bool NativePrerequisiteKnown(std::string_view gate) {
    constexpr std::string_view gates[] = { "native_access",
                                           "existing_source",
                                           "child",
                                           "adult",
                                           "bracelet",
                                           "membership",
                                           "bomb_bag",
                                           "deku_tree",
                                           "suns_song",
                                           "master_sword",
                                           "forest",
                                           "king_dodongo",
                                           "volvagia",
                                           "water",
                                           "bowling_prize",
                                           "iron_boots_serenade",
                                           "spirit",
                                           "ranch_prize",
                                           "longshot",
                                           "bridge_or_longshot",
                                           "water_longshot",
                                           "fire_hammer",
                                           "six_medallions_master",
                                           "adult_bow_bomb_bag",
                                           "thirty_tokens_stone" };
    for (const auto known : gates)
        if (gate == known)
            return true;
    return false;
}

static bool NativePrerequisite(std::string_view gate) {
    if (gate == "native_access" || gate == "existing_source")
        return true;
    if (gate == "child")
        return !LINK_IS_ADULT;
    if (gate == "adult")
        return LINK_IS_ADULT;
    if (gate == "bracelet")
        return CUR_UPG_VALUE(UPG_STRENGTH) >= 1;
    if (gate == "membership")
        return CHECK_QUEST_ITEM(QUEST_GERUDO_CARD);
    if (gate == "bomb_bag")
        return CUR_UPG_VALUE(UPG_BOMB_BAG) >= 1;
    if (gate == "deku_tree")
        return CHECK_QUEST_ITEM(QUEST_KOKIRI_EMERALD);
    if (gate == "suns_song")
        return CHECK_QUEST_ITEM(QUEST_SONG_SUN);
    if (gate == "master_sword")
        return LINK_IS_ADULT && CHECK_OWNED_EQUIP(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_MASTER);
    if (gate == "forest")
        return CHECK_QUEST_ITEM(QUEST_MEDALLION_FOREST);
    if (gate == "king_dodongo")
        return CHECK_QUEST_ITEM(QUEST_GORON_RUBY);
    if (gate == "volvagia")
        return CHECK_QUEST_ITEM(QUEST_MEDALLION_FIRE);
    if (gate == "water")
        return CHECK_QUEST_ITEM(QUEST_MEDALLION_WATER);
    if (gate == "bowling_prize")
        return (Nei_Save()->vanillaRewardEvents & 1) || Flags_GetItemGetInf(ITEMGETINF_11) ||
               Flags_GetItemGetInf(ITEMGETINF_12);
    if (gate == "iron_boots_serenade")
        return CHECK_OWNED_EQUIP(EQUIP_TYPE_BOOTS, EQUIP_INV_BOOTS_IRON) && CHECK_QUEST_ITEM(QUEST_SONG_SERENADE);
    if (gate == "spirit")
        return CHECK_QUEST_ITEM(QUEST_MEDALLION_SPIRIT);
    if (gate == "ranch_prize")
        return Flags_GetItemGetInf(ITEMGETINF_TALON_BOTTLE);
    if (gate == "longshot")
        return gSaveContext.inventory.items[SLOT_HOOKSHOT] == ITEM_LONGSHOT;
    if (gate == "bridge_or_longshot")
        return GET_EVENTCHKINF_CARPENTERS_FREE_ALL() || gSaveContext.inventory.items[SLOT_HOOKSHOT] == ITEM_LONGSHOT;
    if (gate == "water_longshot")
        return CHECK_QUEST_ITEM(QUEST_MEDALLION_WATER) && gSaveContext.inventory.items[SLOT_HOOKSHOT] == ITEM_LONGSHOT;
    if (gate == "fire_hammer")
        return CHECK_QUEST_ITEM(QUEST_MEDALLION_FIRE) && gSaveContext.inventory.items[SLOT_HAMMER] == ITEM_HAMMER;
    if (gate == "six_medallions_master")
        return (gSaveContext.inventory.questItems & 0x3F) == 0x3F &&
               CHECK_OWNED_EQUIP(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_MASTER);
    if (gate == "adult_bow_bomb_bag")
        return LINK_IS_ADULT && gSaveContext.inventory.items[SLOT_BOW] == ITEM_BOW && CUR_UPG_VALUE(UPG_BOMB_BAG) >= 1;
    if (gate == "thirty_tokens_stone")
        return gSaveContext.inventory.gsTokens >= 30 && CHECK_QUEST_ITEM(QUEST_STONE_OF_AGONY);
    return false;
}

static bool NativeOwned(const Reward& reward) {
    const auto* nei = Nei_Save();
    switch (reward.kind) {
        case RewardKind::Cane:
            return reward.payload < 6 && (nei->caneSkills & (1u << reward.payload));
        case RewardKind::Wand:
            return reward.payload < 6 && (nei->wandRodsOwned & (1u << reward.payload));
        case RewardKind::Rune:
            return reward.payload < 5 && (nei->slateRunesOwned & (1u << reward.payload));
        case RewardKind::Season:
            return reward.payload < 4 && (nei->seasonsOwned & (1u << reward.payload));
        case RewardKind::SeasonRod:
            return nei->seasonsRodOwned;
        case RewardKind::Equipment:
            return ExtEquip_HasItem(reward.payload >> 8, reward.payload & 255);
        case RewardKind::Retained:
            return ExtInv_GetSlotItem(SLOT_ROCS) == ITEM_ROCS_FEATHER_SKIJER ||
                   ExtInv_GetSlotItem(SLOT_ROCS) == ITEM_ROCS_CAPE;
        case RewardKind::Item:
            if (reward.payload == ITEM_SHOVEL)
                return nei->shovelOwned || ExtInv_GetSlotItem(SLOT_SHOVEL) == ITEM_SHOVEL;
            if (reward.payload == ITEM_TIME_GATE)
                return TravelTools_IsOwned(reward.payload);
            if (reward.payload == ITEM_HYLIAS_GRACE)
                return GraceHourglass_IsOwned(reward.payload);
            return ExtInv_GetItemSlot(reward.payload) != 0xFF &&
                   ExtInv_GetSlotItem(ExtInv_GetItemSlot(reward.payload)) == reward.payload;
        case RewardKind::Upgrade:
            switch (static_cast<Upgrade>(reward.payload)) {
                case Upgrade::Clawshot:
                    return nei->twilightUpgrade & 1;
                case Upgrade::Ultrashot:
                    return nei->ultrashotOwned;
                case Upgrade::HammerAxe:
                    return nei->weaponUpgrades & 1;
                case Upgrade::TrueMasterSword:
                    return nei->weaponUpgrades & 8;
                case Upgrade::BombArrows:
                    return nei->bombArrowsOwned || (nei->twilightUpgrade & 2);
                case Upgrade::Quartz:
                    return nei->quartzOwned;
                case Upgrade::RitoMask:
                    return nei->ritoMaskFlags & RITO_FLAG_MASK_OWNED;
                case Upgrade::SeparateRocsFeather:
                    return nei->separateRocsFlags & 1;
                default:
                    return false;
            }
    }
    return false;
}

static bool GrantNative(const Reward& reward) {
    auto* nei = Nei_Save();
    switch (reward.kind) {
        case RewardKind::Cane:
            if (reward.payload >= 6)
                return false;
            Cane_GiveSkill(static_cast<u8>(reward.payload));
            break;
        case RewardKind::Wand:
            if (reward.payload >= 6)
                return false;
            Wand_GrantMode(static_cast<u8>(reward.payload));
            break;
        case RewardKind::Rune:
            if (reward.payload >= 5)
                return false;
            Slate_GrantRune(static_cast<u8>(reward.payload));
            break;
        case RewardKind::Season:
            if (reward.payload >= 4)
                return false;
            Seasons_GrantSeason(static_cast<u8>(reward.payload));
            break;
        case RewardKind::SeasonRod:
            Seasons_GrantRod();
            break;
        case RewardKind::Equipment:
            if ((reward.payload >> 8) > 3 || !(reward.payload & 255) || (reward.payload & 255) > 3)
                return false;
            ExtEquip_GiveItem(reward.payload >> 8, reward.payload & 255);
            break;
        case RewardKind::Item:
            if (reward.payload == ITEM_SHOVEL)
                nei->shovelOwned = 1;
            if (reward.payload == ITEM_TIME_GATE)
                TravelTools_Grant(reward.payload);
            else if (reward.payload == ITEM_HYLIAS_GRACE)
                GraceHourglass_Grant(reward.payload);
            else
                ExtInv_SetItemById(reward.payload);
            break;
        case RewardKind::Upgrade:
            switch (static_cast<Upgrade>(reward.payload)) {
                case Upgrade::Clawshot:
                    TwilightUpgrade_SetClawshot(1);
                    break;
                case Upgrade::Ultrashot:
                    if (gSaveContext.inventory.items[SLOT_HOOKSHOT] != ITEM_LONGSHOT)
                        return false;
                    nei->ultrashotOwned = 1;
                    break;
                case Upgrade::HammerAxe:
                    if (gSaveContext.inventory.items[SLOT_HAMMER] != ITEM_HAMMER)
                        return false;
                    WeaponUpgrade_SetHammerAxe(1);
                    break;
                case Upgrade::TrueMasterSword:
                    if (!CHECK_OWNED_EQUIP(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_MASTER))
                        return false;
                    WeaponUpgrade_SetTrueMaster(1);
                    break;
                case Upgrade::BombArrows:
                    nei->bombArrowsOwned = 1;
                    break;
                case Upgrade::Quartz:
                    if (!CHECK_QUEST_ITEM(QUEST_STONE_OF_AGONY))
                        return false;
                    nei->quartzOwned = 1;
                    break;
                case Upgrade::RitoMask:
                    RitoItem_NoteCellItem(gSaveContext.inventory.items[SLOT_FARORES_WIND]);
                    nei->ritoMaskFlags |= RITO_FLAG_MASK_OWNED;
                    if (gSaveContext.inventory.items[SLOT_FARORES_WIND] == ITEM_NONE)
                        gSaveContext.inventory.items[SLOT_FARORES_WIND] = ITEM_RITO_MASK;
                    break;
                case Upgrade::SeparateRocsFeather:
                    VanillaRocsFeather_Grant();
                    break;
                default:
                    return false;
            }
            break;
        case RewardKind::Retained:
            return false;
    }
    return NativeOwned(reward);
}

static bool NativeBowlingPrize(PlayState* play, const GetItemEntry& entry) {
    if (!play || play->sceneNum != SCENE_BOMBCHU_BOWLING_ALLEY || entry.modIndex != MOD_NONE ||
        entry.comboMarketReceipt || entry.comboForeignCheck)
        return false;
    const auto* player = GET_PLAYER(play);
    if (!player || !player->interactRangeActor || player->interactRangeActor->id != ACTOR_EN_BOM_BOWL_PIT)
        return false;
    switch (entry.getItemId) {
        case GI_BOMB_BAG_30:
        case GI_BOMB_BAG_40:
        case GI_HEART_PIECE:
        case GI_BOMBCHUS_10:
        case GI_BOMBS_1:
        case GI_RUPEE_PURPLE:
            return true;
        default:
            return false;
    }
}
// END TESTED NATIVE REWARD HELPERS

constexpr int kReceiptBit = 0x40000000;
constexpr u16 kOfferText = 0x8EE0, kReceiptText = 0x8EE1, kResultText = 0x8EE2;
RewardHost sHost = nullptr;
const Reward* sReceiptReward = nullptr;
int sReceiptSlot = -1, sReceiptTicket = 0, sSequence = 0;
bool sOffered = false, sDelivered = false;
GetItemEntry sReceiptEntry{};
std::string sStatus;
s16 sActorId = -1;
Vec3f sEntryPosition{};
bool sEntryReady = false;

struct RewardActor {
    Actor actor;
    GetItemEntry entry;
    bool talking;
};

static bool Active() {
    return Combo_IsVanilla() && sHost && gPlayState && gSaveContext.fileNum <= 2 &&
           gSaveContext.gameMode == GAMEMODE_NORMAL && gPlayState->state.running;
}

static int Host(RewardAction action, const Reward& reward, int slot, const char* native = nullptr) {
    return sHost ? sHost(static_cast<int>(action), 0, slot, reward.checkId, native) : -1;
}

static bool Safe(PlayState* play) {
    if (!Active() || play != gPlayState || play->transitionTrigger != TRANS_TRIGGER_OFF ||
        play->transitionMode != TRANS_MODE_OFF || play->pauseCtx.state != 0 || play->pauseCtx.debugState != 0 ||
        play->gameOverCtx.state != GAMEOVER_INACTIVE || play->csCtx.state != CS_STATE_IDLE ||
        gSaveContext.cutsceneTrigger)
        return false;
    const auto* player = GET_PLAYER(play);
    return player && !Player_InCsMode(play) &&
           !(player->stateFlags1 &
             (PLAYER_STATE1_DEAD | PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_TALKING | PLAYER_STATE1_IN_CUTSCENE |
              PLAYER_STATE1_HANGING_OFF_LEDGE | PLAYER_STATE1_CLIMBING_LEDGE | PLAYER_STATE1_FREEFALL |
              PLAYER_STATE1_JUMPING | PLAYER_STATE1_IN_WATER | PLAYER_STATE1_CLIMBING_LADDER));
}

static RandomizerGet PresentationId(const Reward& reward) {
    switch (reward.kind) {
        case RewardKind::Item: {
            const auto* item = Nei_FindByItem(reward.payload);
            return item && item->rg != NEI_NO_RG ? static_cast<RandomizerGet>(item->rg) : RG_NONE;
        }
        case RewardKind::Cane: {
            constexpr RandomizerGet ids[] = { RG_CANE_OF_SOMARIA, RG_CANE_SOMARIA_BLOCK, RG_CANE_SOMARIA_PLATFORM,
                                              RG_CANE_PACCI_FLIP, RG_CANE_PACCI_STONE,   RG_CANE_PACCI_ULTRAHAND };
            return reward.payload < std::size(ids) ? ids[reward.payload] : RG_NONE;
        }
        case RewardKind::Wand: {
            constexpr RandomizerGet ids[] = { RG_WAND_SAND_ROD,   RG_WAND_TORNADO_ROD, RG_WAND_WATER_ROD,
                                              RG_WAND_METEOR_ROD, RG_WAND_STORM_ROD,   RG_WAND_SHADOW_SCEPTER };
            return reward.payload < std::size(ids) ? ids[reward.payload] : RG_NONE;
        }
        case RewardKind::Rune: {
            constexpr RandomizerGet ids[] = { RG_SLATE_RUNE_BOMB, RG_SLATE_RUNE_STASIS, RG_SLATE_RUNE_CRYONIS,
                                              RG_SLATE_RUNE_MASTER_CYCLE, RG_DESIRE_SENSOR };
            return reward.payload < std::size(ids) ? ids[reward.payload] : RG_NONE;
        }
        case RewardKind::Season: {
            constexpr RandomizerGet ids[] = { RG_SEASON_SPRING, RG_SEASON_SUMMER, RG_SEASON_AUTUMN, RG_SEASON_WINTER };
            return reward.payload < std::size(ids) ? ids[reward.payload] : RG_NONE;
        }
        case RewardKind::SeasonRod:
            return RG_ROD_OF_SEASONS;
        case RewardKind::Equipment: {
            constexpr RandomizerGet ids[4][3] = {
                { RG_EXT_CANE_OF_BYRNA, RG_EXT_FOUR_SWORD, RG_EXT_TRIDENT },
                { RG_EXT_DIVINE_SHIELD, RG_EXT_SHEIKAH_SHIELD, RG_EXT_SHIELD_OF_IKANA },
                { RG_EXT_CHAMPIONS_TUNIC, RG_EXT_SPIRIT_BREASTPLATE, RG_EXT_WATER_DRAGON_SCALE },
                { RG_EXT_PEGASUS_ANKLET, RG_EXT_CLIMB_BOOTS, RG_EXT_ROC_BOOTS }
            };
            const unsigned type = reward.payload >> 8, index = reward.payload & 255;
            return type < 4 && index >= 1 && index <= 3 ? ids[type][index - 1] : RG_NONE;
        }
        case RewardKind::Upgrade:
            switch (static_cast<Upgrade>(reward.payload)) {
                case Upgrade::Clawshot:
                    return RG_CLAWSHOT;
                case Upgrade::Ultrashot:
                    return RG_ULTRASHOT;
                case Upgrade::HammerAxe:
                    return RG_IRON_KNUCKLE_AXE;
                case Upgrade::TrueMasterSword:
                    return RG_TRUE_MASTER_SWORD;
                case Upgrade::BombArrows:
                    return RG_BOMB_ARROWS;
                case Upgrade::Quartz:
                    return RG_QUARTZ_OF_MOTION;
                case Upgrade::SeparateRocsFeather:
                    return RG_ROCS_FEATHER;
                // Rito has no randomizer row. Its isolated receipt uses an
                // inert row only for native animation; custom art/text and the
                // token-checked explicit grant below own its real identity.
                case Upgrade::RitoMask:
                    return RG_CLAWSHOT;
                default:
                    return RG_NONE;
            }
        case RewardKind::Retained:
            return RG_NONE;
    }
    return RG_NONE;
}

static void DrawRitoReceipt(PlayState* play, GetItemEntry*) {
    const auto* icon = ExtInv_GetItemIcon(ITEM_RITO_MASK);
    if (!icon)
        return;
    auto* vertices = static_cast<Vtx*>(Graph_Alloc(play->state.gfxCtx, 4 * sizeof(Vtx)));
    if (!vertices)
        return;
    for (unsigned i = 0; i < 4; ++i) {
        auto& v = vertices[i];
        v = {};
        v.v.ob[0] = (i == 0 || i == 3) ? -16 : 16;
        v.v.ob[1] = i < 2 ? 16 : -16;
        v.v.tc[0] = (i == 0 || i == 3) ? 0 : 1024;
        v.v.tc[1] = i < 2 ? 0 : 1024;
        v.v.cn[0] = v.v.cn[1] = v.v.cn[2] = v.v.cn[3] = 255;
    }
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, const_cast<char*>(__FILE__), __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPClearGeometryMode(POLY_OPA_DISP++, G_LIGHTING | G_CULL_BOTH);
    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_DECALRGBA, G_CC_DECALRGBA);
    gSPTexture(POLY_OPA_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPLoadTextureBlock(POLY_OPA_DISP++, icon, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0, G_TX_CLAMP, G_TX_CLAMP, 5, 5,
                        G_TX_NOLOD, G_TX_NOLOD);
    gSPVertex(POLY_OPA_DISP++, reinterpret_cast<uintptr_t>(vertices), 4, 0);
    gSP2Triangles(POLY_OPA_DISP++, 0, 1, 2, 0, 0, 2, 3, 0);
    CLOSE_DISPS(play->state.gfxCtx);
}

static bool BuildEntry(const Reward& reward, GetItemEntry& entry) {
    const auto id = PresentationId(reward);
    if (id == RG_NONE)
        return false;
    const auto source = Rando::StaticData::RetrieveItem(id).GetGIEntryUnresolved();
    if (!source || source->modIndex != MOD_RANDOMIZER)
        return false;
    entry = *source;
    entry.textId = kReceiptText;
    entry.getItemFrom = ITEM_FROM_FREESTANDING;
    entry.comboForeignCheck = 0;
    entry.comboMarketReceipt = 0;
    if (reward.kind == RewardKind::Upgrade && reward.payload == static_cast<u16>(Upgrade::RitoMask))
        entry.drawFunc = DrawRitoReceipt;
    return true;
}

static void CancelReceipt(bool destroying = false) {
    if (sReceiptReward && !sDelivered) {
        Host(RewardAction::Abort, *sReceiptReward, sReceiptSlot);
        if (!destroying && gPlayState) {
            auto* player = GET_PLAYER(gPlayState);
            if (player && player->getItemEntry.comboMarketReceipt == sReceiptTicket) {
                player->getItemEntry = GET_ITEM_NONE;
                player->getItemId = GI_NONE;
                if (player->interactRangeActor == &player->actor)
                    player->interactRangeActor = nullptr;
            }
        }
    }
    sReceiptReward = nullptr;
    sReceiptSlot = -1;
    sReceiptTicket = 0;
    sOffered = sDelivered = false;
    sReceiptEntry = {};
}

static bool AcknowledgeOwned(const Reward& reward) {
    SaveManager::Instance->ThreadPoolWait();
    const int slot = gSaveContext.fileNum;
    if (Host(RewardAction::BeginOwned, reward, slot) != 1)
        return false;
    try {
        SaveManager::Instance->ThreadPoolWait();
        const auto snapshot = SaveManager::Instance->SaveToJsonObject().dump();
        if (Host(RewardAction::Commit, reward, slot, snapshot.c_str()) == 1)
            return true;
    } catch (const std::exception& exception) {
        SPDLOG_ERROR("[Vanilla NEI] owned check {}: {}", reward.checkId, exception.what());
    }
    SaveManager::Instance->ThreadPoolWait();
    Host(RewardAction::Abort, reward, slot);
    return false;
}

static bool BeginReceipt(const Reward& reward) {
    if (!Active() || sReceiptReward || !NativePrerequisite(reward.prerequisite) || !SaveManager::Instance ||
        Host(RewardAction::Query, reward, gSaveContext.fileNum) != 0)
        return false;
    if (NativeOwned(reward)) {
        const bool committed = AcknowledgeOwned(reward);
        sStatus = committed ? "You already own this reward. No rupees were charged." : "Unable to save. Try again.";
        return false;
    }
    const int rupees = gSaveContext.rupees + std::min<int>(0, gSaveContext.rupeeAccumulator);
    if (rupees < reward.price) {
        sStatus = "You need more rupees.";
        return false;
    }
    GetItemEntry entry;
    if (!BuildEntry(reward, entry)) {
        sStatus = "This reward is unavailable.";
        return false;
    }
    SaveManager::Instance->ThreadPoolWait();
    const int slot = gSaveContext.fileNum;
    if (Host(RewardAction::Begin, reward, slot) != 1) {
        sStatus = "Unable to save. Try again.";
        return false;
    }
    sReceiptReward = &reward;
    sReceiptSlot = slot;
    sSequence = sSequence >= 0x3FFFFFFE ? 1 : sSequence + 1;
    sReceiptTicket = kReceiptBit | sSequence;
    sOffered = sDelivered = false;
    entry.comboMarketReceipt = sReceiptTicket;
    sReceiptEntry = entry;
    sStatus.clear();
    return true;
}

static bool FixedReceiptGrant(PlayState* play, const GetItemEntry& entry) {
    if (!(entry.comboMarketReceipt & kReceiptBit))
        return false;
    const auto* player = play == gPlayState && play ? GET_PLAYER(play) : nullptr;
    if (!Active() || !sReceiptReward || sDelivered || !sOffered || gSaveContext.fileNum != sReceiptSlot ||
        entry.comboMarketReceipt != sReceiptTicket || !player ||
        player->getItemEntry.comboMarketReceipt != sReceiptTicket || entry.getItemId != sReceiptEntry.getItemId ||
        !(player->stateFlags1 & PLAYER_STATE1_GETTING_ITEM))
        return true; // A stale/cancelled token must not reach any progressive grant.
    const auto& reward = *sReceiptReward;
    const SaveContext before = gSaveContext;
    const NeiSaveData beforeNei = *Nei_Save();
    bool committed = false;
    try {
        const bool owned = NativeOwned(reward);
        if (owned) {
            Host(RewardAction::Abort, reward, sReceiptSlot);
            if (Host(RewardAction::BeginOwned, reward, sReceiptSlot) != 1)
                throw std::runtime_error("cannot acknowledge pre-owned reward");
        } else {
            if (!NativePrerequisite(reward.prerequisite) || gSaveContext.rupees < reward.price || !GrantNative(reward))
                throw std::runtime_error("native prerequisite or grant rejected");
            gSaveContext.rupees -= reward.price;
        }
        // A native grant may have queued a section save. Drain it while the
        // host stage still quarantines ordinary writes, before final commit.
        SaveManager::Instance->ThreadPoolWait();
        const auto snapshot = SaveManager::Instance->SaveToJsonObject().dump();
        committed = Host(RewardAction::Commit, reward, sReceiptSlot, snapshot.c_str()) == 1;
    } catch (const std::exception& exception) {
        SPDLOG_ERROR("[Vanilla NEI] receipt {}: {}", reward.checkId, exception.what());
    }
    if (!committed) {
        SaveManager::Instance->ThreadPoolWait();
        gSaveContext = before;
        *Nei_Save() = beforeNei;
        Host(RewardAction::Abort, reward, sReceiptSlot);
        sStatus = "The reward could not be saved. It remains available; no rupees were charged.";
        CustomMessage message(sStatus);
        message.AutoFormat();
        message.LoadIntoFont();
    }
    sDelivered = true; // Native presentation finishes normally even after rollback.
    return true;
}

static void MonitorReceipt() {
    if (!sReceiptReward)
        return;
    if (!Active() || gSaveContext.fileNum != sReceiptSlot) {
        CancelReceipt();
        return;
    }
    auto* play = gPlayState;
    auto* player = GET_PLAYER(play);
    if (play->transitionTrigger != TRANS_TRIGGER_OFF || play->gameOverCtx.state != GAMEOVER_INACTIVE) {
        CancelReceipt();
        return;
    }
    if (sDelivered) {
        if (Safe(play) && Message_GetState(&play->msgCtx) == TEXT_STATE_NONE)
            CancelReceipt();
        return;
    }
    if (sOffered) {
        if (player && player->getItemEntry.comboMarketReceipt != sReceiptTicket &&
            !(player->stateFlags1 & PLAYER_STATE1_GETTING_ITEM))
            CancelReceipt();
        return;
    }
    if (Safe(play) && Message_GetState(&play->msgCtx) == TEXT_STATE_NONE)
        sOffered = GiveItemEntryWithoutActor(play, sReceiptEntry) != 0;
}

static const Reward* ActorReward(const Actor* actor) {
    if (!actor || actor->id != sActorId || actor->params < 0 ||
        static_cast<size_t>(actor->params) >= std::size(kRewards))
        return nullptr;
    const auto& reward = kRewards[actor->params];
    return reward.game == Game::Oot ? &reward : nullptr;
}

static void RewardText(u16* textId, bool* fromTable) {
    if (!Active())
        return;
    const Reward* reward = nullptr;
    if (*textId == kReceiptText)
        reward = sReceiptReward;
    else if (*textId == kOfferText || *textId == kResultText)
        reward = ActorReward(gPlayState->msgCtx.talkActor);
    if (!reward)
        return;
    std::string text;
    if (*textId == kResultText)
        text = sStatus;
    else if (*textId == kReceiptText)
        text = std::string("You received %g") + reward->label + "%w!";
    else {
        text = std::string("%g") + reward->label + "%w";
        if (reward->price)
            text += " - " + std::to_string(reward->price) + " Rupees";
        text += "&";
        text += reward->price ? "Buy it?" : "Take this optional reward?";
        text += "&" + CustomMessage::TWO_WAY_CHOICE() + "Yes&No";
    }
    CustomMessage message(text);
    message.AutoFormat();
    message.LoadIntoFont();
    *fromTable = false;
}

static void RewardInit(Actor* actor, PlayState*) {
    auto* placed = reinterpret_cast<RewardActor*>(actor);
    const auto* reward = ActorReward(actor);
    placed->talking = false;
    if (!reward || !BuildEntry(*reward, placed->entry)) {
        Actor_Kill(actor);
        return;
    }
    actor->textId = kOfferText;
    Actor_SetScale(actor, 0.2f);
    Actor_SetFocus(actor, 25);
}

static void RewardUpdate(Actor* actor, PlayState* play) {
    auto* placed = reinterpret_cast<RewardActor*>(actor);
    const auto* reward = ActorReward(actor);
    if (!Active() || !reward || Host(RewardAction::Query, *reward, gSaveContext.fileNum) == 1) {
        Actor_Kill(actor);
        return;
    }
    if (sReceiptReward)
        return;
    if (Actor_ProcessTalkRequest(actor, play))
        placed->talking = true;
    if (placed->talking) {
        const auto state = Message_GetState(&play->msgCtx);
        if (state == TEXT_STATE_CHOICE && Message_ShouldAdvance(play)) {
            if (play->msgCtx.choiceIndex == 0) {
                if (BeginReceipt(*reward)) {
                    Message_CloseTextbox(play);
                    placed->talking = false;
                } else {
                    if (sStatus.empty())
                        sStatus = "This reward is not ready yet.";
                    Message_ContinueTextbox(play, kResultText);
                }
            } else {
                Message_CloseTextbox(play);
                placed->talking = false;
            }
        } else if (state == TEXT_STATE_CLOSING || state == TEXT_STATE_NONE) {
            placed->talking = false;
        }
    } else if (Safe(play) && NativePrerequisite(reward->prerequisite)) {
        actor->textId = kOfferText;
        Actor_OfferTalk(actor, play, 80);
    }
}

static void RewardDraw(Actor* actor, PlayState* play) {
    const auto* reward = ActorReward(actor);
    if (!reward || sReceiptReward == reward)
        return;
    func_8002EBCC(actor, play, 0);
    func_8002ED80(actor, play, 0);
    GetItemEntry_Draw(play, reinterpret_cast<RewardActor*>(actor)->entry);
}

static bool EntranceMatches(int entrance) {
    return entrance >= 0 && gSaveContext.entranceIndex >= entrance && gSaveContext.entranceIndex <= entrance + 3;
}

static bool Anchor(const Reward& reward, const NativeRewardBinding& binding, Vec3f& point) {
    auto* play = gPlayState;
    if (!sEntryReady || binding.scene != play->sceneNum || !play->linkActorEntry)
        return false;
    const bool forestMeadow = std::string_view(reward.location) == "sacred_forest_meadow";
    if (!forestMeadow && binding.entrance >= 0 && !EntranceMatches(binding.entrance) &&
        !EntranceMatches(binding.alternateEntrance))
        return false;
    // Entry-side optional GTG cache: never spawn behind a native key door.
    if (play->sceneNum == SCENE_GERUDO_TRAINING_GROUND && play->roomCtx.curRoom.num != 0)
        return false;
    int landmark = binding.actor;
    if (forestMeadow) {
        if (reward.kind == RewardKind::Item) {
            if (!EntranceMatches(ENTR_SACRED_FOREST_MEADOW_SOUTH_EXIT))
                return false;
            landmark = -1;
        } else {
            if (!EntranceMatches(ENTR_SACRED_FOREST_MEADOW_OUTSIDE_TEMPLE) &&
                !EntranceMatches(ENTR_SACRED_FOREST_MEADOW_FOREST_TEMPLE_BLUE_WARP))
                return false;
            landmark = -1;
        }
    }
    point = sEntryPosition;
    if (landmark >= 0) {
        bool found = false;
        for (const auto& list : play->actorCtx.actorLists)
            for (auto* actor = list.head; actor; actor = actor->next)
                if (actor->id == landmark && actor->update) {
                    point = actor->home.pos;
                    found = true;
                    break;
                }
        if (!found)
            return false;
    }
    return true;
}

static bool SafeRewardPosition(PlayState* play, const Vec3f& anchor, unsigned ordinal, Vec3f& point) {
    auto* player = GET_PLAYER(play);
    if (!player)
        return false;
    // A small set of nearby alternatives keeps the item off the native door,
    // pedestal and return warp. Each is checked against the actual loaded
    // floor/water/walls; no archive-specific coordinates are assumed.
    constexpr std::array<Vec3f, 8> offsets = { { { 65, 0, 45 },
                                                 { -65, 0, 45 },
                                                 { 45, 0, -65 },
                                                 { -45, 0, -65 },
                                                 { 95, 0, 0 },
                                                 { -95, 0, 0 },
                                                 { 0, 0, 95 },
                                                 { 0, 0, -95 } } };
    for (unsigned index = 0; index < offsets.size(); ++index) {
        const auto offset = offsets[(index + ordinal) % offsets.size()];
        Vec3f probe = { anchor.x + offset.x, anchor.y + 80, anchor.z + offset.z };
        CollisionPoly* floor = nullptr;
        s32 bg = BGCHECK_SCENE;
        const float y = BgCheck_EntityRaycastFloor5(play, &play->colCtx, &floor, &bg, &player->actor, &probe);
        if (!std::isfinite(y) || y == BGCHECK_Y_MIN || !floor || bg != BGCHECK_SCENE || floor->normal.y < 30000 ||
            std::abs(y - anchor.y) > 45 || SurfaceType_GetSceneExitIndex(&play->colCtx, floor, bg))
            continue;
        float water = 0;
        WaterBox* waterBox = nullptr;
        if (WaterBox_GetSurface1(play, &play->colCtx, probe.x, probe.z, &water, &waterBox) && water > y - 5)
            continue;
        Vec3f from = { anchor.x, anchor.y + 30, anchor.z }, to = { probe.x, y + 30, probe.z }, hit{};
        CollisionPoly* wall = nullptr;
        s32 wallBg = BGCHECK_SCENE;
        if (BgCheck_EntityLineTest1(&play->colCtx, &from, &to, &hit, &wall, true, false, false, true, &wallBg))
            continue;
        point = { probe.x, y + 22, probe.z };
        return true;
    }
    return false;
}

static void SpawnRewards() {
    MonitorReceipt();
    if (!Safe(gPlayState) || sReceiptReward || !SaveManager::Instance || !ActorDB::Instance)
        return;
    auto* play = gPlayState;
    const auto* player = GET_PLAYER(play);
    if (play->gameplayFrames % 16 != 0)
        return;
    if (!sEntryReady) {
        if (!play->linkActorEntry || !player->actor.floorPoly || player->actor.floorBgId != BGCHECK_SCENE ||
            player->actor.floorPoly->normal.y < 30000 ||
            std::abs(player->actor.world.pos.y - player->actor.floorHeight) > 8)
            return;
        const auto& native = play->linkActorEntry->pos;
        sEntryPosition = { static_cast<float>(native.x), static_cast<float>(native.y), static_cast<float>(native.z) };
        sEntryReady = true;
    }
    for (unsigned i = 0; i < std::size(kRewards); ++i) {
        const auto& reward = kRewards[i];
        if (reward.game != Game::Oot)
            continue;
        const auto binding = NativeBinding(reward.location);
        if (binding.scene != play->sceneNum || !NativePrerequisite(reward.prerequisite) ||
            Host(RewardAction::Query, reward, gSaveContext.fileNum) != 0)
            continue;
        if (reward.kind == RewardKind::Retained) {
            if (NativeOwned(reward))
                AcknowledgeOwned(reward);
            continue;
        }
        bool present = false;
        for (auto* actor = play->actorCtx.actorLists[ACTORCAT_NPC].head; actor; actor = actor->next)
            if (actor->id == sActorId && actor->params == i && actor->update) {
                present = true;
                break;
            }
        if (present)
            continue;
        Vec3f anchor, position;
        if (!Anchor(reward, binding, anchor) || !SafeRewardPosition(play, anchor, i, position))
            continue;
        Actor_Spawn(&play->actorCtx, play, sActorId, position.x, position.y, position.z, 0, 0, 0, i);
    }
}

static void Register() {
    if (!ActorDB::Instance)
        return;
    ActorDBInit actor;
    actor.name = "ComboVanillaReward";
    actor.desc = "Fixed optional Vanilla NEI reward";
    actor.category = ACTORCAT_NPC;
    actor.flags = ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_FRIENDLY | ACTOR_FLAG_UPDATE_CULLING_DISABLED |
                  ACTOR_FLAG_DRAW_CULLING_DISABLED;
    actor.objectId = OBJECT_GAMEPLAY_KEEP;
    actor.instanceSize = sizeof(RewardActor);
    actor.init = RewardInit;
    actor.update = RewardUpdate;
    actor.draw = RewardDraw;
    sActorId = ActorDB::Instance->AddEntry(actor).entry.id;
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](s16) {
        CancelReceipt(true);
        sEntryReady = false;
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>([](int) { CancelReceipt(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnOpenText>(RewardText);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnItemReceive>([](GetItemEntry entry) {
        if (!Active() || !NativeBowlingPrize(gPlayState, entry) || (Nei_Save()->vanillaRewardEvents & 1))
            return;
        Nei_Save()->vanillaRewardEvents |= 1;
        if (SaveManager::Instance)
            SaveManager::Instance->SaveFile(gSaveContext.fileNum);
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>(SpawnRewards);
}
static RegisterShipInitFunc sInit(Register);

} // namespace

extern "C" COMBO_EXPORT void SOH_SetComboVanillaRewardCallback(ComboVanilla::RewardHost callback) {
    sHost = callback;
}

extern "C" bool ComboVanillaOot_TryReceiptGrant(PlayState* play, GetItemEntry entry) {
    return FixedReceiptGrant(play, entry);
}
#endif
