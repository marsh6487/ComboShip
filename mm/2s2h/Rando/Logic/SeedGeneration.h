#pragma once
#include <string>
namespace Rando {
// Shared native file-create phases, without UI/audio/file IO.
void PrepareNewRandomizedSave();
std::string GenerateSeedInSaveContext(std::string inputSeed);
} // namespace Rando
