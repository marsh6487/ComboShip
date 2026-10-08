#include "ItemReceiptText.h"
#include "2s2h/Rando/StaticData/StaticData.h"
#include "2s2h/FleetShipCombo/FleetComboItemsGlue.h"
#include "2s2h/FleetShipCombo/FleetComboItems.h"
#include "ComboItemReceiptText.h"
#include "ComboDungeonKeyReceipt.h"
#include "ComboCapeReceiptChoice.h"
#include "ComboSongReceiptText.h"
#include "ComboSongDrawMM.h"
#include <algorithm>
#include <array>
#include <cstring>
#ifdef COMBO_BUILD
#include "ComboExport.h"
#include "ComboResolve.h"
#include "rando/CrossForeign.h"
#include <unordered_map>
namespace Rando::MiscBehavior {
uint64_t ComboRandoGen();
const ComboRando::ForeignItem* MM_LookupForeign(RandoCheckId check);
} // namespace Rando::MiscBehavior
#endif

extern "C" {
#include "mods/extended_inventory.h"
#include "message_data_static.h"
extern float sNESFontWidths[160];
u16 Player_GetItemReceiptTextId(s16 getItemId, s16 itemId);
}

namespace {
struct DungeonInformation {
    const char* temple;
    const char* boss;
    const char* entrance;
    RandoCheckId reward;
};

// MM currently keeps these physical entrances and bosses native. The reward
// belongs to the saved check placement, independently of ownership or discovery.
constexpr DungeonInformation kDungeonInformation[] = {
    { "Woodfall Temple", "Odolwa", "Woodfall", RC_WOODFALL_TEMPLE_BOSS_WARP },
    { "Snowhead Temple", "Goht", "Snowhead", RC_SNOWHEAD_TEMPLE_BOSS_WARP },
    { "Great Bay Temple", "Gyorg", "Zora Cape's turtle", RC_GREAT_BAY_TEMPLE_BOSS_WARP },
    { "Stone Tower Temple", "Twinmold", "Stone Tower", RC_STONE_TOWER_TEMPLE_INVERTED_BOSS_WARP },
};

#ifdef COMBO_BUILD
std::string ForeignRewardName(const ComboRando::ForeignItem& item) {
    return ComboRando::StripGameSuffix(item.itemName) + (item.shared                             ? ""
                                                         : item.itemGame == ComboRando::GAME_OOT ? " (OOT)"
                                                                                                 : " (MM)");
}

struct RewardIdentity {
    std::string name;
    int game = ComboRando::GAME_MM;
    bool imported = false;
    bool shared = false;
};

RewardIdentity LoadedSeedDungeonReward(int32_t dungeon) {
    static uint64_t generation = static_cast<uint64_t>(-1);
    static int slot = -1;
    static std::array<RewardIdentity, 4> names;
    const uint64_t current = Rando::MiscBehavior::ComboRandoGen();
    if (generation != current || slot != gSaveContext.fileNum) {
        generation = current;
        slot = gSaveContext.fileNum;
        names = {};
        try {
            const auto seed = nlohmann::json::parse(ComboRando::g_comboForeignJson);
            const auto placements =
                seed.value("mm", nlohmann::json::object()).value("placements", nlohmann::json::object());
            const auto foreign = ComboRando::LoadForeignForGame(0, ComboRando::GAME_MM);
            for (int i = 0; i < 4; ++i) {
                const auto& checkName = Rando::StaticData::GetCheckDisplayName(kDungeonInformation[i].reward);
                const auto placement = placements.find(checkName);
                if (placement == placements.end() || !placement->is_string())
                    continue;
                const auto imported = foreign.find(checkName);
                if (imported != foreign.end()) {
                    names[i] = { ComboRando::StripGameSuffix(imported->second.itemName), imported->second.itemGame,
                                 true, imported->second.shared };
                } else {
                    names[i].name = placement->get<std::string>();
                }
                if (names[i].name == ComboRando::kForeignSentinelNameMM)
                    names[i] = {};
            }
        } catch (...) { names = {}; }
    }
    return names[dungeon];
}
#endif

std::string DungeonRewardName(int32_t dungeon) {
    if (dungeon < 0 || dungeon >= 4)
        return {};
#ifdef COMBO_BUILD
    // The launcher pushes the selected seed before MM gameplay/save hydration.
    // Prefer it over dormant save data, which may still belong to a prior slot.
    if (!ComboRando::g_comboForeignJson.empty()) {
        const auto reward = LoadedSeedDungeonReward(dungeon);
        return reward.name.empty() ? std::string{}
                                   : reward.name + (reward.imported && !reward.shared
                                                        ? (reward.game == ComboRando::GAME_OOT ? " (OOT)" : " (MM)")
                                                        : "");
    }
#endif
    if (gSaveContext.fileNum == 0xFF || gSaveContext.save.shipSaveInfo.saveType != SAVETYPE_RANDO)
        return {};
    const auto check = kDungeonInformation[dungeon].reward;
    const auto id = gSaveContext.save.shipSaveInfo.rando.randoSaveChecks[check].randoItemId;
#ifdef COMBO_BUILD
    if (id == RI_COMBO_FOREIGN) {
        const auto* foreign = Rando::MiscBehavior::MM_LookupForeign(check);
        if (!foreign || foreign->itemName.empty())
            return {};
        return ForeignRewardName(*foreign);
    }
#endif
    const auto item = Rando::StaticData::Items.find(id);
    return id != RI_UNKNOWN && item != Rando::StaticData::Items.end() && item->second.name ? item->second.name
                                                                                           : std::string{};
}

int MapCompassDungeon(RandoItemId id, bool& compass) {
    compass = true;
    switch (id) {
        case RI_WOODFALL_COMPASS:
            return 0;
        case RI_SNOWHEAD_COMPASS:
            return 1;
        case RI_GREAT_BAY_COMPASS:
            return 2;
        case RI_STONE_TOWER_COMPASS:
            return 3;
        default:
            break;
    }
    compass = false;
    switch (id) {
        case RI_WOODFALL_MAP:
            return 0;
        case RI_SNOWHEAD_MAP:
            return 1;
        case RI_GREAT_BAY_MAP:
            return 2;
        case RI_STONE_TOWER_MAP:
            return 3;
        default:
            return -1;
    }
}

// ConvertItem already resolved these identities using MM's save. Never ask the
// donor to choose a tier again: its state may differ or already include this grant.
const char* ConcreteReceiptName(RandoItemId id) {
    switch (id) {
        case RI_SINGLE_MAGIC:
            return "Magic Meter";
        case RI_DOUBLE_MAGIC:
            return "Enhanced Magic Meter";
        // These FC chains count identical keys, rather than equipment tiers.
        // Each local key already has the concrete identity the donor needs.
        case RI_OOT_SMALL_KEY_BOTTOM_OF_THE_WELL:
            return "Bottom of the Well Small Key";
        case RI_OOT_SMALL_KEY_FIRE_TEMPLE:
            return "Fire Temple Small Key";
        case RI_OOT_SMALL_KEY_FOREST_TEMPLE:
            return "Forest Temple Small Key";
        case RI_OOT_SMALL_KEY_GANONS_CASTLE:
            return "Ganon's Castle Small Key";
        case RI_OOT_SMALL_KEY_GERUDO_FORTRESS:
            return "Gerudo Fortress Small Key";
        case RI_OOT_SMALL_KEY_GERUDO_TRAINING_GROUND:
            return "Training Ground Small Key";
        case RI_OOT_SMALL_KEY_SHADOW_TEMPLE:
            return "Shadow Temple Small Key";
        case RI_OOT_SMALL_KEY_SPIRIT_TEMPLE:
            return "Spirit Temple Small Key";
        case RI_OOT_SMALL_KEY_TREASURE_GAME:
            return "Chest Game Small Key";
        case RI_OOT_SMALL_KEY_WATER_TEMPLE:
            return "Water Temple Small Key";
        // Shared songs still need MM's description of their use in Termina.
        // These are concrete identities, including each Goron Lullaby tier.
        case RI_SONG_SONATA:
            return "Sonata of Awakening";
        case RI_SONG_LULLABY:
            return "Goron Lullaby";
        case RI_SONG_LULLABY_INTRO:
            return "Goron Lullaby Intro";
        case RI_SONG_NOVA:
            return "New Wave Bossa Nova";
        case RI_SONG_ELEGY:
            return "Elegy of Emptiness";
        case RI_SONG_OATH:
            return "Oath to Order";
        case RI_SONG_HEALING:
            return "Song of Healing";
        case RI_SONG_SOARING:
            return "Song of Soaring";
        case RI_SONG_TIME:
            return "Song of Time (MM)";
        case RI_SONG_STORMS:
            return "Song of Storms (MM)";
        case RI_SONG_SUN:
            return "Sun's Song (MM)";
        case RI_SONG_EPONA:
            return "Epona's Song (MM)";
        case RI_SONG_SARIA:
            return "Saria's Song (MM)";
        case RI_SONG_DOUBLE_TIME:
            return "Song of Double Time";
        case RI_SONG_INVERTED_TIME:
            return "Inverted Song of Time";
        case RI_CLAWSHOT:
            return "Clawshot";
        case RI_HOOKSHOT: {
            int level = Nei_Save()->ootHookshotLevel;
            if (!level && INV_CONTENT(ITEM_HOOKSHOT) == ITEM_HOOKSHOT)
                level = 1;
            return level == 0 ? "Hookshot" : level == 1 ? "Longshot" : "Ultrashot";
        }
        case RI_FAIRY_SLINGSHOT:
            if (!Nei_Save()->slingshotOwned || Nei_BulletBagLevel() == 0)
                return "Fairy Slingshot";
            return Nei_BulletBagLevel() < 2 ? "Big Deku Seed Bullet Bag" : "Biggest Deku Seed Bullet Bag";
        case RI_OOT_PROGRESSIVE_STICK_CAPACITY: {
            const int level = CUR_UPG_VALUE(UPG_DEKU_STICKS);
            return level == 0 ? "Deku Stick Bag" : level == 1 ? "Deku Stick Capacity (20)" : "Deku Stick Capacity (30)";
        }
        case RI_OOT_PROGRESSIVE_NUT_CAPACITY: {
            const int level = CUR_UPG_VALUE(UPG_DEKU_NUTS);
            return level == 0 ? "Deku Nut Bag" : level == 1 ? "Deku Nut Capacity (30)" : "Deku Nut Capacity (40)";
        }
        case RI_OOT_HAMMER:
            return "Megaton Hammer";
        case RI_OOT_IRON_KNUCKLE_AXE:
            return "Iron Knuckle's Axe";
        case RI_OOT_MASTER_SWORD:
            return "Master Sword";
        case RI_OOT_TRUE_MASTER_SWORD:
            return "True Master Sword";
        case RI_OOT_BIGGORON_SWORD:
            return "Biggoron's Sword";
        case RI_OOT_STONE_OF_AGONY:
            return Nei_Save()->ootQuestItems & (1u << OOT_QUEST_STONE_OF_AGONY) ? "Quartz of Motion" : "Stone of Agony";
        case RI_OOT_QUARTZ_OF_MOTION:
            return "Quartz of Motion";
        case RI_OOT_GORONS_BRACELET:
            return "Goron's Bracelet";
        case RI_OOT_SILVER_GAUNTLETS:
            return "Silver Gauntlets";
        case RI_OOT_GOLDEN_GAUNTLETS:
            return "Golden Gauntlets";
        case RI_OOT_NEI_ROCS_FEATHER:
            return "Progressive Roc"; // donor's Skijer feather row
        case RI_OOT_NEI_ROCS_CAPE:
            return "Roc's Cape";
        case RI_OOT_NEI_CANE_PACCI_FLIP:
            return "Cane of Pacci";
        case RI_OOT_NEI_CANE_PACCI_STONE:
            return "Pacci Stone Skill";
        case RI_OOT_NEI_CANE_PACCI_ULTRAHAND:
            return "Ultrahand";
        case RI_OOT_NEI_CANE_SOMARIA_BLOCK:
            return "Somaria Block Skill";
        case RI_OOT_NEI_CANE_SOMARIA_PLATFORM:
            return "Somaria Platform Skill";
        default:
            return nullptr;
    }
}

void SetReceiptBody(CustomMessage::Entry& entry, std::string body) {
    if (entry.receiptPresentation.singleBox) {
        entry.icon = 0xFE; // The reward is a final-line sprite, not a header icon.
    } else {
        ComboItemReceiptText::Wrap(body, sNESFontWidths, 160, entry.icon == 0xFE ? 300.0f : 240.0f);
    }
    entry.msg = std::move(body);
    entry.autoFormat = false;
}

bool NativeReceipt(GetItemId gi, ItemId itemId, CustomMessage::Entry& entry) {
    if (!gPlayState || !gPlayState->msgCtx.messageTableNES)
        return false;
    const u16 textId = Player_GetItemReceiptTextId(gi, itemId);
    if (!textId)
        return false;
    for (const auto* text = gPlayState->msgCtx.messageTableNES; text->textId != 0xFFFF; ++text) {
        if (text->textId != textId)
            continue;
        if (!text->segment || text->msgSize <= MESSAGE_HEADER_SIZE)
            return false;
        std::string body;
        if (!ComboItemReceiptText::FromMMMessage(
                std::string_view(text->segment + MESSAGE_HEADER_SIZE, text->msgSize - MESSAGE_HEADER_SIZE), body)) {
            return false;
        }
        SetReceiptBody(entry, std::move(body));
        return true;
    }
    return false;
}
} // namespace

