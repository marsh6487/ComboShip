#include "Rando/Logic/SeedGeneration.h"
#include "Rando/VanillaCompatibility.h"
#include "Rando/Rando.h"
#include "Rando/Logic/Logic.h"
#include "2s2h/ShipUtils.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
extern "C" {
#include "functions.h"
#include "variables.h"
#include "overlays/actors/ovl_En_Sth/z_en_sth.h"
}
namespace Rando {
void PrepareNewRandomizedSave() {
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    // Zero out the rando struct
    memset(&gSaveContext.save.shipSaveInfo.rando, 0, sizeof(gSaveContext.save.shipSaveInfo.rando));
    // Copy whatever the current dungeon keys are, they're initialized as -1 in the save, not 0
    memcpy(&gSaveContext.save.shipSaveInfo.rando.foundDungeonKeys, &gSaveContext.save.saveInfo.inventory.dungeonKeys,
           sizeof(gSaveContext.save.saveInfo.inventory.dungeonKeys));

    // Skip the first cycle, in Rando we start as Human at south clock town.
    gSaveContext.save.entrance = ENTRANCE(SOUTH_CLOCK_TOWN, 0);
    gSaveContext.save.cutsceneIndex = 0;
    gSaveContext.save.hasTatl = true;
    gSaveContext.save.playerForm = PLAYER_FORM_HUMAN;
    gSaveContext.save.saveInfo.playerData.threeDayResetCount = 1;
    gSaveContext.save.isFirstCycle = true;
    SET_WEEKEVENTREG(WEEKEVENTREG_59_04);                                                  // Tatl
    SET_WEEKEVENTREG(WEEKEVENTREG_31_04);                                                  // Tatl
    gSaveContext.save.saveInfo.permanentSceneFlags[SCENE_INSIDETOWER].switch0 |= (1 << 0); // Happy Mask Salesman

    // Remove Sword & Shield
    SET_EQUIP_VALUE(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_NONE);
    BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) = ITEM_NONE;
    SET_EQUIP_VALUE(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_NONE);
}
std::string GenerateSeedInSaveContext(std::string inputSeed) {
    if (inputSeed.empty()) {
        inputSeed = std::to_string(Ship_Random(0, 1000000));
    }

    SPDLOG_INFO("Generating new randomizer with seed: {}", inputSeed);
    uint32_t finalSeed = Ship_Hash(inputSeed);
    Ship_Random_Seed(finalSeed);

    // Persist options to the save
    gSaveContext.save.shipSaveInfo.rando.finalSeed = finalSeed;
    for (auto& [randoOptionId, randoStaticOption] : Rando::StaticData::Options) {
        RANDO_SAVE_OPTIONS[randoOptionId] =
            (uint32_t)CVarGetInteger(randoStaticOption.cvar, randoStaticOption.defaultValue);
    }

    // Imported/headless settings follow the same policy as the menu.
    VanillaCompatibility::NormalizeOptions(gSaveContext.save.shipSaveInfo.rando);

    // If Skulltula tokens are not shuffled, use the vanilla requirement
    if (!RANDO_SAVE_OPTIONS[RO_SHUFFLE_GOLD_SKULLTULAS]) {
        RANDO_SAVE_OPTIONS[RO_SKULLTULA_TOKENS_REQUIRED] = SPIDER_HOUSE_TOKENS_REQUIRED;
    }

    // Starting with every Stray Fairy means the Great Fairies always have their full set
    if (RANDO_SAVE_OPTIONS[RO_PLACEMENT_STRAY_FAIRIES] == RO_DUNGEON_ITEM_START_WITH) {
        RANDO_SAVE_OPTIONS[RO_STRAY_FAIRIES_REQUIRED] = STRAY_FAIRY_SCATTERED_TOTAL;
    }

    // Vanilla Stray Fairies never enter the item pool, so every dungeon keeps its full set
    if (RANDO_SAVE_OPTIONS[RO_PLACEMENT_STRAY_FAIRIES] == RO_DUNGEON_ITEM_VANILLA) {
        RANDO_SAVE_OPTIONS[RO_STRAY_FAIRIES_MAX] = STRAY_FAIRY_SCATTERED_TOTAL;
    }

    // Persist StartingItems to the save
    auto startingItems = Rando::GetStartingItemsFromConfig();
    Rando::SetStartingItemsInSave(gSaveContext.save.shipSaveInfo.rando, startingItems);

    // Persist SariaPriorityItems to the save
    auto priorityItems = Rando::GetSariaPriorityItemsFromConfig();
    Rando::SetSariaPriorityItemsInSave(gSaveContext.save.shipSaveInfo.rando, priorityItems);

    std::vector<RandoCheckId> checkPool;
    std::vector<RandoItemId> itemPool;
    Rando::Logic::GeneratePools(gSaveContext.save.shipSaveInfo.rando, checkPool, itemPool);

    if (checkPool.empty()) {
        throw std::runtime_error("Check pool is empty");
    }
    if (itemPool.empty()) {
        throw std::runtime_error("Item pool is empty");
    }

    // Balance pools
    int heartPiecesRemoved = 0;
    while (checkPool.size() != itemPool.size()) {
        if (checkPool.size() > itemPool.size()) {
            itemPool.push_back(RI_JUNK);
        } else {
            // First, remove junk items if we have any
            bool removedJunk = false;
            for (int i = 0; i < itemPool.size(); i++) {
                if (Rando::StaticData::Items[itemPool[i]].randoItemType == RITYPE_JUNK) {
                    itemPool.erase(itemPool.begin() + i);
                    removedJunk = true;
                    break;
                }
            }
            if (removedJunk) {
                continue;
            }

            // Next replace 4 heart pieces with a heart container
            bool removedHeartPieces = false;
            for (int i = 0; i < itemPool.size(); i++) {
                if (Rando::StaticData::Items[itemPool[i]].randoItemId == RI_HEART_PIECE) {
                    if (heartPiecesRemoved == 0) {
                        itemPool[i] = RI_HEART_CONTAINER;
                    } else {
                        itemPool.erase(itemPool.begin() + i);
                    }

                    removedHeartPieces = true;
                    heartPiecesRemoved++;
                    if (heartPiecesRemoved == 4) {
                        heartPiecesRemoved = 0;
                    }
                    break;
                }
            }

            if (removedHeartPieces) {
                continue;
            }

            SPDLOG_ERROR("Could not balance item/check pools. Too many items. {}/{}", itemPool.size(),
                         checkPool.size());
            throw std::runtime_error("Could not balance item/check pools. Too many items.");
        }
    }

    // Grant the starting stuff
    Rando::GrantStartingItems();

    // Run prelim compatibility/validation checks before attempting to place items

    // Verify we have at least one time item if clock shuffle is enabled
    if (RANDO_SAVE_OPTIONS[RO_CLOCK_SHUFFLE] != 0) {
        if (Rando::Logic::ClockCount() == 0) {
            throw std::runtime_error("Shuffle Time is enabled but no starting time was given");
        }
    }

    if (RANDO_SAVE_OPTIONS[RO_LOGIC] == RO_LOGIC_VANILLA) {
        GiveItem(RI_SWORD_KOKIRI);
        GiveItem(RI_SHIELD_HERO);

        for (auto& [randoCheckId, randoStaticCheck] : Rando::StaticData::Checks) {
            if (randoStaticCheck.randoCheckId != RC_UNKNOWN) {
                RANDO_SAVE_CHECKS[randoCheckId].randoItemId = randoStaticCheck.randoItemId;
                RANDO_SAVE_CHECKS[randoCheckId].shuffled = true;
            }
        }
        VanillaCompatibility::NormalizeSavedState(gSaveContext.save.shipSaveInfo.rando);
    } else if (RANDO_SAVE_OPTIONS[RO_LOGIC] == RO_LOGIC_NO_LOGIC) {
        Rando::Logic::ApplyNoLogicToSaveContext(checkPool, itemPool);
    } else if (RANDO_SAVE_OPTIONS[RO_LOGIC] == RO_LOGIC_NEARLY_NO_LOGIC) {
        Rando::Logic::ApplyNearlyNoLogicToSaveContext(checkPool, itemPool);
    } else if (RANDO_SAVE_OPTIONS[RO_LOGIC] == RO_LOGIC_GLITCHLESS) {
        // Various things can result in failed generation now (confined placement, junked items,
        // too many traps, etc), so we retry a few times before giving up
        const int maxAttempts = 10;
        for (int attempt = 1; attempt <= maxAttempts; attempt++) {
            std::vector<RandoCheckId> attemptCheckPool = checkPool;
            std::vector<RandoItemId> attemptItemPool = itemPool;
            try {
                Rando::Logic::ApplyGlitchlessLogicToSaveContext(attemptCheckPool, attemptItemPool);
                break;
            } catch (const std::exception& e) {
                if (attempt >= maxAttempts) {
                    throw;
                }
                SPDLOG_WARN("Glitchless generation attempt {} failed ({}), retrying", attempt, e.what());
            }
        }
    } else {
        throw std::runtime_error("Logic option not implemented: " + std::to_string(RANDO_SAVE_OPTIONS[RO_LOGIC]));
    }
    return inputSeed;
}
} // namespace Rando
