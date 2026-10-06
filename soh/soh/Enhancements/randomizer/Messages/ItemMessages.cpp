/**
 * This file handles custom messages relating to Items,
 * such as Get Item messages for non-vanilla items,
 * Vanilla/MQ hints when collecting Maps, Ice Trap messages,
 * etc.
 */
#include <soh/OTRGlobals.h>
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/Enhancements/custom-message/CustomMessageTypes.h"
#include "soh/Enhancements/randomizer/Traps.h"
#include "soh/Enhancements/randomizer/item.h"
#include "soh/Enhancements/randomizer/dungeon.h"
#include "soh/Enhancements/randomizer/randomizer.h"
#include "soh/Enhancements/randomizer/randostatupgrade.h"
#include "soh/Enhancements/randomizer/randomizer_entrance.h"
#include "soh/Enhancements/randomizer/entrance.h"
#include "soh/Enhancements/randomizer/randomizer_entrance_tracker.h"
#include "soh/FleetShipCombo/FleetComboIds.h"
#include "ComboSongDrawOOT.h"
#include "ComboItemReceiptPresentation.h"
#include "soh/ShipInit.hpp"
#include <soh/ResourceManagerHelpers.h>
#ifdef COMBO_BUILD
#include "ComboExport.h"
#include "ComboResolve.h"
#include "ComboItemReceiptText.h"
#include "message_data_static.h"
#include "rando/CrossForeign.h"
#include "soh/Enhancements/randomizer/hook_handlers.h"
#include "soh/Enhancements/randomizer/draw.h"
extern "C" COMBO_EXPORT int32_t OOT_GetSeedItemIconInfo(const char* itemName, CwItemIconInfo* out);
extern "C" int gComboGoalRequired;
extern "C" int (*gComboOtherTriforceCount)(void);
#endif

#include <cstdarg>
#include <algorithm>
#include <cstring>

extern "C" {
#include "variables.h"
#include "macros.h"
#include "functions.h"
#include "z64item.h"
#include "mods/extended_inventory.h"
extern PlayState* gPlayState;
extern u8 gLanternCatchPending; // item_lantern.c — fire type pending message display
}

// Forward declaration for custom item messages from randomizer.cpp
struct CustomItemMessageEntry {
    s16 rgId;
    ItemID itemId;
    const char* english;
    const char* german;
    const char* french;
};
extern const CustomItemMessageEntry* GetCustomItemMessage(s16 rgId);

bool BuildDungeonItemReceiptMessage(RandomizerGet rg, CustomMessage& msg, bool received = true);
bool BuildTokenReceiptMessage(RandomizerGet rg, CustomMessage& msg);

static bool DungeonInformationEnabled() {
    if (!OTRGlobals::Instance || !OTRGlobals::Instance->gRandoContext || !OTRGlobals::Instance->gRandomizer)
        return false;
    const auto ctx = OTRGlobals::Instance->gRandoContext;
    // A Combo seed can start in MM before OoT creates a PlayState or hydrates
    // its save. The generated/loaded seed context is already authoritative.
    return (IS_RANDO || ctx->IsSeedGenerated() || ctx->IsSpoilerLoaded()) &&
           ctx->GetOption(RSK_MAPS_COMPASSES_GIVE_INFORMATION).Is(RO_GENERIC_ON);
}

#ifdef COMBO_BUILD
extern "C" COMBO_EXPORT int32_t OOT_MapCompassInfoEnabled(void) {
    return DungeonInformationEnabled();
}

extern "C" MessageTableEntry* sNesMessageEntryTablePtr;
void BuildQuarterHeartMessage(CustomMessage& msg);
void BuildDefenseUpgradeMessage(CustomMessage& msg);
void BuildSpeedUpgradeMessage(CustomMessage& msg);
void BuildPowerUpgradeMessage(CustomMessage& msg);
void BuildMagicStatUpgradeMessage(CustomMessage& msg);
void BuildCrawlSpeedUpgradeMessage(CustomMessage& msg);
void BuildClimbSpeedUpgradeMessage(CustomMessage& msg);
void BuildPushSpeedUpgradeMessage(CustomMessage& msg);

// Read-only counterpart to OOT_GetItemDrawInfo. The caller supplies its own
// buffer; no C++ object, donor pointer, icon ID or story state crosses the ABI.
extern "C" COMBO_EXPORT int32_t OOT_GetItemReceiptText(const char* itemName, char* buffer, uint32_t capacity) {
    try {
        if (!itemName || !buffer || !capacity || !OTRGlobals::Instance || !OTRGlobals::Instance->gRandoContext ||
            !OTRGlobals::Instance->gRandomizer)
            return 0;
        const auto found = Rando::StaticData::itemNameToEnum.find(itemName);
        if (found == Rando::StaticData::itemNameToEnum.end() || found->second == RG_NONE ||
            found->second == RG_COMBO_FOREIGN || found->second == RG_ICE_TRAP)
            return 0;
        const RandomizerGet rg = found->second;
        auto item = Rando::StaticData::RetrieveItem(rg);
        // Consumables/traps retain their normal brief receipts.
        if (item.GetCategory() == ITEM_CATEGORY_JUNK)
            return 0;
        std::string body;
        // Randomizer dungeon keys point at a generic custom message, but MM
        // still needs the full traditional OoT tutorial. Native song receipt
        // IDs are safe here; teaching/cutscene text is never requested.
        uint16_t traditionalText = 0;
        if (rg >= RG_DEKU_TREE_MAP && rg <= RG_ICE_CAVERN_MAP)
            traditionalText = 0x66;
        else if (rg >= RG_DEKU_TREE_COMPASS && rg <= RG_ICE_CAVERN_COMPASS)
            traditionalText = 0x67;
        else if (rg >= RG_FOREST_TEMPLE_SMALL_KEY && rg <= RG_TREASURE_GAME_SMALL_KEY)
            traditionalText = rg == RG_TREASURE_GAME_SMALL_KEY ? 0xF3 : 0x60;
        else if (rg >= RG_FOREST_TEMPLE_BOSS_KEY && rg <= RG_GANONS_CASTLE_BOSS_KEY)
            traditionalText = 0xC7;
        else if (rg >= RG_ZELDAS_LULLABY && rg <= RG_PRELUDE_OF_LIGHT) {
            if (const auto gi = item.GetGIEntryUnresolved())
                traditionalText = gi->textId;
        }
        if (traditionalText && sNesMessageEntryTablePtr &&
            !((rg >= RG_DEKU_TREE_MAP && rg <= RG_ICE_CAVERN_COMPASS) && DungeonInformationEnabled())) {
            for (const auto* text = sNesMessageEntryTablePtr; text->textId != 0xFFFF; ++text) {
                if (text->textId != traditionalText)
                    continue;
                if (!text->segment ||
                    !ComboItemReceiptText::FromOotMessage(std::string_view(text->segment, text->msgSize), body))
                    return 0;
                break;
            }
        }
        void (*builder)(CustomMessage&) = nullptr;
        switch (rg) {
            case RG_QUARTER_HEART:
                builder = BuildQuarterHeartMessage;
                break;
            case RG_DEFENSE_UPGRADE:
                builder = BuildDefenseUpgradeMessage;
                break;
            case RG_SPEED_UPGRADE:
                builder = BuildSpeedUpgradeMessage;
                break;
            case RG_POWER_UPGRADE:
                builder = BuildPowerUpgradeMessage;
                break;
            case RG_MAGIC_STAT_UPGRADE:
                builder = BuildMagicStatUpgradeMessage;
                break;
            case RG_CRAWL_SPEED_UPGRADE:
                builder = BuildCrawlSpeedUpgradeMessage;
                break;
            case RG_CLIMB_SPEED_UPGRADE:
                builder = BuildClimbSpeedUpgradeMessage;
                break;
            case RG_PUSH_SPEED_UPGRADE:
                builder = BuildPushSpeedUpgradeMessage;
                break;
            default:
                break;
        }
        CustomMessage contextual;
        if (BuildDungeonItemReceiptMessage(rg, contextual, body.empty()) || BuildTokenReceiptMessage(rg, contextual)) {
            std::string information;
            if (!ComboItemReceiptText::FromOotMessage(contextual.GetEnglish(MF_RAW), information))
                return 0;
            if (!body.empty() && !information.empty())
                body += '\x10';
            body += information;
        } else if (!body.empty()) {
            // Already selected the complete native body above.
        } else if (builder) {
            CustomMessage message;
            builder(message); // same read-only description builder as OoT's own receipt
            if (!ComboItemReceiptText::FromOotMessage(message.GetEnglish(MF_RAW), body))
                return 0;
        } else {
            const auto* custom = GetCustomItemMessage(rg);
            if (custom && custom->english && *custom->english) {
                body = ComboItemReceiptText::FromNeiMarkup(custom->english);
            } else {
                const auto gi = item.GetGIEntryUnresolved();
                if (!gi)
                    return 0;
                if (gi->textId != TEXT_RANDOMIZER_CUSTOM_ITEM && sNesMessageEntryTablePtr) {
                    for (const auto* text = sNesMessageEntryTablePtr; text->textId != 0xFFFF; ++text) {
                        if (text->textId != gi->textId)
                            continue;
                        if (!text->segment ||
                            !ComboItemReceiptText::FromOotMessage(std::string_view(text->segment, text->msgSize), body))
                            return 0;
                        break;
                    }
                }
            }
        }
        if (body.empty() || body.size() > capacity)
            return 0;
        std::memcpy(buffer, body.data(), body.size());
        return static_cast<int32_t>(body.size());
    } catch (...) {
        return 0; // exceptions must not unwind into the other game module
    }
}
#endif

