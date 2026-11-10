#pragma once

#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include "combo/rando/CrossDungeonEntrances.h"

struct RandoSaveInfo;
struct RandoSoloEntranceSave;

namespace Rando::SoloEntrances {
// These conversions are pure. Reading a file-select preview never binds gameplay routing.
nlohmann::json Seed(const RandoSaveInfo& save);
void ApplySeed(RandoSaveInfo& save, const nlohmann::json& seed);
void Clear(RandoSaveInfo& save);
void ValidateSettings(const RandoSaveInfo& save);
std::vector<ComboRando::CrossDungeon::Route> BuildRoutes(const RandoSaveInfo& save);
std::vector<ComboRando::CrossDungeon::Dungeon> CatalogueForSave(const RandoSaveInfo& save);
int BossSourceForSave(const RandoSaveInfo& save, int physicalBoss);
bool BossesUsePhysicalClears(const RandoSaveInfo& save);

// Generation owns only the candidate save. Layout RNG is independent of item-fill RNG.
void Generate(RandoSaveInfo& save, uint32_t masterSeed, uint32_t attempt = 0);
void SelectForLogic(RandoSaveInfo& save, const std::function<bool(const RandoSoloEntranceSave&)>& isValid);

// Native logic reads the exact candidate/loaded save, including after inventory snapshots.
int ResolveEntrance(int index, int region);
bool BossLogicEnabled();
int LogicalClear(int sourceBoss, int nativeValue);

// Headless replay is an explicit oracle input, separate from both runtime and JSON previews.
void RestoreForOracle(RandoSaveInfo& save);
} // namespace Rando::SoloEntrances
