#pragma once
#include <string>

namespace SOH::Unbound {
// Compares the effective legacy scene/room/alternate/collision/path graph with
// this scene's JSON layers under OOT's owning RM and its current Alt setting.
bool LegacySceneOverridesJson(const std::string& legacyScene, const std::string& jsonScene);
} // namespace SOH::Unbound