void BuildTriforcePieceMessage(CustomMessage& msg) {
    auto rando = OTRGlobals::Instance->gRandomizer;
#ifdef COMBO_BUILD
    // ComboShip (#136): the goal counts BOTH games' pieces, so the native per-trigger thresholds below
    // would show the wrong numbers — report combined progress instead.
    if (gComboGoalRequired > 0) {
        const int combined = gSaveContext.ship.quest.data.randomizer.triforcePiecesCollected + 1 +
                             (gComboOtherTriforceCount != NULL ? gComboOtherTriforceCount() : 0);
        if (combined >= gComboGoalRequired) {
            msg = { "You completed the %yTriforce%w! %gGG%w!", TODO_TRANSLATE, TODO_TRANSLATE };
        } else {
            msg = { "You found a %yTriforce Piece%w! %g[[current]]%w of %c[[d]]%w.", TODO_TRANSLATE, TODO_TRANSLATE };
            msg.InsertNumber(gComboGoalRequired);
        }
        msg.Replace("[[current]]", std::to_string(combined));
        msg.AutoFormat(ITEM_CUSTOM);
        return;
    }
#endif
    uint8_t current = gSaveContext.ship.quest.data.randomizer.triforcePiecesCollected + 1;
    // if any settings are off, 0 them out here as a precaution
    uint8_t bridge = rando->GetRandoSettingValue(RSK_RAINBOW_BRIDGE) == RO_BRIDGE_TRIFORCE_PIECES
                         ? rando->GetRandoSettingValue(RSK_RAINBOW_BRIDGE_TRIFORCE_COUNT)
                         : 0;
    uint8_t wincon = rando->GetRandoSettingValue(RSK_WINCON) == RO_WINCON_TRIFORCE_PIECES
                         ? rando->GetRandoSettingValue(RSK_WINCON_TRIFORCE_COUNT)
                         : 0;
    uint8_t GBK = rando->GetRandoSettingValue(RSK_GANONS_BOSS_KEY) == RO_GANON_BOSS_KEY_TRIFORCE_PIECES
                      ? rando->GetRandoSettingValue(RSK_GBK_TRIFORCE_COUNT)
                      : 0;
    uint8_t soul = rando->GetRandoSettingValue(RSK_GANONS_SOUL) == RO_GANONS_SOUL_TRIFORCE_PIECES
                       ? rando->GetRandoSettingValue(RSK_GANONS_SOUL_TRIFORCE_COUNT)
                       : 0;

    // If we reach wincon, we win!
    if (current == wincon) {
        msg = { "You completed the %yTriforce of Courage%w! %gGG%w!",
                "Das %yTriforce des Mutes%w! Du hast alle Splitter gefunden. %gGut gemacht%w!",
                "Vous avez complété la %yTriforce du Courage%w! %gFélicitations%w!" };
        // otherwise prioritise the different triggers
    } else if (current == bridge) {
        msg = { "You made your wish to the %yTriforce%w! %rTh%ye R%gai%cnb%bow %pBr%rid%yge %gha%cs r%bai%psed%w!",
                TODO_TRANSLATE, TODO_TRANSLATE };
    } else if (current == GBK) {
        msg = { "You completed the %yTriforce of Power%w! %rThe Key to Evil is yours%w!", TODO_TRANSLATE,
                TODO_TRANSLATE };
    } else if (current == soul) {
        msg = { "You completed the %yTriforce of Wisdom%w! %bGanon's soul is reclaimed%w!", TODO_TRANSLATE,
                TODO_TRANSLATE };
        // if everything is zero, then there's no goal...
    } else if (bridge + wincon + GBK + soul == 0) {
        msg = { "You found a %yTriforce Piece%w! But it's %puseless%w...", TODO_TRANSLATE, TODO_TRANSLATE };
    } else {
        // if nothing is complete, we need to check is we have more than we need
        uint8_t highest = std::max({ current, bridge, wincon, GBK, soul });
        if (highest == current) {
            // RANDOTODO TODO_TRANSLATE you could maybe make this sound cleaner because InsertNumber allows for dynamic
            // plurals
            msg = { "You found a spare %yTriforce Piece%w! You only needed %c[[d]]%w, but you have %g[[current]]%w!",
                    "Ein übriger %yTriforce-Splitter%w! Du hast nun %g[[current]]%w von %c[[d]]%w nötigen gefunden.",
                    "Vous avez trouvé un %yFragment de Triforce%w en plus! Vous n'aviez besoin que de %c[[d]]%w, "
                    "mais vous en avez %g[[current]]%w en tout!" };
            msg.InsertNumber(std::max({ bridge, wincon, GBK, soul }));
        } else {
            // find the next goal by setting everything below current (including failed conditions set to 0 before)
            // to a high number, then looking for the lowest.
            // if we have the exact amount, it will be caught by the first check, so no worries there
            if (bridge < current) {
                bridge = 255;
            }
            if (GBK < current) {
                GBK = 255;
            }
            if (soul < current) {
                soul = 255;
            }
            if (wincon < current) {
                wincon = 255;
            }
            uint8_t next = std::min({ bridge, GBK, soul, wincon });

            uint8_t remaining = next - current;
            float percentageCollected = (float)current / (float)next;

            if (percentageCollected <= 0.25) {
                msg = { "You found a %yTriforce Piece%w! %g[[current]]%w down, %c[[d]]%w more and you [[condition]]! "
                        "It's a start!",
                        TODO_TRANSLATE, TODO_TRANSLATE };
            } else if (percentageCollected <= 0.5) {
                msg = { "You found a %yTriforce Piece%w! that makes %g[[current]]%w, %c[[d]]%w to go until you "
                        "[[condition]]! Progress!",
                        TODO_TRANSLATE, TODO_TRANSLATE };
            } else if (percentageCollected <= 0.75) {
                msg = { "You found a %yTriforce Piece%w! You have %g[[current]]%w and need %c[[d]]%w more and you "
                        "[[condition]]! Over half-way there!",
                        TODO_TRANSLATE, TODO_TRANSLATE };
            } else if (percentageCollected < 1.0) {
                msg = { "You found a %yTriforce Piece%w! %g[[current]]%w down, %c[[d]]%w left until you [[condition]]! "
                        "Almost done!",
                        TODO_TRANSLATE, TODO_TRANSLATE };
            }

            // default condition is soul
            CustomMessage condition = { "%brelease Ganons Soul%w", TODO_TRANSLATE, TODO_TRANSLATE };
            if (next == wincon) {
                condition = { "%gWin the game%w", TODO_TRANSLATE, TODO_TRANSLATE };
            } else if (next == bridge) {
                condition = { "%csummon the Rainbow Bridge%w", TODO_TRANSLATE, TODO_TRANSLATE };
            } else if (next == GBK) {
                condition = { "%rfind the key to Ganondorf's Lair%w", TODO_TRANSLATE, TODO_TRANSLATE };
            }
            msg.Replace("[[condition]]", condition);
            msg.InsertNumber(remaining);
        }
    }
    msg.Replace("[[current]]", std::to_string(current));
    msg.AutoFormat(ITEM_CUSTOM);
}

void BuildTriforceMessage(CustomMessage& msg) {
    msg = { "You completed the %yTriforce of&Courage%w! %gGG%w!",
            "Das %yTriforce des Mutes%w! Du hast&alle Splitter gefunden. %gGut gemacht%w!",
            "Vous avez complété la %yTriforce&du Courage%w! %gFélicitations%w!" };
    msg.Format(ITEM_CUSTOM);
}

void BuildCustomItemMessage(Player* player, CustomMessage& msg) {
    int16_t rgid;
    if (player->getItemEntry.objectId != OBJECT_INVALID) {
        rgid = player->getItemEntry.getItemId;
    } else {
        rgid = player->getItemId;
    }

    if (BuildDungeonItemReceiptMessage(static_cast<RandomizerGet>(rgid), msg) ||
        BuildTokenReceiptMessage(static_cast<RandomizerGet>(rgid), msg)) {
        return;
    }

    // Check if this is a custom item with a detailed message
    const CustomItemMessageEntry* customMsg = GetCustomItemMessage(rgid);
    if (customMsg != nullptr) {
        // Use the detailed custom message. Pass the real ItemID so Message_LoadItemIcon's
        // ">= ITEM_ROCS_FEATHER_SKIJER" branch fires (z_message_PAL.c:1671) and loads the
        // 32x32 icon via ExtInv_GetItemIcon(itemId). Without this, AutoFormat() with no
        // argument leaves the message without an ITEM_OBTAINED token at all, and the
        // textbox renders with no icon on the left.
        msg = CustomMessage(customMsg->english, customMsg->german, customMsg->french, TEXTBOX_TYPE_BLUE);
        msg.AutoFormat(customMsg->itemId);
        return;
    }

    // Fall back to generic "You found X!" message for other items
    msg = CustomMessage("You found [[article]][[color]][[name]]%w!", "Du hast [[article]][[color]][[name]]%w gefunden!",
                        "Vous avez trouvé [[article]][[color]][[name]]%w!", TEXTBOX_TYPE_BLUE);
    CustomMessage name =
        CustomMessage(Rando::StaticData::RetrieveItem(static_cast<RandomizerGet>(rgid)).GetName(), TEXTBOX_TYPE_BLUE);
    if (rgid == RG_OPEN_CHEST &&
        OTRGlobals::Instance->gRandoContext->GetOption(RSK_SHUFFLE_OPEN_CHEST).Is(RO_OPEN_CHEST_PROGRESSIVE)) {
        // message is built before the item is given, so the flags still say which copy this is
        name = Flags_GetRandomizerInf(RAND_INF_CAN_OPEN_CHEST)
                   ? CustomMessage("Open Big Chests", "Große Truhen öffnen", "Ouvrir les grands coffres",
                                   TEXTBOX_TYPE_BLUE)
                   : CustomMessage("Open Small Chests", "Kleine Truhen öffnen", "Ouvrir les petits coffres",
                                   TEXTBOX_TYPE_BLUE);
    }
    CustomMessage article = CustomMessage(
        Rando::StaticData::RetrieveItem(static_cast<RandomizerGet>(rgid)).GetArticle(), TEXTBOX_TYPE_BLUE);
    msg.Replace("[[article]]", article);
    msg.Replace("[[color]]", Rando::StaticData::RetrieveItem(static_cast<RandomizerGet>(rgid)).GetColor());
    msg.Replace("[[name]]", name);
    if (Rando::StaticData::RetrieveItem(static_cast<RandomizerGet>(rgid)).HasCustomIcon()) {
        // Use the real ItemID from the item table so vanilla's Message_LoadItemIcon picks
        // up the ">= ITEM_ROCS_FEATHER_SKIJER" branch and resolves via ExtInv_GetItemIcon.
        ItemID itemId =
            static_cast<ItemID>(Rando::StaticData::RetrieveItem(static_cast<RandomizerGet>(rgid)).GetItemID());
        msg.AutoFormat(itemId);
    } else {
        // No custom icon: AutoFormat() with no argument inserts no item-icon token, so the textbox
        // renders with NO icon on the left. For a plain vanilla item (bomb bag, quiver, hover boots,
        // tunics...) that is just a missing icon, and its real one is one lookup away: pass
        // giEntry->itemId — the actual ItemID, NOT GetItemID() which returns the get-item id.
        //
        // Bounded on purpose. Many MM-port rows are built with a RandomizerGet in the itemId slot
        // (see RG_MM_SONG_SONATA), which is far past the end of gItemIcons; handing that to
        // Message_LoadItemIcon would take the custom-item branch and memcpy from a NULL icon.
        // Below ITEM_ROCS_FEATHER_SKIJER is exactly the vanilla range, and everything custom
        // already went through the HasCustomIcon path above. Anything else stays iconless — an
        // empty textbox beats a wrong or invented icon (Skijer's call). Skijer's NEI
        auto gi = Rando::StaticData::RetrieveItem(static_cast<RandomizerGet>(rgid)).GetGIEntry();
        if (gi != nullptr && gi->itemId != ITEM_NONE && gi->itemId < ITEM_ROCS_FEATHER_SKIJER) {
            msg.AutoFormat(static_cast<ItemID>(gi->itemId));
        } else {
            msg.AutoFormat();
        }
    }
}

