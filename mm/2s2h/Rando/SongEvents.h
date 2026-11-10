#pragma once
#include "Rando/Logic/Logic.h"
#include "combo/rando/SongEvents.h"
namespace Rando::SongEvents {
void Generate(RandoSaveInfo& info, bool prepareForOracle = true);
void ClearImportedMapping();
void RestoreForOracle(RandoSaveInfo& info);
void RememberSavedMapping(const RandoSaveInfo& info);
void Import(RandoSaveInfo& info, const nlohmann::json& spoiler);
bool CanPlay(unsigned event);
} // namespace Rando::SongEvents
