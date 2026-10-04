#include "ItemReceiptText.h"
#include "2s2h/Rando/StaticData/StaticData.h"
#include "2s2h/FleetShipCombo/FleetComboItemsGlue.h"
#include "2s2h/FleetShipCombo/FleetComboItems.h"
#include "ComboItemReceiptText.h"
#ifdef COMBO_BUILD
#include "ComboResolve.h"
#include <unordered_map>
namespace Rando::MiscBehavior {
uint64_t ComboRandoGen();
}
#endif

extern "C" {
#include "mods/extended_inventory.h"
#include "message_data_static.h"
extern float sNESFontWidths[160];
u16 Player_GetItemReceiptTextId(s16 getItemId, s16 itemId);
}

namespace {
// ConvertItem already resolved these identities using MM's save. Never ask the
// donor to choose a tier again: its state may differ or already include this grant.
const char* ConcreteReceiptName(RandoItemId id) {
    switch (id) {
        case RI_SINGLE_MAGIC:
            return "Magic Meter";
        case RI_DOUBLE_MAGIC:
            return "Enhanced Magic Meter";
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
    ComboItemReceiptText::Wrap(body, sNESFontWidths, 160, entry.icon == 0xFE ? 300.0f : 240.0f);
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

#ifdef COMBO_BUILD
bool Rando::ApplyForeignItemReceiptText(const char* itemName, CustomMessage::Entry& entry, RandoCheckId check) {
    // Capture dynamic descriptions before the cross grant. Cycle recollection
    // must keep that receipt's counters as well as its resolved item identity.
    static std::unordered_map<int, std::string> bodies;
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
            SetReceiptBody(entry, saved->second);
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
    if (check != RC_UNKNOWN)
        bodies[check] = body;
    SetReceiptBody(entry, std::move(body));
    return true;
}
#endif

bool Rando::ApplyItemReceiptText(RandoItemId id, CustomMessage::Entry& entry) {
    if (id == RI_TRAP)
        return false;
    const auto it = StaticData::Items.find(id);
    if (it == StaticData::Items.end())
        return false;
    const auto& item = it->second;
    if (item.randoItemType != RITYPE_MAJOR && item.randoItemType != RITYPE_MASK &&
        item.randoItemType != RITYPE_LESSER && item.randoItemType != RITYPE_HEALTH)
        return false;
    const char* concrete = ConcreteReceiptName(id);
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