bool Rando::MapCompassInfoEnabled() {
#ifdef COMBO_BUILD
    using GetEnabled = int32_t (*)();
    static GetEnabled getEnabled = nullptr;
    if (!getEnabled)
        getEnabled = reinterpret_cast<GetEnabled>(Combo_ResolveSym("soh", "OOT_MapCompassInfoEnabled"));
    return gSaveContext.fileNum != 0xFF && gSaveContext.save.shipSaveInfo.saveType == SAVETYPE_RANDO && getEnabled &&
           getEnabled() != 0;
#else
    return false;
#endif
}

std::string Rando::GetDungeonMapCompassInfo(int32_t dungeon, bool compass) {
    if (dungeon < 0 || dungeon >= 4 || !MapCompassInfoEnabled())
        return {};
    const auto& info = kDungeonInformation[dungeon];
    if (!compass)
        return std::string("The entrance to ") + info.temple + " is at " + info.entrance + ".";
    const std::string reward = DungeonRewardName(dungeon);
    if (reward.empty())
        return {};
    return std::string("The boss of ") + info.temple + " is " + info.boss + ". Defeat " + info.boss + " for " + reward +
           ".";
}

#ifdef COMBO_BUILD
extern "C" COMBO_EXPORT int32_t MM_GetSeedItemIconInfo(const char* name, CwItemIconInfo* out) {
    try {
        if (!name || !out)
            return 0;
        *out = CwItemIconInfo{};
        const std::string key = ComboRando::StripGameSuffix(name);
        for (const auto& [id, item] : Rando::StaticData::Items) {
            if (id == RI_UNKNOWN || id == RI_COMBO_FOREIGN || !item.name || key != item.name)
                continue;
            out->path = Rando::StaticData::GetIconTexturePath(id);
            if (!out->path || std::strncmp(out->path, "__OTR__", 7))
                return 0;
            out->width = out->height = std::strstr(out->path, "icon_item_24_static") ? 24 : 32;
            const int song =
                id == RI_PROGRESSIVE_LULLABY ? ComboSongForMmItem(RI_SONG_LULLABY) : ComboSongForMmItem(id);
            if (song >= 0 || std::strstr(out->path, "SongNote")) {
                out->width = 16;
                out->height = 24;
                out->isIA8 = 1;
                out->hasColor = ComboSongShimmerColor(song, out->color);
            } else if (std::strstr(out->path, "gOcarinaBtnIcon")) {
                out->width = out->height = 16;
                out->isIA8 = 1;
            } else if (std::strstr(out->path, "gHeartPieceIcon")) {
                out->width = out->height = 48;
                out->isIA8 = 1;
            }
            return 1;
        }
        return 0;
    } catch (...) { return 0; }
}