void LoadCustomItemIcon(bool displayAsEnglish) {
    Player* player = GET_PLAYER(gPlayState);
    const char* customIcon = nullptr;
    CustomIconSize iconSize = ICON_SIZE_32;
    // Same rule as the hooks above: getItemId is only an RG on MOD_RANDOMIZER entries.
    if (player->getItemEntry.objectId != OBJECT_INVALID && player->getItemEntry.modIndex == MOD_RANDOMIZER) {
        RandomizerGet rgid = static_cast<RandomizerGet>(player->getItemEntry.getItemId);
        customIcon = Rando::StaticData::RetrieveItem(rgid).GetCustomIcon();
        iconSize = Rando::StaticData::RetrieveItem(rgid).GetCustomIconSize();
    } else if (player->getItemEntry.objectId != OBJECT_INVALID) {
        customIcon = nullptr; // vanilla entry: its own icon token in the message is already right
    } else {
        // if we're seeing an icon and we don't have a GI, assume we're in the alter text showing a triforce piece
        customIcon = Rando::StaticData::RetrieveItem(RG_TRIFORCE_PIECE).GetCustomIcon();
        iconSize = Rando::StaticData::RetrieveItem(RG_TRIFORCE_PIECE).GetCustomIconSize();
    }
    if (customIcon != nullptr) {
        static int16_t sIconItem32XOffsets[] = { 74, 74, 74, 54 };
        static int16_t sIconItem24XOffsets[] = { 72, 72, 72, 50 };
        MessageContext* msgCtx = &gPlayState->msgCtx;
        uint8_t language = displayAsEnglish ? LANGUAGE_ENG : (Language)gSaveContext.language;
        if (std::strstr(customIcon, "/gSongNoteTex")) {
            R_TEXTBOX_ICON_XPOS = R_TEXT_INIT_XPOS - sIconItem24XOffsets[language];
            R_TEXTBOX_ICON_YPOS = (R_TEXTBOX_Y + 10) + 10;
            R_TEXTBOX_ICON_SIZE = 16;
        } else if (iconSize == ICON_SIZE_32) {
            R_TEXTBOX_ICON_XPOS = R_TEXT_INIT_XPOS - sIconItem32XOffsets[language];
            R_TEXTBOX_ICON_YPOS = (R_TEXTBOX_Y + 10) + 6;
            R_TEXTBOX_ICON_SIZE = 32;
        } else {
            R_TEXTBOX_ICON_XPOS = R_TEXT_INIT_XPOS - sIconItem24XOffsets[language];
            R_TEXTBOX_ICON_YPOS = (R_TEXTBOX_Y + 10) + 10;
            R_TEXTBOX_ICON_SIZE = 24;
        }
        strcpy((char*)((uintptr_t)msgCtx->textboxSegment + MESSAGE_STATIC_TEX_SIZE), customIcon);
        msgCtx->msgBufPos++;
        msgCtx->choiceNum = 1;
    }
}