extern "C" COMBO_EXPORT int32_t MM_GetDungeonRewardIconInfo(int32_t dungeon, CwItemIconInfo* out) {
    try {
        if (!out || dungeon < 0 || dungeon >= 4)
            return 0;
        *out = CwItemIconInfo{};
        RewardIdentity reward;
        if (!ComboRando::g_comboForeignJson.empty()) {
            reward = LoadedSeedDungeonReward(dungeon);
        } else {
            if (gSaveContext.fileNum == 0xFF || gSaveContext.save.shipSaveInfo.saveType != SAVETYPE_RANDO)
                return 0;
            const auto check = kDungeonInformation[dungeon].reward;
            const auto id = gSaveContext.save.shipSaveInfo.rando.randoSaveChecks[check].randoItemId;
            if (id == RI_COMBO_FOREIGN) {
                const auto* foreign = Rando::MiscBehavior::MM_LookupForeign(check);
                if (foreign)
                    reward = { ComboRando::StripGameSuffix(foreign->itemName), foreign->itemGame, true,
                               foreign->shared };
            } else {
                const auto item = Rando::StaticData::Items.find(id);
                if (id != RI_UNKNOWN && item != Rando::StaticData::Items.end() && item->second.name)
                    reward.name = item->second.name;
            }
        }
        if (reward.name.empty())
            return 0;
        if (reward.game == ComboRando::GAME_OOT) {
            auto getIcon = reinterpret_cast<Fn_GetItemIconInfo>(Combo_ResolveSym("soh", "OOT_GetSeedItemIconInfo"));
            if (!getIcon || getIcon(reward.name.c_str(), out) != 1)
                return 0;
            // Preserve the owner in the path when this result is consumed in OoT.
            static std::unordered_map<std::string, std::string> paths;
            CwItemReceiptPresentation p{};
            p.singleBox = 1;
            p.rewardLine = 2;
            if (!ComboReceipt_CopyIcon(&p, out, "oot"))
                return 0;
            out->path = paths.emplace(p.iconPath, p.iconPath).first->second.c_str();
            return 1;
        }
        return MM_GetSeedItemIconInfo(reward.name.c_str(), out);
    } catch (...) { return 0; }
}