void DrawCustomItemIcon(Gfx** p) {
    Gfx* gfx = *p;
    MessageContext* msgCtx = &gPlayState->msgCtx;
    Player* player = GET_PLAYER(gPlayState);
    CustomIconSize iconSize = ICON_SIZE_32;
    const char* customIcon = nullptr;
    RandomizerGet rgid = RG_NONE;
    if (player->getItemEntry.objectId != OBJECT_INVALID && player->getItemEntry.modIndex == MOD_RANDOMIZER) {
        rgid = static_cast<RandomizerGet>(player->getItemEntry.getItemId);
        customIcon = Rando::StaticData::RetrieveItem(rgid).GetCustomIcon();
        iconSize = Rando::StaticData::RetrieveItem(rgid).GetCustomIconSize();
    }
    if (customIcon && std::strstr(customIcon, "/gSongNoteTex")) {
        Color_RGB8 color = { 255, 255, 255 };
        uint8_t rgba[4];
        if (ComboSongShimmerColor(ComboSongForOotItem(rgid), rgba))
            color = { rgba[0], rgba[1], rgba[2] };
        gDPSetPrimColor(gfx++, 0, 0, color.r, color.g, color.b, msgCtx->textColorAlpha);
        gDPLoadTextureBlock(gfx++, (uintptr_t)msgCtx->textboxSegment + MESSAGE_STATIC_TEX_SIZE, G_IM_FMT_IA,
                            G_IM_SIZ_8b, 16, 24, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK,
                            G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    } else if (iconSize == ICON_SIZE_24) {
        gDPLoadTextureBlock(gfx++, (uintptr_t)msgCtx->textboxSegment + MESSAGE_STATIC_TEX_SIZE, G_IM_FMT_RGBA,
                            G_IM_SIZ_32b, 24, 24, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK,
                            G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    } else {
        gDPLoadTextureBlock(gfx++, (uintptr_t)msgCtx->textboxSegment + MESSAGE_STATIC_TEX_SIZE, G_IM_FMT_RGBA,
                            G_IM_SIZ_32b, 32, 32, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK,
                            G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    }
    *p = gfx;
}

#ifdef COMBO_BUILD
// ComboShip: get-item text for a foreign check — the real MM item's display name (from the foreign
// map) instead of the "Combo Foreign Item" sentinel. Check identity rides in comboForeignCheck.
void BuildComboForeignMessage(Player* player, CustomMessage& msg) {
    std::string name = "Foreign Item";
    RandomizerCheck rc = (RandomizerCheck)player->getItemEntry.comboForeignCheck;
    if (rc != RC_UNKNOWN_CHECK) {
        const ComboRando::ForeignItem* fi =
            OOT_LookupForeign(gSaveContext.fileNum, Rando::StaticData::GetLocation(rc)->GetName());
        // ComboShip: a disguised trap keeps lying at pickup, matching OOT's native ice trap and MM's
        // side (CheckQueue reads the trick name before marking the check obtained).
        if (fi != nullptr && fi->HasDisguise() && !fi->fakeTrickName.empty()) {
            name = fi->fakeTrickName;
        } else if (fi != nullptr && !fi->displayName.empty()) {
            name = fi->displayName;
        }
        // A foreign trap taunts with OOT's own ice-trap tables, naming what it pretended to be.
        if (fi != nullptr && fi->trap) {
            Rando::Traps::BuildIceTrapMessageNamed(msg, name);
            return;
        }
        if (fi != nullptr) {
            // Freeze the tier now (pre-grant), then name it: MM's cross-grant follows this textbox.
            Randomizer_LatchComboForeign(rc);
            const char* resolved = Randomizer_ComboForeignLatchedName((int32_t)rc);
            if (resolved != nullptr) {
                name = ComboRando::ShownForeignName(*fi, resolved);
            }
            if (fi->itemGame == ComboRando::GAME_MM) {
                // Use the name frozen by the model latch, never resolve a
                // progressive against the now-changing MM save a second time.
                std::string receiptName = resolved ? resolved : fi->itemName;
                if (receiptName == "Saria's Song" || receiptName == "Epona's Song" || receiptName == "Song of Time" ||
                    receiptName == "Song of Storms" || receiptName == "Sun's Song") {
                    receiptName += " (MM)";
                }
                const auto found = Rando::StaticData::itemNameToEnum.find(receiptName);
                if (found != Rando::StaticData::itemNameToEnum.end()) {
                    if (BuildDungeonItemReceiptMessage(found->second, msg) ||
                        BuildTokenReceiptMessage(found->second, msg)) {
                        // The foreign sentinel has no local custom icon. Keep
                        // the existing iconless path, including on later pages.
                        if (!msg.receiptPresentation.singleBox)
                            for (const auto icon : { ITEM_CUSTOM, ITEM_SKULL_TOKEN, ITEM_DUNGEON_MAP, ITEM_COMPASS })
                                msg.Replace(CustomMessage::ITEM_OBTAINED(icon), "");
                        return;
                    }
                    const auto* custom = GetCustomItemMessage(found->second);
                    if (custom) {
                        msg = CustomMessage(custom->english, custom->german, custom->french, TEXTBOX_TYPE_BLUE);
                        msg.AutoFormat();
                        return;
                    }
                }
            }
        }
    }
    msg = CustomMessage("You found %g[[name]]%w!", "Du erhältst %g[[name]]%w!", "Vous avez trouvé %g[[name]]%w!",
                        TEXTBOX_TYPE_BLUE);
    msg.Replace("[[name]]", name);
    // Plain AutoFormat (no ITEM_CUSTOM): the sentinel has no custom icon, so the icon control code
    // would render a stray glyph + a stale item texture.
    msg.AutoFormat();
}
#endif
void BuildQuarterHeartMessage(CustomMessage& msg) {
    msg = { "You found a %yQuarter Heart Container%w!&You gained quarter of a heart.",
            "Du erhältst einen %yViertelherz-Behälter%w!&Du gewinnst ein Viertel eines Herzens.",
            "Vous trouvez un %yContenant de Quart de Cœur%w!&Vous gagnez un quart de cœur." };
    msg.AutoFormat();
}

void BuildDefenseUpgradeMessage(CustomMessage& msg) {
    uint8_t level = gSaveContext.ship.quest.data.randomizer.defenseUpgrades + 1;
    uint8_t required =
        StatUpgradeRequired(5, RSK_DEFENSE_UPGRADE_ADJUSTABLE, RSK_DEFENSE_UPGRADE_TOTAL, RSK_DEFENSE_UPGRADE_REQUIRED);
    if (level < required) {
        uint8_t remaining = required - level;
        msg = { "You found a %yDefense Upgrade%w!&%c[[remaining]]%w more to reach max stat.",
                "Du erhältst ein %yVerteidigungs-Upgrade%w!&Noch %c[[remaining]]%w bis zum Maximum.",
                "Vous trouvez une %yAmélioration de Défense%w!&Encore %c[[remaining]]%w pour atteindre le maximum." };
        msg.Replace("[[remaining]]", std::to_string(remaining));
    } else if (level == required) {
        msg = { "You found a %yDefense Upgrade%w!&%gMax defense reached!%w",
                "Du erhältst ein %yVerteidigungs-Upgrade%w!&%gMaximale Verteidigung erreicht!%w",
                "Vous trouvez une %yAmélioration de Défense%w!&%gDéfense maximale atteinte!%w" };
    } else {
        msg = { "You found a %yDefense Upgrade%w!&%rAlready at max defense!%w",
                "Du erhältst ein %yVerteidigungs-Upgrade%w!&%rBereits bei maximaler Verteidigung!%w",
                "Vous trouvez une %yAmélioration de Défense%w!&%rDéfense déjà au maximum!%w" };
    }
    msg.AutoFormat(ITEM_CUSTOM);
}

void BuildSpeedUpgradeMessage(CustomMessage& msg) {
    uint8_t level = gSaveContext.ship.quest.data.randomizer.speedUpgrades + 1;
    uint8_t required =
        StatUpgradeRequired(5, RSK_SPEED_UPGRADE_ADJUSTABLE, RSK_SPEED_UPGRADE_TOTAL, RSK_SPEED_UPGRADE_REQUIRED);
    if (level < required) {
        uint8_t remaining = required - level;
        msg = { "You found a %ySpeed Upgrade%w!&%c[[remaining]]%w more to reach max stat.",
                "Du erhältst ein %yGeschwindigkeits-Upgrade%w!&Noch %c[[remaining]]%w bis zum Maximum.",
                "Vous trouvez une %yAmélioration de Vitesse%w!&Encore %c[[remaining]]%w pour atteindre le maximum." };
        msg.Replace("[[remaining]]", std::to_string(remaining));
    } else if (level == required) {
        msg = { "You found a %ySpeed Upgrade%w!&%gMax speed reached!%w",
                "Du erhältst ein %yGeschwindigkeits-Upgrade%w!&%gMaximale Geschwindigkeit erreicht!%w",
                "Vous trouvez une %yAmélioration de Vitesse%w!&%gVitesse maximale atteinte!%w" };
    } else {
        msg = { "You found a %ySpeed Upgrade%w!&%rAlready at max speed!%w",
                "Du erhältst ein %yGeschwindigkeits-Upgrade%w!&%rBereits bei maximaler Geschwindigkeit!%w",
                "Vous trouvez une %yAmélioration de Vitesse%w!&%rVitesse déjà au maximum!%w" };
    }
    msg.AutoFormat(ITEM_CUSTOM);
}

void BuildPowerUpgradeMessage(CustomMessage& msg) {
    uint8_t level = gSaveContext.ship.quest.data.randomizer.powerUpgrades + 1;
    uint8_t required =
        StatUpgradeRequired(5, RSK_POWER_UPGRADE_ADJUSTABLE, RSK_POWER_UPGRADE_TOTAL, RSK_POWER_UPGRADE_REQUIRED);
    if (level < required) {
        uint8_t remaining = required - level;
        msg = { "You found a %yPower Upgrade%w!&Raises your double damage chance!&%c[[remaining]]%w more to reach max "
                "stat.",
                "Du erhältst ein %yKraft-Upgrade%w!&Sammle mehr für höhere Chance auf doppelten Schaden!&Noch "
                "%c[[remaining]]%w.",
                "Vous trouvez une %yAmélioration de Force%w!&Collectez-en plus pour doubler vos dégâts plus "
                "souvent!&%c[[remaining]]%w restants." };
        msg.Replace("[[remaining]]", std::to_string(remaining));
    } else if (level == required) {
        msg = { "You found a %yPower Upgrade%w!&%gEvery hit now deals double damage!%w",
                "Du erhältst ein %yKraft-Upgrade%w!&%gJeder Treffer macht jetzt doppelten Schaden!%w",
                "Vous trouvez une %yAmélioration de Force%w!&%gChaque coup inflige maintenant le double de dégâts!%w" };
    } else {
        msg = { "You found a %yPower Upgrade%w!&%rAlready at max power!%w",
                "Du erhältst ein %yKraft-Upgrade%w!&%rBereits bei maximaler Kraft!%w",
                "Vous trouvez une %yAmélioration de Force%w!&%rForce déjà au maximum!%w" };
    }
    msg.AutoFormat(ITEM_CUSTOM);
}

void BuildMagicStatUpgradeMessage(CustomMessage& msg) {
    const unsigned level = static_cast<unsigned>(gSaveContext.ship.quest.data.randomizer.magicStatUpgrades) + 1;
    uint8_t required = StatUpgradeRequired(8, RSK_MAGIC_STAT_UPGRADE_ADJUSTABLE, RSK_MAGIC_STAT_UPGRADE_TOTAL,
                                           RSK_MAGIC_STAT_UPGRADE_REQUIRED);
    if (level < required) {
        uint8_t remaining = required - level;
        msg = { "You found a %yMagic Meter%w!&%c[[remaining]]%w more to reach max stat.",
                "Du erhältst ein %yMagisches Maß%w!&Noch %c[[remaining]]%w bis zum Maximum.",
                "Vous trouvez une %yJauge de Magie%w!&Encore %c[[remaining]]%w pour atteindre le maximum." };
        msg.Replace("[[remaining]]", std::to_string(remaining));
    } else if (level == required) {
        msg = { "You found a %yMagic Meter%w!&%gMax magic reached!%w",
                "Du erhältst ein %yMagisches Maß%w!&%gMaximale Magie erreicht!%w",
                "Vous trouvez une %yJauge de Magie%w!&%gMagie maximale atteinte!%w" };
    } else {
        msg = { "You found a %yMagic Meter%w!&%rAlready at max magic!%w",
                "Du erhältst ein %yMagisches Maß%w!&%rBereits bei maximaler Magie!%w",
                "Vous trouvez une %yJauge de Magie%w!&%rMagie déjà au maximum!%w" };
    }
    msg.AutoFormat(ITEM_CUSTOM);
}

void BuildCrawlSpeedUpgradeMessage(CustomMessage& msg) {
    uint8_t level = gSaveContext.ship.quest.data.randomizer.crawlSpeedUpgrades + 1;
    uint8_t required = StatUpgradeRequired(5, RSK_CRAWL_SPEED_UPGRADE_ADJUSTABLE, RSK_CRAWL_SPEED_UPGRADE_TOTAL,
                                           RSK_CRAWL_SPEED_UPGRADE_REQUIRED);
    if (level < required) {
        uint8_t remaining = required - level;
        msg = { "You found a %yCrawl Speed Upgrade%w!&%c[[remaining]]%w more to reach max stat.",
                "Du erhältst ein %yKriech-Upgrade%w!&Noch %c[[remaining]]%w bis zum Maximum.",
                "Vous trouvez une %yAmélioration de Vitesse de Reptation%w!&Encore %c[[remaining]]%w pour atteindre le "
                "maximum." };
        msg.Replace("[[remaining]]", std::to_string(remaining));
    } else if (level == required) {
        msg = {
            "You found a %yCrawl Speed Upgrade%w!&%gMax crawl speed reached!%w",
            "Du erhältst ein %yKriech-Upgrade%w!&%gMaximale Kriechgeschwindigkeit erreicht!%w",
            "Vous trouvez une %yAmélioration de Vitesse de Reptation%w!&%gVitesse de reptation maximale atteinte!%w"
        };
    } else {
        msg = {
            "You found a %yCrawl Speed Upgrade%w!&%rAlready at max crawl speed!%w",
            "Du erhältst ein %yKriech-Upgrade%w!&%rBereits bei maximaler Kriechgeschwindigkeit!%w",
            "Vous trouvez une %yAmélioration de Vitesse de Reptation%w!&%rVitesse de reptation déjà au maximum!%w"
        };
    }
    msg.AutoFormat(ITEM_CUSTOM);
}

void BuildClimbSpeedUpgradeMessage(CustomMessage& msg) {
    uint8_t level = gSaveContext.ship.quest.data.randomizer.climbSpeedUpgrades + 1;
    uint8_t required = StatUpgradeRequired(5, RSK_CLIMB_SPEED_UPGRADE_ADJUSTABLE, RSK_CLIMB_SPEED_UPGRADE_TOTAL,
                                           RSK_CLIMB_SPEED_UPGRADE_REQUIRED);
    if (level < required) {
        uint8_t remaining = required - level;
        msg = { "You found a %yClimb Speed Upgrade%w!&%c[[remaining]]%w more to reach max stat.",
                "Du erhältst ein %yKletter-Upgrade%w!&Noch %c[[remaining]]%w bis zum Maximum.",
                "Vous trouvez une %yAmélioration de Vitesse d'Escalade%w!&Encore %c[[remaining]]%w pour atteindre le "
                "maximum." };
        msg.Replace("[[remaining]]", std::to_string(remaining));
    } else if (level == required) {
        msg = { "You found a %yClimb Speed Upgrade%w!&%gMax climb speed reached!%w",
                "Du erhältst ein %yKletter-Upgrade%w!&%gMaximale Klettergeschwindigkeit erreicht!%w",
                "Vous trouvez une %yAmélioration de Vitesse d'Escalade%w!&%gVitesse d'escalade maximale atteinte!%w" };
    } else {
        msg = { "You found a %yClimb Speed Upgrade%w!&%rAlready at max climb speed!%w",
                "Du erhältst ein %yKletter-Upgrade%w!&%rBereits bei maximaler Klettergeschwindigkeit!%w",
                "Vous trouvez une %yAmélioration de Vitesse d'Escalade%w!&%rVitesse d'escalade déjà au maximum!%w" };
    }
    msg.AutoFormat(ITEM_CUSTOM);
}

void BuildPushSpeedUpgradeMessage(CustomMessage& msg) {
    uint8_t level = gSaveContext.ship.quest.data.randomizer.pushSpeedUpgrades + 1;
    uint8_t required = StatUpgradeRequired(5, RSK_PUSH_SPEED_UPGRADE_ADJUSTABLE, RSK_PUSH_SPEED_UPGRADE_TOTAL,
                                           RSK_PUSH_SPEED_UPGRADE_REQUIRED);
    if (level < required) {
        uint8_t remaining = required - level;
        msg = { "You found a %yPush Speed Upgrade%w!&%c[[remaining]]%w more to reach max stat.",
                "Du erhältst ein %ySchub-Upgrade%w!&Noch %c[[remaining]]%w bis zum Maximum.",
                "Vous trouvez une %yAmélioration de Vitesse de Poussée%w!&Encore %c[[remaining]]%w pour atteindre le "
                "maximum." };
        msg.Replace("[[remaining]]", std::to_string(remaining));
    } else if (level == required) {
        msg = { "You found a %yPush Speed Upgrade%w!&%gMax push speed reached!%w",
                "Du erhältst ein %ySchub-Upgrade%w!&%gMaximale Schubgeschwindigkeit erreicht!%w",
                "Vous trouvez une %yAmélioration de Vitesse de Poussée%w!&%gVitesse de poussée maximale atteinte!%w" };
    } else {
        msg = { "You found a %yPush Speed Upgrade%w!&%rAlready at max push speed!%w",
                "Du erhältst ein %ySchub-Upgrade%w!&%rBereits bei maximaler Schubgeschwindigkeit!%w",
                "Vous trouvez une %yAmélioration de Vitesse de Poussée%w!&%rVitesse de poussée déjà au maximum!%w" };
    }
    msg.AutoFormat(ITEM_CUSTOM);
}

void BuildItemMessage(u16* textId, bool* loadFromMessageTable) {
    Player* player = GET_PLAYER(gPlayState);
    // Fixed Time Gate and house feather rewards use custom-item text in normal saves.
    // Other vanilla uses of 0xF8 must continue loading their ordinary message.
    if (*textId == TEXT_RANDOMIZER_CUSTOM_ITEM && !IS_RANDO &&
        (player->getItemEntry.objectId == OBJECT_INVALID || player->getItemEntry.modIndex != MOD_RANDOMIZER ||
         (player->getItemEntry.getItemId != RG_TIME_GATE && player->getItemEntry.getItemId != RG_PROGRESSIVE_ROCS))) {
        return;
    }
    CustomMessage msg;

    if (player->getItemEntry.getItemId == RG_ICE_TRAP) {
        Rando::Traps::BuildIceTrapMessage(msg, player->getItemEntry);
    } else if (player->getItemEntry.getItemId == RG_TRIFORCE_PIECE) {
        BuildTriforcePieceMessage(msg);
    } else if (player->getItemEntry.getItemId == RG_TRIFORCE) {
        BuildTriforceMessage(msg);
#ifdef COMBO_BUILD
    } else if (player->getItemEntry.getItemId == RG_COMBO_FOREIGN) {
        BuildComboForeignMessage(player, msg);
#endif
    } else if (player->getItemEntry.getItemId == RG_QUARTER_HEART) {
        BuildQuarterHeartMessage(msg);
    } else if (player->getItemEntry.getItemId == RG_DEFENSE_UPGRADE) {
        BuildDefenseUpgradeMessage(msg);
    } else if (player->getItemEntry.getItemId == RG_SPEED_UPGRADE) {
        BuildSpeedUpgradeMessage(msg);
    } else if (player->getItemEntry.getItemId == RG_POWER_UPGRADE) {
        BuildPowerUpgradeMessage(msg);
    } else if (player->getItemEntry.getItemId == RG_MAGIC_STAT_UPGRADE) {
        BuildMagicStatUpgradeMessage(msg);
    } else if (player->getItemEntry.getItemId == RG_CRAWL_SPEED_UPGRADE) {
        BuildCrawlSpeedUpgradeMessage(msg);
    } else if (player->getItemEntry.getItemId == RG_CLIMB_SPEED_UPGRADE) {
        BuildClimbSpeedUpgradeMessage(msg);
    } else if (player->getItemEntry.getItemId == RG_PUSH_SPEED_UPGRADE) {
        BuildPushSpeedUpgradeMessage(msg);
    } else {
        BuildCustomItemMessage(player, msg);
    }
    *loadFromMessageTable = false;
    msg.LoadIntoFont();
}

// The custom get-item textbox and MM donor export run before the grant, with
// an already selected identity. En_Si's post-grant native GS textbox does not
// call this builder; its existing native count message remains separate.
bool BuildTokenReceiptMessage(RandomizerGet rg, CustomMessage& msg) {
    unsigned count;
    CustomMessage place;
    switch (rg) {
        case RG_GOLD_SKULLTULA_TOKEN:
            count = gSaveContext.inventory.gsTokens + 1;
            place = CustomMessage(" in Hyrule", " in Hyrule", " en Hyrule");
            break;
        case RG_MM_GS_TOKEN_SWAMP:
            count = Nei_Save()->comboObtained[FC_MM_SKULLS_SWAMP] + 1;
            place = CustomMessage(" in the Swamp Spider House", " im Sumpf-Spinnenhaus",
                                  " dans la maison des araignées du marais");
            break;
        case RG_MM_GS_TOKEN_OCEAN:
            count = Nei_Save()->comboObtained[FC_MM_SKULLS_OCEAN] + 1;
            place = CustomMessage(" in the Ocean Spider House", " im Ozean-Spinnenhaus",
                                  " dans la maison des araignées de l'océan");
            break;
        default:
            return false;
    }
    msg = CustomMessage(
        "You found [[article]][[color]][[name]]%w!^You've collected [[color]][[count]]%w in total[[place]].",
        "Du hast ein [[color]][[name]]%w gefunden!^Insgesamt [[color]][[count]]%w gesammelt[[place]].",
        "Vous obtenez un [[color]][[name]]%w!^Vous en avez collecté [[color]][[count]]%w au total[[place]].",
        TEXTBOX_TYPE_BLUE);
    msg.Replace("[[color]]", rg == RG_GOLD_SKULLTULA_TOKEN ? "%r" : "%c");
    msg.Replace("[[article]]", rg == RG_MM_GS_TOKEN_OCEAN ? "an " : "a ");
    msg.Replace("[[name]]", CustomMessage(Rando::StaticData::RetrieveItem(rg).GetName()));
    msg.Replace("[[count]]", std::to_string(count));
    msg.Replace("[[place]]", place);
    msg.AutoFormat(rg == RG_GOLD_SKULLTULA_TOKEN ? ITEM_SKULL_TOKEN : ITEM_CUSTOM);
    return true;
}

static CustomMessage DungeonRewardName(RandomizerCheck check) {
    const auto* location = OTRGlobals::Instance->gRandoContext->GetItemLocation(check);
    const auto item = location->GetPlacedRandomizerGet();
#ifdef COMBO_BUILD
    if (item == RG_COMBO_FOREIGN) {
        const auto* foreign = OOT_LookupForeignByCheck(check);
        if (foreign && !foreign->itemName.empty())
            return CustomMessage(foreign->displayName.empty() ? foreign->itemName : foreign->displayName);
        return CustomMessage("an unknown reward", "eine unbekannte Belohnung", "une récompense inconnue");
    }
#endif
    if (item == RG_NONE || item == RG_HINT || item == RG_SOLD_OUT)
        return CustomMessage("an unknown reward", "eine unbekannte Belohnung", "une récompense inconnue");
    // Seed identity only: a preview must not resolve a progressive against the
    // current inventory or reveal a reward belonging to the dungeon's old boss.
    return CustomMessage(location->GetPlacedItemName());
}

static int16_t DungeonEntranceDestination(int16_t entrance) {
    auto* overrides = Randomizer_GetEntranceOverrides();
    for (size_t i = 0; i < ENTRANCE_OVERRIDES_MAX_COUNT; ++i) {
        if (Entrance_EntranceIsNull(&overrides[i]))
            break;
        if (overrides[i].index == entrance)
            return overrides[i].override;
    }
    return entrance;
}

static int DungeonBossDestination(int dungeon) {
    constexpr int entries[] = { ENTR_DEKU_TREE_ENTRANCE,     ENTR_DODONGOS_CAVERN_ENTRANCE, ENTR_JABU_JABU_ENTRANCE,
                                ENTR_FOREST_TEMPLE_ENTRANCE, ENTR_FIRE_TEMPLE_ENTRANCE,     ENTR_WATER_TEMPLE_ENTRANCE,
                                ENTR_SPIRIT_TEMPLE_ENTRANCE, ENTR_SHADOW_TEMPLE_ENTRANCE };
    constexpr int doors[] = { ENTR_DEKU_TREE_BOSS_ENTRANCE,     ENTR_DODONGOS_CAVERN_BOSS_ENTRANCE,
                              ENTR_JABU_JABU_BOSS_ENTRANCE,     ENTR_FOREST_TEMPLE_BOSS_ENTRANCE,
                              ENTR_FIRE_TEMPLE_BOSS_ENTRANCE,   ENTR_WATER_TEMPLE_BOSS_ENTRANCE,
                              ENTR_SPIRIT_TEMPLE_BOSS_ENTRANCE, ENTR_SHADOW_TEMPLE_BOSS_ENTRANCE };
    const auto ctx = OTRGlobals::Instance->gRandoContext;
    if (ctx->GetOption(RSK_SHUFFLE_BOSS_ENTRANCES).Is(RO_BOSS_ROOM_ENTRANCE_SHUFFLE_OFF))
        return dungeon;
    unsigned visited = 0;
    while (dungeon >= 0 && dungeon < 8 && !(visited & (1u << dungeon))) {
        visited |= 1u << dungeon;
        const int destination = DungeonEntranceDestination(doors[dungeon]);
        int nextDungeon = -1;
        for (int i = 0; i < 8; ++i) {
            if (destination == doors[i])
                return i;
            if (destination == entries[i])
                nextDungeon = i;
        }
        // Mixed pools can put another dungeon behind the boss door. Follow
        // that dungeon's saved boss route rather than discarding the hint.
        // An exterior/interior, boss-less dungeon or loop supplies no boss.
        dungeon = nextDungeon;
    }
    return -1;
}

static std::string DungeonPhysicalEntranceName(int16_t entrance) {
    constexpr int entrances[] = { ENTR_DEKU_TREE_ENTRANCE,          ENTR_DODONGOS_CAVERN_ENTRANCE,
                                  ENTR_JABU_JABU_ENTRANCE,          ENTR_FOREST_TEMPLE_ENTRANCE,
                                  ENTR_FIRE_TEMPLE_ENTRANCE,        ENTR_WATER_TEMPLE_ENTRANCE,
                                  ENTR_SPIRIT_TEMPLE_ENTRANCE,      ENTR_SHADOW_TEMPLE_ENTRANCE,
                                  ENTR_BOTTOM_OF_THE_WELL_ENTRANCE, ENTR_ICE_CAVERN_ENTRANCE };
    constexpr const char* names[] = { "Deku Tree",          "Dodongo's Cavern", "Jabu-Jabu's Belly", "Forest Temple",
                                      "Fire Temple",        "Water Temple",     "Spirit Temple",     "Shadow Temple",
                                      "Bottom of the Well", "Ice Cavern" };
    for (size_t i = 0; i < 10; ++i)
        if (entrance == entrances[i])
            return names[i];
    const auto* data = EntranceTracker::GetEntranceData(entrance);
    return data ? data->source : std::string{};
}

static std::string DungeonEntranceSource(int16_t dungeonEntrance) {
    auto* overrides = Randomizer_GetEntranceOverrides();
    std::vector<std::string> sources;
    for (size_t i = 0; i < ENTRANCE_OVERRIDES_MAX_COUNT; ++i) {
        if (Entrance_EntranceIsNull(&overrides[i]))
            break;
        if (overrides[i].override != dungeonEntrance)
            continue;
        const auto source = DungeonPhysicalEntranceName(overrides[i].index);
        if (!source.empty() && std::find(sources.begin(), sources.end(), source) == sources.end())
            sources.push_back(source);
    }
    if (sources.empty() && DungeonEntranceDestination(dungeonEntrance) == dungeonEntrance) {
        // An excluded/unshuffled entrance still has its physical vanilla source.
        const auto source = DungeonPhysicalEntranceName(dungeonEntrance);
        if (!source.empty())
            sources.push_back(source);
    }
    std::string source;
    for (const auto& name : sources) {
        if (!source.empty())
            source += ", ";
        source += name;
    }
    return source;
}

static void AddDungeonRewardIcon(CustomMessage& msg, RandomizerCheck check) {
#ifdef COMBO_BUILD
    CwItemIconInfo icon{};
    const auto* location = OTRGlobals::Instance->gRandoContext->GetItemLocation(check);
    const auto rg = location->GetPlacedRandomizerGet();
    const char* owner = "oot";
    int32_t available = 0;
    if (rg == RG_COMBO_FOREIGN) {
        const auto* foreign = OOT_LookupForeignByCheck(check);
        if (foreign && !foreign->itemName.empty()) {
            owner = foreign->itemGame == ComboRando::GAME_MM ? "mm" : "oot";
            auto getIcon = reinterpret_cast<Fn_GetItemIconInfo>(Combo_ResolveSym(
                foreign->itemGame == ComboRando::GAME_MM ? "2ship" : "soh",
                foreign->itemGame == ComboRando::GAME_MM ? "MM_GetSeedItemIconInfo" : "OOT_GetSeedItemIconInfo"));
            if (getIcon)
                available = getIcon(ComboRando::StripGameSuffix(foreign->itemName).c_str(), &icon);
        }
    } else if (rg != RG_NONE && rg != RG_HINT && rg != RG_SOLD_OUT) {
        available = OOT_GetSeedItemIconInfo(location->GetPlacedItemName().GetEnglish().c_str(), &icon);
    }
    if (available == 1)
        ComboReceipt_CopyIcon(&msg.receiptPresentation, &icon, owner);
#endif
}

bool BuildDungeonItemReceiptMessage(RandomizerGet rg, CustomMessage& msg, bool received) {
    const bool ootMap = rg >= RG_DEKU_TREE_MAP && rg <= RG_ICE_CAVERN_MAP;
    const bool ootCompass = rg >= RG_DEKU_TREE_COMPASS && rg <= RG_ICE_CAVERN_COMPASS;
    const bool mmMap = rg >= RG_MM_MAP_WOODFALL && rg <= RG_MM_MAP_STONE_TOWER;
    const bool mmCompass = rg >= RG_MM_COMPASS_WOODFALL && rg <= RG_MM_COMPASS_STONE_TOWER;
    if (!ootMap && !ootCompass && !mmMap && !mmCompass)
        return false;

    auto ctx = OTRGlobals::Instance->gRandoContext;
    const auto item = Rando::StaticData::RetrieveItem(rg);
    msg = received ? CustomMessage(
                         "You found the [[color]][[name]]%w![[typeHint]][[bossHint]][[rewardHint]][[entranceHint]]",
                         "Du erhältst das [[color]][[name]]%w![[typeHint]][[bossHint]][[rewardHint]][[entranceHint]]",
                         "Vous obtenez [[color]][[name]]%w![[typeHint]][[bossHint]][[rewardHint]][[entranceHint]]",
                         TEXTBOX_TYPE_BLUE)
                   : CustomMessage("[[color]][[name]]%w[[typeHint]][[bossHint]][[rewardHint]][[entranceHint]]",
                                   TEXTBOX_TYPE_BLUE);
    msg.Replace("[[name]]", CustomMessage(item.GetName()));
    msg.Replace("[[color]]", item.GetColor());

    CustomMessage typeHint;
    CustomMessage bossHint;
    CustomMessage rewardHint;
    CustomMessage entranceHint;
    CustomMessage rewardName;
    bool hasReward = false;
    const bool information = DungeonInformationEnabled();
    msg.receiptPresentation.singleBox = information;
    msg.receiptPresentation.rewardLine = 2;
    const char* boss = nullptr;
    if (ootMap || ootCompass) {
        const int dungeon = rg - (ootMap ? RG_DEKU_TREE_MAP : RG_DEKU_TREE_COMPASS);
        constexpr int scenes[] = { SCENE_DEKU_TREE,     SCENE_DODONGOS_CAVERN, SCENE_JABU_JABU,
                                   SCENE_FOREST_TEMPLE, SCENE_FIRE_TEMPLE,     SCENE_WATER_TEMPLE,
                                   SCENE_SPIRIT_TEMPLE, SCENE_SHADOW_TEMPLE,   SCENE_BOTTOM_OF_THE_WELL,
                                   SCENE_ICE_CAVERN };
        const auto* seedDungeon = ctx->GetDungeons()->GetDungeonFromScene(scenes[dungeon]);
        const bool masterQuest =
            IS_RANDO ? ResourceMgr_IsSceneMasterQuest(scenes[dungeon]) : seedDungeon && seedDungeon->IsMQ();
        if (information && ootMap) {
            typeHint = Rando::StaticData::hintTextTable[masterQuest ? RHT_DUNGEON_MASTERFUL : RHT_DUNGEON_ORDINARY]
                           .GetHintMessage();
        } else if (!information && !ctx->GetOption(RSK_MQ_DUNGEON_RANDOM).Is(RO_MQ_DUNGEONS_NONE) &&
                   !(ctx->GetOption(RSK_MQ_DUNGEON_RANDOM).Is(RO_MQ_DUNGEONS_SET_NUMBER) &&
                     ctx->GetOption(RSK_MQ_DUNGEON_COUNT).Is(MAX_MQ_DUNGEON_COUNT))) {
            // The resource helper follows the active OoT quest. In an MM-first
            // seed the donor is dormant, so read its loaded seed dungeon mode.
            typeHint = Rando::StaticData::hintTextTable[masterQuest ? RHT_DUNGEON_MASTERFUL : RHT_DUNGEON_ORDINARY]
                           .GetHintMessage();
        }
        if (information && ootMap && !ctx->GetOption(RSK_SHUFFLE_DUNGEON_ENTRANCES).Is(0)) {
            constexpr int entrances[] = { ENTR_DEKU_TREE_ENTRANCE,          ENTR_DODONGOS_CAVERN_ENTRANCE,
                                          ENTR_JABU_JABU_ENTRANCE,          ENTR_FOREST_TEMPLE_ENTRANCE,
                                          ENTR_FIRE_TEMPLE_ENTRANCE,        ENTR_WATER_TEMPLE_ENTRANCE,
                                          ENTR_SPIRIT_TEMPLE_ENTRANCE,      ENTR_SHADOW_TEMPLE_ENTRANCE,
                                          ENTR_BOTTOM_OF_THE_WELL_ENTRANCE, ENTR_ICE_CAVERN_ENTRANCE };
            const auto source = DungeonEntranceSource(entrances[dungeon]);
            if (!source.empty()) {
                entranceHint = CustomMessage("&It seems the entrance is at %c[[source]]%w.",
                                             "&Der Eingang scheint bei %c[[source]]%w zu liegen.",
                                             "&L'entrée semble se trouver à %c[[source]]%w.");
                entranceHint.Replace("[[source]]", source);
            }
        }
        if (information && ootCompass && dungeon < 8) {
            constexpr const char* bosses[] = { "Queen Gohma", "King Dodongo", "Barinade", "Phantom Ganon",
                                               "Volvagia",    "Morpha",       "Twinrova", "Bongo Bongo" };
            constexpr RandomizerCheck rewards[] = { RC_QUEEN_GOHMA, RC_KING_DODONGO, RC_BARINADE, RC_PHANTOM_GANON,
                                                    RC_VOLVAGIA,    RC_MORPHA,       RC_TWINROVA, RC_BONGO_BONGO };
            const int assignedBoss = DungeonBossDestination(dungeon);
            if (assignedBoss >= 0) {
                boss = bosses[assignedBoss];
                rewardName = DungeonRewardName(rewards[assignedBoss]);
                hasReward = true;
                AddDungeonRewardIcon(msg, rewards[assignedBoss]);
            } else {
                bossHint = CustomMessage("&Its boss door does not lead to a known boss room.",
                                         "&Die Bosstür führt zu keinem bekannten Bossraum.",
                                         "&Sa porte ne mène pas à une salle de boss connue.");
            }
        }
    } else if (information && mmMap) {
        // Match MM's known native entrances; this port has no MM entrance graph.
        constexpr const char* entrances[] = { "Woodfall", "Snowhead", "Zora Cape's turtle", "Stone Tower" };
        entranceHint = CustomMessage("&It seems the entrance is at %c[[source]]%w.",
                                     "&Der Eingang scheint bei %c[[source]]%w zu liegen.",
                                     "&L'entrée semble se trouver à %c[[source]]%w.");
        entranceHint.Replace("[[source]]", entrances[rg - RG_MM_MAP_WOODFALL]);
    } else if (information && mmCompass) {
        // This port has no MM boss-entrance shuffle; Combo seeds currently
        // serialize only the OoT entrance graph.
        constexpr const char* bosses[] = { "Odolwa", "Goht", "Gyorg", "Twinmold" };
        boss = bosses[rg - RG_MM_COMPASS_WOODFALL];
#ifdef COMBO_BUILD
        using RewardNameFn = int32_t (*)(int32_t, char*, uint32_t);
        auto getRewardName = reinterpret_cast<RewardNameFn>(Combo_ResolveSym("2ship", "MM_GetDungeonRewardName"));
        char name[512];
        if (getRewardName) {
            const int32_t size = getRewardName(rg - RG_MM_COMPASS_WOODFALL, name, sizeof(name));
            if (size > 0 && static_cast<size_t>(size) < sizeof(name)) {
                rewardName = CustomMessage(std::string(name, size));
                hasReward = true;
                using RewardIconFn = int32_t (*)(int32_t, CwItemIconInfo*);
                auto getIcon = reinterpret_cast<RewardIconFn>(Combo_ResolveSym("2ship", "MM_GetDungeonRewardIconInfo"));
                CwItemIconInfo icon{};
                if (getIcon && getIcon(rg - RG_MM_COMPASS_WOODFALL, &icon) == 1)
                    ComboReceipt_CopyIcon(&msg.receiptPresentation, &icon, "mm");
            }
        }
#endif
    }
    if (hasReward) {
        rewardHint = CustomMessage("&Defeating the boss grants the %g[[reward]]%w!",
                                   "&Der Boss hinterlässt %g[[reward]]%w!", "&Vaincre le boss donne %g[[reward]]%w!");
        rewardHint.Replace("[[reward]]", rewardName);
    }
    if (boss) {
        bossHint = CustomMessage("&It points to %r[[boss]]%w!", "&Er zeigt zu %r[[boss]]%w!",
                                 "&Elle pointe vers %r[[boss]]%w!");
        bossHint.Replace("[[boss]]", boss);
    }
    msg.Replace("[[typeHint]]", typeHint);
    msg.Replace("[[bossHint]]", bossHint);
    msg.Replace("[[rewardHint]]", rewardHint);
    msg.Replace("[[entranceHint]]", entranceHint);
    if (information) {
        msg.Replace("Great Deku Tree", "Deku Tree");
        if (ComboReceipt_HasIcon(&msg.receiptPresentation))
            msg += CustomMessage::ITEM_OBTAINED(ITEM_CUSTOM);
        msg.Format(); // the renderer wraps authored lines at the native font size
    } else {
        msg.AutoFormat(ootCompass || mmCompass ? ITEM_COMPASS : ITEM_DUNGEON_MAP);
    }
    return true;
}

#ifdef COMBO_BUILD
extern "C" COMBO_EXPORT int32_t OOT_GetDungeonItemReceiptPresentation(const char* itemName,
                                                                      CwItemReceiptPresentation* out) {
    try {
        if (!itemName || !out || !DungeonInformationEnabled())
            return 0;
        *out = CwItemReceiptPresentation{};
        const auto found = Rando::StaticData::itemNameToEnum.find(itemName);
        CustomMessage message;
        if (found == Rando::StaticData::itemNameToEnum.end() || !BuildDungeonItemReceiptMessage(found->second, message))
            return 0;
        *out = message.receiptPresentation;
        return out->singleBox == 1;
    } catch (...) { return 0; }
}
#endif

extern "C" uint16_t Randomizer_GetDungeonItemInfoTextId(uint16_t cursorItem) {
    if (!DungeonInformationEnabled() || !gPlayState || gSaveContext.mapIndex < 0 || gSaveContext.mapIndex >= 10 ||
        !((gPlayState->sceneNum >= SCENE_DEKU_TREE && gPlayState->sceneNum <= SCENE_ICE_CAVERN) ||
          (gPlayState->sceneNum >= SCENE_DEKU_TREE_BOSS && gPlayState->sceneNum <= SCENE_SHADOW_TEMPLE_BOSS)))
        return 0;
    if (cursorItem == ITEM_COMPASS && CHECK_DUNGEON_ITEM(DUNGEON_COMPASS, gSaveContext.mapIndex))
        return TEXT_DESC_DUNGEON_COMPASS_INFO;
    if (cursorItem == ITEM_DUNGEON_MAP && CHECK_DUNGEON_ITEM(DUNGEON_MAP, gSaveContext.mapIndex))
        return TEXT_DESC_DUNGEON_MAP_INFO;
    return 0;
}

void BuildDungeonPauseInfoMessage(uint16_t* textId, bool* loadFromMessageTable) {
    const bool compass = *textId == TEXT_DESC_DUNGEON_COMPASS_INFO;
    if (Randomizer_GetDungeonItemInfoTextId(compass ? ITEM_COMPASS : ITEM_DUNGEON_MAP) != *textId)
        return;
    CustomMessage msg;
    const auto rg =
        static_cast<RandomizerGet>((compass ? RG_DEKU_TREE_COMPASS : RG_DEKU_TREE_MAP) + gSaveContext.mapIndex);
    if (BuildDungeonItemReceiptMessage(rg, msg, false)) {
        msg.LoadIntoFont();
        *loadFromMessageTable = false;
    }
}

void BuildMapMessage(uint16_t* textId, bool* loadFromMessageTable) {
    if (!DungeonInformationEnabled())
        return; // Off retains the complete native map/compass tutorial.
    CustomMessage msg;
    const auto entry = GET_PLAYER(gPlayState)->getItemEntry;
    auto rg = static_cast<RandomizerGet>(entry.getItemId);
    // Native entries have no dungeon-specific RG. The pickup scene identifies
    // their dungeon even when its entrances alone are shuffled.
    if (entry.modIndex != MOD_RANDOMIZER && (entry.itemId == ITEM_COMPASS || entry.itemId == ITEM_DUNGEON_MAP) &&
        gPlayState->sceneNum >= SCENE_DEKU_TREE && gPlayState->sceneNum <= SCENE_ICE_CAVERN) {
        rg = static_cast<RandomizerGet>((entry.itemId == ITEM_COMPASS ? RG_DEKU_TREE_COMPASS : RG_DEKU_TREE_MAP) +
                                        gPlayState->sceneNum - SCENE_DEKU_TREE);
    }
    if (!BuildDungeonItemReceiptMessage(rg, msg))
        return;
    *loadFromMessageTable = false;
    msg.LoadIntoFont();
}

void BuildBossKeyMessage(uint16_t* textId, bool* loadFromMessageTable) {
    Player* player = GET_PLAYER(gPlayState);
    if (player->getItemEntry.getItemId == RG_GANONS_CASTLE_BOSS_KEY &&
        !DUNGEON_ITEMS_CAN_BE_OUTSIDE_DUNGEON(RSK_GANONS_BOSS_KEY)) {
        return;
    }
    if (player->getItemEntry.getItemId != RG_GANONS_CASTLE_BOSS_KEY &&
        !DUNGEON_ITEMS_CAN_BE_OUTSIDE_DUNGEON(RSK_BOSS_KEYSANITY)) {
        return;
    }
    CustomMessage msg;
    BuildCustomItemMessage(player, msg);
    *loadFromMessageTable = false;
    msg.LoadIntoFont();
}

void BuildSmallKeyMessage(uint16_t* textId, bool* loadFromMessageTable) {
    Player* player = GET_PLAYER(gPlayState);
    if (player->getItemEntry.getItemId == RG_GERUDO_FORTRESS_SMALL_KEY &&
        OTRGlobals::Instance->gRandoContext->GetOption(RSK_GERUDO_KEYS).Is(RO_GERUDO_KEYS_VANILLA)) {
        return;
    }
    if (player->getItemEntry.getItemId != RG_GERUDO_FORTRESS_SMALL_KEY &&
        DUNGEON_ITEMS_CAN_BE_OUTSIDE_DUNGEON(RSK_KEYSANITY)) {
        return;
    }
    CustomMessage msg;
    BuildCustomItemMessage(player, msg);
    *loadFromMessageTable = false;
    msg.LoadIntoFont();
}

// Time Gate custom item - "Travel through time?" Yes/No prompt
void BuildTimeGateMessage(uint16_t* textId, bool* loadFromMessageTable) {
    CustomMessage msg = CustomMessage("Travel through time?\x1B%g&&Yes&No%w", "Durch die Zeit reisen?\x1B%g&&Ja&Nein%w",
                                      "Voyager dans le temps?\x1B%g&&Oui&Non%w");
    msg.Format();
    msg.LoadIntoFont();
    *loadFromMessageTable = false;
}

void RegisterItemMessages() {
    COND_ID_HOOK(OnOpenText, TEXT_RANDOMIZER_CUSTOM_ITEM, true, BuildItemMessage);
    COND_ID_HOOK(OnOpenText, TEXT_DESC_DUNGEON_MAP_INFO, true, BuildDungeonPauseInfoMessage);
    COND_ID_HOOK(OnOpenText, TEXT_DESC_DUNGEON_COMPASS_INFO, true, BuildDungeonPauseInfoMessage);
    COND_ID_HOOK(OnOpenText, TEXT_ITEM_DUNGEON_MAP,
                 DUNGEON_ITEMS_CAN_BE_OUTSIDE_DUNGEON(RSK_SHUFFLE_MAPANDCOMPASS) || DungeonInformationEnabled(),
                 BuildMapMessage);
    COND_ID_HOOK(OnOpenText, TEXT_ITEM_COMPASS,
                 DUNGEON_ITEMS_CAN_BE_OUTSIDE_DUNGEON(RSK_SHUFFLE_MAPANDCOMPASS) || DungeonInformationEnabled(),
                 BuildMapMessage);
    COND_ID_HOOK(OnOpenText, TEXT_ITEM_KEY_BOSS,
                 (DUNGEON_ITEMS_CAN_BE_OUTSIDE_DUNGEON(RSK_BOSS_KEYSANITY) ||
                  DUNGEON_ITEMS_CAN_BE_OUTSIDE_DUNGEON(RSK_GANONS_BOSS_KEY)),
                 BuildBossKeyMessage);
    COND_ID_HOOK(OnOpenText, TEXT_ITEM_KEY_SMALL,
                 (OTRGlobals::Instance->gRandoContext->GetOption(RSK_GERUDO_KEYS).IsNot(RO_GERUDO_KEYS_VANILLA) ||
                  DUNGEON_ITEMS_CAN_BE_OUTSIDE_DUNGEON(RSK_KEYSANITY)),
                 BuildSmallKeyMessage);
}

// ── Lantern fire catch messages (always available) ──────────────────────────

#define TEXT_LANTERN_CATCH 0x00F9

void BuildLanternCatchMessage(uint16_t* textId, bool* loadFromMessageTable) {
    u8 fireType = gLanternCatchPending;
    CustomMessage msg;

    // \x13\xB4 = item icon for ITEM_LANTERN (0xB4)
    // All fire types: swing lights torches, burns grass (updraft + spread)
    switch (fireType) {
        case 1: // REGULAR (orange)
            msg = CustomMessage(
                "\x13\xB4"
                "You caught %rRegular Fire%w!&Swing to %rlight torches%w,&%rburn grass%w and spawn flames.",
                "\x13\xB4"
                "Du hast %rnormales Feuer%w!&Schwinge um %rFackeln%w und&%rGras zu verbrennen%w.",
                "\x13\xB4"
                "Vous avez le %rFeu Normal%w!&Agitez pour %rallumer%w et&%rbruler l'herbe%w.",
                TEXTBOX_TYPE_BLUE);
            break;
        case 2: // BLUE
            msg = CustomMessage("\x13\xB4"
                                "You caught %bBlue Fire%w!&Swing to release %bblue fire%w&that %cmelts red ice%w.",
                                "\x13\xB4"
                                "Du hast %bblaues Feuer%w!&Schwinge um %crotes Eis%w&%bzu schmelzen%w.",
                                "\x13\xB4"
                                "Vous avez le %bFeu Bleu%w!&Agitez pour %cfondre la&glace rouge%w.",
                                TEXTBOX_TYPE_BLUE);
            break;
        case 3: // POE (purple)
            msg =
                CustomMessage("\x13\xB4"
                              "You caught %pPoe Fire%w!&%pReveals the invisible%w and&%pdispels illusions%w. No magic.",
                              "\x13\xB4"
                              "Du hast %pIrrlichterfeuer%w!&%pEnthullt Unsichtbares%w und&%plost Illusionen auf%w.",
                              "\x13\xB4"
                              "Vous avez le %pFeu Spectral%w!&%pRevele l'invisible%w et&%pdissipe les illusions%w.",
                              TEXTBOX_TYPE_BLUE);
            break;
        case 4: // GREEN
            msg = CustomMessage("\x13\xB4"
                                "You caught %gGreen Fire%w!&Slowly %gregenerates health%w&while it stays lit.",
                                "\x13\xB4"
                                "Du hast %ggruenes Feuer%w!&%gRegeneriert langsam Leben%w,&solange es brennt.",
                                "\x13\xB4"
                                "Vous avez le %gFeu Vert%w!&%gRegenere lentement la vie%w&tant qu'il brule.",
                                TEXTBOX_TYPE_BLUE);
            break;
        default:
            msg = CustomMessage("\x13\xB4"
                                "The lantern is empty.",
                                "\x13\xB4"
                                "Die Laterne ist leer.",
                                "\x13\xB4"
                                "La lanterne est vide.",
                                TEXTBOX_TYPE_BLUE);
            break;
    }

    msg.AutoFormat();
    msg.LoadIntoFont();
    *loadFromMessageTable = false;
}

void RegisterLanternCatchMessage() {
    // Always available — not randomizer-dependent
    static HOOK_ID hookId = 0;
    GameInteractor::Instance->UnregisterGameHookForID<GameInteractor::OnOpenText>(hookId);
    hookId = GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(TEXT_LANTERN_CATCH,
                                                                                         BuildLanternCatchMessage);
}

// Time Gate message registration (always available, not rando-dependent)
void RegisterTimeGateMessage() {
    COND_ID_HOOK(OnOpenText, TEXT_TIME_GATE_PROMPT, true, BuildTimeGateMessage);
}

// Chateau Romani get-item message (always available, not rando-dependent)
void BuildChateauRomaniMessage(uint16_t* textId, bool* loadFromMessageTable) {
    CustomMessage msg = CustomMessage("You got %r\x08"
                                      "Chateau Romani%w!\x04"
                                      "Your magic power won't run out!%w",
                                      "Du hast %r\x08"
                                      "Chateau Romani%w erhalten!\x04"
                                      "Deine Magie wird nicht leer!%w",
                                      "Vous obtenez le %r\x08"
                                      "Chateau Romani%w!\x04"
                                      "Votre magie ne s'\xE9puisera pas!%w");
    msg.Format();
    msg.LoadIntoFont();
    *loadFromMessageTable = false;
}

void RegisterChateauRomaniMessage() {
    COND_ID_HOOK(OnOpenText, 0x9214, true, BuildChateauRomaniMessage);
}

// (Fleet Ship Combo: the old Happy Mask Shop "Travel to Termina?" prompt (0x9215) was removed —
// the blue warp is now a Door_Ana hole; falling in IS the confirmation.)

static RegisterShipInitFunc initFunc(RegisterItemMessages, { "IS_RANDO" });
static RegisterShipInitFunc initTimeGate(RegisterTimeGateMessage);
static RegisterShipInitFunc initChateau(RegisterChateauRomaniMessage);
static RegisterShipInitFunc initLanternCatch(RegisterLanternCatchMessage);

void RegisterCustomIconHooks() {
    // The original hook only fires when *should == false, but nothing in the call path
    // ever sets it to false for custom items — so the custom icon loaders never run and
    // vanilla tries to load Message_LoadItemIcon(ITEM_CUSTOM=0x9C) which is not a valid
    // OBJECT_GI_*. Detect custom-icon items via the player's getItemEntry, suppress
    // vanilla, and call our loader/drawer.
    // getItemId only holds a RandomizerGet when the entry IS a randomizer entry: the Item ctor puts
    // the RG there for MOD_RANDOMIZER rows and the vanilla GI id there for MOD_NONE ones. Casting a
    // GI id to RandomizerGet indexes a completely unrelated row, and if THAT row has a custom icon
    // the hook hijacks the textbox — which is why Iron Boots (GI 0x2E) showed Deku Nuts
    // (RG #0x2E = RG_PROGRESSIVE_NUT_UPGRADE) and Hover Boots (GI 0x2F) showed Deku Sticks. Gate on
    // modIndex so vanilla items keep their own icon token. Skijer's NEI
    COND_VB_SHOULD(VB_LOAD_ITEM_ICON, IS_RANDO, {
        Player* player = GET_PLAYER(gPlayState);
        if (player->getItemEntry.objectId != OBJECT_INVALID && player->getItemEntry.modIndex == MOD_RANDOMIZER) {
            RandomizerGet rgid = static_cast<RandomizerGet>(player->getItemEntry.getItemId);
            if (Rando::StaticData::RetrieveItem(rgid).HasCustomIcon()) {
                *should = false;
                LoadCustomItemIcon(static_cast<bool>(va_arg(args, int)));
                return;
            }
        }
        if (*should == false) {
            LoadCustomItemIcon(static_cast<bool>(va_arg(args, int)));
        }
    });
    COND_VB_SHOULD(VB_DRAW_ITEM_ICON, IS_RANDO, {
        Player* player = GET_PLAYER(gPlayState);
        if (player->getItemEntry.objectId != OBJECT_INVALID && player->getItemEntry.modIndex == MOD_RANDOMIZER) {
            RandomizerGet rgid = static_cast<RandomizerGet>(player->getItemEntry.getItemId);
            if (Rando::StaticData::RetrieveItem(rgid).HasCustomIcon()) {
                *should = false;
                DrawCustomItemIcon(va_arg(args, Gfx**));
                return;
            }
        }
        if (*should == false) {
            DrawCustomItemIcon(va_arg(args, Gfx**));
        }
    });
}

static RegisterShipInitFunc customIconInitFunc(RegisterCustomIconHooks, { "IS_RANDO" });