extern "C" COMBO_EXPORT int32_t MM_GetDungeonRewardName(int32_t dungeon, char* buffer, uint32_t capacity) {
    if (!buffer || !capacity)
        return 0;
    buffer[0] = '\0';
    try {
        const std::string name = DungeonRewardName(dungeon);
        if (name.empty() || name.size() >= capacity)
            return 0;
        std::memcpy(buffer, name.c_str(), name.size() + 1);
        return static_cast<int32_t>(name.size());
    } catch (...) { return 0; }
}

bool Rando::ApplyForeignItemReceiptText(const char* itemName, CustomMessage::Entry& entry, RandoCheckId check) {
    entry.capeVisibilityChoice = ComboCapeReceiptChoice::IsCape(itemName);
    if (!itemName || !*itemName)
        return false;
    const auto keyBody = ComboDungeonKeyReceipt::Markup(itemName);
    if (!keyBody.empty()) {
        entry.receiptPresentation = {};
        SetReceiptBody(entry, ComboItemReceiptText::FromNeiMarkup(keyBody));
        return true;
    }
    // Capture dynamic descriptions before the cross grant. Cycle recollection
    // must keep that receipt's counters as well as its resolved item identity.
    struct SavedReceipt {
        std::string body;
        CwItemReceiptPresentation presentation{};
    };
    static std::unordered_map<int, SavedReceipt> bodies;
    static int slot = -1;
    static uint64_t generation = static_cast<uint64_t>(-1);
    const uint64_t currentGeneration = MiscBehavior::ComboRandoGen();
    if (slot != gSaveContext.fileNum || generation != currentGeneration) {
        bodies.clear();
        slot = gSaveContext.fileNum;
        generation = currentGeneration;
    }
    if (check != RC_UNKNOWN) {
        const auto saved = bodies.find(check);
        if (saved != bodies.end()) {
            entry.receiptPresentation = saved->second.presentation;
            SetReceiptBody(entry, saved->second.body);
            return true;
        }
    }
    static Fn_OOT_GetItemReceiptText getText = nullptr;
    if (!getText)
        getText = reinterpret_cast<Fn_OOT_GetItemReceiptText>(Combo_ResolveSym("soh", "OOT_GetItemReceiptText"));
    if (!getText || !itemName || !*itemName)
        return false;
    char buffer[BUFFER_SIZE - MESSAGE_HEADER_SIZE]{};
    const int32_t size = getText(itemName, buffer, sizeof(buffer));
    if (size <= 0 || static_cast<size_t>(size) > sizeof(buffer))
        return false;
    std::string body(buffer, size); // explicit length retains color byte 0
    entry.receiptPresentation = {};
    static Fn_GetDungeonItemReceiptPresentation getPresentation = nullptr;
    if (!getPresentation)
        getPresentation = reinterpret_cast<Fn_GetDungeonItemReceiptPresentation>(
            Combo_ResolveSym("soh", "OOT_GetDungeonItemReceiptPresentation"));
    if (getPresentation && getPresentation(itemName, &entry.receiptPresentation) != 1)
        entry.receiptPresentation = {};
    if (check != RC_UNKNOWN)
        bodies[check] = { body, entry.receiptPresentation };
    SetReceiptBody(entry, std::move(body));
    return true;
}
#endif

bool Rando::ApplyItemReceiptText(RandoItemId id, CustomMessage::Entry& entry) {
    entry.capeVisibilityChoice = id == RI_OOT_EXT_MAGIC_CAPE;
    entry.receiptPresentation = {};
    if (id == RI_TRAP)
        return false;
    const auto it = StaticData::Items.find(id);
    if (it == StaticData::Items.end())
        return false;
    const auto& item = it->second;
    const auto keyBody = item.name ? ComboDungeonKeyReceipt::Markup(item.name) : std::string{};
    if (!keyBody.empty()) {
        SetReceiptBody(entry, ComboItemReceiptText::FromNeiMarkup(keyBody));
        return true;
    }
    bool compass;
    const int dungeon = MapCompassDungeon(id, compass);
    if (dungeon >= 0) {
        if (MapCompassInfoEnabled()) {
            entry.receiptPresentation.singleBox = 1;
            entry.receiptPresentation.rewardLine = 2;
            std::string body = (compass ? "You received a %g" : "You found the %g") + std::string(item.name) + "%w!";
            if (compass) {
                body += "&It points to %r" + std::string(kDungeonInformation[dungeon].boss) + "%w";
#ifdef COMBO_BUILD
                CwItemIconInfo icon{};
                if (MM_GetDungeonRewardIconInfo(dungeon, &icon) == 1)
                    ComboReceipt_CopyIcon(&entry.receiptPresentation, &icon, "mm");
#endif
                entry.receiptPresentation.rewardLine = 1;
            } else {
                body += "&It seems the entrance is at %c" + std::string(kDungeonInformation[dungeon].entrance) + "%w.";
            }
            SetReceiptBody(entry, ComboItemReceiptText::FromNeiMarkup(body));
            return true;
        }
    }
    if (item.randoItemType == RITYPE_SKULLTULA_TOKEN) {
        // CheckQueue composes this before GiveItem. Native MESSAGE_TOKENS
        // instead reads the current scene, which is wrong for shuffled tokens.
        unsigned count;
        std::string place;
        switch (id) {
            case RI_GS_TOKEN_SWAMP:
                count = (gSaveContext.save.saveInfo.skullTokenCount >> 16) + 1;
                place = " in the Swamp Spider House";
                break;
            case RI_GS_TOKEN_OCEAN:
                count = (gSaveContext.save.saveInfo.skullTokenCount & 0xFFFF) + 1;
                place = " in the Ocean Spider House";
                break;
            case RI_OOT_GS_TOKEN:
                // The FC total includes newly found tokens pending the next
                // shared-state projection into MM's OoT quest-page counter.
                count = std::max<unsigned>(Nei_Save()->ootGsCount, Nei_Save()->comboObtainedFc[FCI_OOT_GS_TOKEN]) + 1;
                place = " in Hyrule";
                break;
            default:
                return false;
        }
        const std::string article = item.article && *item.article ? std::string(item.article) + " " : "";
        const std::string color = id == RI_OOT_GS_TOKEN ? "%r" : "%c";
        SetReceiptBody(entry, ComboItemReceiptText::FromNeiMarkup(
                                  "You got " + article + color + item.name + "%w!^You've collected " + color +
                                  std::to_string(count) + "%w " + (count == 1 ? "token" : "tokens") + place + "."));
        return true;
    }
    // A junk reward still owns a native receipt. Prefer that brief body over
    // the queue's generic name/icon text, then allow imported ammo/refills to
    // use their donor or local registry when MM has no matching native GI.
    if (item.randoItemType == RITYPE_JUNK && NativeReceipt(item.getItemId, item.itemId, entry))
        return true;
    if (item.randoItemType != RITYPE_MAJOR && item.randoItemType != RITYPE_MASK &&
        item.randoItemType != RITYPE_LESSER && item.randoItemType != RITYPE_HEALTH && item.randoItemType != RITYPE_JUNK)
        return false;
    const char* concrete = ConcreteReceiptName(id);
    // Cross-game text export carries English bytes only. MM owns the meaning
    // and locale of its songs, including shared songs and both lullaby tiers.
    if (concrete) {
        if (const auto* song = ComboSongReceiptText::Find(concrete)) {
            const char* text = gSaveContext.options.language == LANGUAGE_GER   ? song->german
                               : gSaveContext.options.language == LANGUAGE_FRE ? song->french
                                                                               : song->english;
            SetReceiptBody(entry, ComboItemReceiptText::FromNeiMarkup(text));
            return true;
        }
    }
#ifdef COMBO_BUILD
    if (concrete && ApplyForeignItemReceiptText(concrete, entry))
        return true;
    // Single-identity catalog rows can use the full OoT description directly.
    // Progressive pool rows need concrete mapping above or MM's own native text.
    const int fc = FcCombo_ItemForNative(id);
    if (!concrete && fc >= 0 && fc < FC_COMBO_ITEM_COUNT && gFcComboItems[fc].chainLen == 1 &&
        *gFcComboItems[fc].ootName && ApplyForeignItemReceiptText(gFcComboItems[fc].ootName, entry))
        return true;
#endif
    // MM's registry uses RI IDs in its rg column, including rows with ITEM_NONE
    // in StaticData. This lookup also works in standalone 2Ship without OoT.
    const NeiItem* nei = id == RI_OOT_NEI_ROCS_CAPE ? Nei_FindByItem(ITEM_ROCS_CAPE) : Nei_FindByRg(id);
    if (!nei && item.itemId != ITEM_NONE && !concrete)
        nei = Nei_FindByItem(item.itemId);
    if (nei && nei->nameEn && *nei->nameEn) {
        SetReceiptBody(entry, ComboItemReceiptText::FromNeiMarkup(nei->nameEn));
        return true;
    }
    // Clawshot and upper Hookshot tiers reuse a vanilla GI/model but aren't that
    // vanilla item. A missing donor must fall back safely, not describe Hookshot.
    if (id == RI_CLAWSHOT || (id == RI_HOOKSHOT && std::string_view(concrete) != "Hookshot"))
        return false;
    if (NativeReceipt(item.getItemId, item.itemId, entry))
        return true;
    return false;
}

void Rando::AppendReceiptSource(CustomMessage::Entry& entry, const std::string& source) {
    if (entry.autoFormat) {
        entry.msg += source;
        return;
    }
    if (!source.empty())
        entry.msg += "\x10" + source;
    // The queue/custom-item path needs a normal END; vanilla EVENT, FADE and
    // next-textbox behavior must never escape into this receipt.
    entry.msg += '\xBF';
}
