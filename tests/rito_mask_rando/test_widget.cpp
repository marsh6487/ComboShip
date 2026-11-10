#include "combo/rando/MagicMirrorSettingsSnapshot.h"
#include "combo/ComboGameMode.h"
#include "combo/gui/ComboMenuModel.h"
#include "combo/rando/RitoMaskSettingsSnapshot.h"
#include "combo/rando/RitoMaskSettingsSnapshot.h"
#include "soh/tests/test_require.h"
#include <cstdint>
#include <map>
#include <string>

#ifdef COMBO_BUILD
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
}
#endif
static std::map<std::string, int> values;
extern "C" int32_t CVarGetInteger(const char *name, int32_t fallback) {
  auto it = values.find(name);
  return it == values.end() ? fallback : it->second;
}
extern "C" void CVarSetInteger(const char *name, int32_t value) {
  values[name] = value;
}
extern "C" void CVarClear(const char *name) { values.erase(name); }

namespace M = ComboRando::RitoMaskSettings;
using Json = nlohmann::json;
namespace ComboRando {
// Native rendering/DLL dispatch is the service boundary. Execute the actual
// production adapter with the real CwWidget/GameMenu ABI and both callback
// slots.
ComboMenuModel &ComboMenuModel::Get() {
  static ComboMenuModel model;
  return model;
}
static int requested, observed, refresh[2];
static void RenderNativeWidget(const CwWidget &widget, const GameMenu &,
                               int width) {
  REQUIRE(width == 90 && widget.kind == CW_CHECKBOX);
  observed = CVarGetInteger(widget.cvar, -1);
  CVarSetInteger(widget.cvar, requested);
}
#include "native_widget.inc"
static void TestNativeTabs() {
  auto &model = ComboMenuModel::Get();
  const_cast<GameMenu &>(model.Oot()).applyCVarChange = [](const char *key) {
    REQUIRE(!std::strcmp(key, M::SharedCvar));
    ++refresh[0];
  };
  const_cast<GameMenu &>(model.Mm()).applyCVarChange = [](const char *key) {
    REQUIRE(!std::strcmp(key, M::SharedCvar));
    ++refresh[1];
  };
  for (int oot : {0, 1})
    for (int mm : {0, 1})
      for (int shared : {0, 1})
        for (const char *source : {M::OotCvar, M::MmCvar})
          for (int next : {0, 1}) {
            values = {
                {M::OotCvar, oot}, {M::MmCvar, mm}, {M::SharedCvar, shared}};
            CwWidget widget{};
            widget.kind = CW_CHECKBOX;
            widget.cvar = source;
            requested = next;
            refresh[0] = refresh[1] = 0;
            RenderWidget(widget,
                         source == M::OotCvar ? model.Oot() : model.Mm(), 90);
            REQUIRE(observed == shared && M::Shared() == next);
            REQUIRE(values[M::OotCvar] == oot && values[M::MmCvar] == mm);
            REQUIRE(refresh[0] == (next != shared) &&
                    refresh[1] == (next != shared));
          }
}
} // namespace ComboRando
int main() {
  ComboRando::TestNativeTabs();
  for (int oot : {0, 1})
    for (int mm : {0, 1})
      for (int shared : {0, 1}) {
        values = {{M::OotCvar, oot}, {M::MmCvar, mm}, {M::SharedCvar, shared}};
        const auto beforePreview = values;
        REQUIRE(M::Preview() == shared && values == beforePreview);
        REQUIRE(M::Shared() == shared);
        M::PrepareGeneration();
        REQUIRE(values[M::GenerationCvar] == shared);
        REQUIRE(values[M::OotCvar] == oot && values[M::MmCvar] == mm);
        for (const char *source : {M::OotCvar, M::MmCvar})
          for (int requested : {0, 1}) {
            M::SetShared(shared);
            {
              M::NativePreferenceScope edit(true);
              REQUIRE(values[M::OotCvar] == shared &&
                      values[M::MmCvar] == shared);
              CVarSetInteger(source, requested);
            }
            REQUIRE(M::Shared() == requested);
            REQUIRE(values[M::OotCvar] == oot && values[M::MmCvar] == mm);
            // Config/preset loads may rewrite unrelated CVars and even a stale
            // saved canonical key. Only the authored preset value is accepted.
            {
              M::NativePreferenceScope preset(true);
              CVarSetInteger(M::SharedCvar, !requested);
              CVarSetInteger(M::OotCvar, !oot);
              CVarClear(M::MmCvar);
              preset.Finish(requested);
            }
            REQUIRE(M::Shared() == requested);
            REQUIRE(values[M::OotCvar] == oot && values[M::MmCvar] == mm);
          }
        // Actual compile/session guard leaves solo native edits native.
        M::SetShared(shared);
        {
          M::NativePreferenceScope edit(Combo_IsCombinedGame());
          CVarSetInteger(M::OotCvar, !oot);
        }
#ifdef COMBO_BUILD
        REQUIRE(values[M::OotCvar] == oot && values[M::MmCvar] == mm);
#else
        REQUIRE(values[M::OotCvar] == !oot && values[M::MmCvar] == mm);
        REQUIRE(M::Shared() == shared);
#endif
      }
  values.clear();
  {
    M::NativePreferenceScope edit(true);
    CVarSetInteger(M::MmCvar, 1);
  }
  REQUIRE(M::Shared() == 1 && !values.contains(M::OotCvar) &&
          !values.contains(M::MmCvar));
  for (int oot : {0, 1})
    for (int mm : {0, 1}) {
      values = {{M::OotCvar, oot}, {M::MmCvar, mm}};
      const auto beforeMigrationPreview = values;
      REQUIRE(M::Preview() == (oot || mm) && values == beforeMigrationPreview);
      REQUIRE(M::Shared() == (oot || mm)); // absent-key migration only
      M::SetShared(0);
      REQUIRE(M::Shared() == 0);
      Json os = {{M::OotCvar, oot}}, ms = {{M::MmCvar, mm}};
      M::NormalizeSnapshots(os, ms);
      REQUIRE(os.at(M::SharedCvar).get<int>() == (oot || mm) &&
              ms.at(M::SharedCvar).get<int>() == (oot || mm));
      for (int shared : {0, 1})
        for (bool mmFirst : {false, true}) {
          os = {{M::OotCvar, oot}, {M::SharedCvar, shared}};
          ms = {{M::MmCvar, mm}};
          M::NormalizeSnapshots(os, ms);
          REQUIRE(os[M::GenerationCvar] == shared &&
                  ms[M::GenerationCvar] == shared);
          auto a = os, b = ms;
          if (mmFirst) {
            M::RestoreSnapshot(b);
            M::RestoreSnapshot(a);
          } else {
            M::RestoreSnapshot(a);
            M::RestoreSnapshot(b);
          }
          REQUIRE(M::Shared() == shared && values[M::GenerationCvar] == shared);
          REQUIRE(values[M::OotCvar] == oot && values[M::MmCvar] == mm);
          REQUIRE(!a.contains(M::OotCvar) && !b.contains(M::MmCvar));
        }
    }
  Json a = {{M::SharedCvar, 0}}, b = {{M::SharedCvar, 1}};
  bool rejected = false;
  try {
    M::NormalizeSnapshots(a, b);
  } catch (const std::runtime_error &) {
    rejected = true;
  }
  REQUIRE(rejected);
  a = {{M::SharedCvar, "ON"}};
  b = Json::object();
  rejected = false;
  try {
    M::NormalizeSnapshots(a, b);
  } catch (const std::runtime_error &) {
    rejected = true;
  }
  REQUIRE(rejected);
  for (int shared : {0, 1}) {
    Json config = {
        {"CVars",
         {{"gRandoSettings", {{"ShuffleRitoMask", 1}}},
          {"gRando", {{"Options", {{"RO_SHUFFLE_RITO_MASK", 1}}}}},
          {"gCombo", {{"Rando", {{"ShuffleRitoMask", shared}}}}}}}};
    M::NormalizeConfig(config);
    REQUIRE(config.at(Json::json_pointer(
                "/CVars/gCombo/Rando/ShuffleRitoMask")) == shared);
    REQUIRE(config.at(Json::json_pointer(
                "/CVars/gRandoSettings/ShuffleRitoMask")) == 1);
    REQUIRE(config.at(Json::json_pointer(
                "/CVars/gRando/Options/RO_SHUFFLE_RITO_MASK")) == 1);
    Json section = {{"gRandoSettings", {{"ShuffleRitoMask", shared}}}};
    REQUIRE(M::OotPresetValue(section, false, !shared) == shared);
    REQUIRE(M::OotPresetValue(section, true, !shared) == shared);
    REQUIRE(M::OotPresetValue({{"gRandoSettings", Json::object()}}, true,
                              shared) == shared);
    REQUIRE(M::OotPresetValue({{"gRandoSettings", Json::object()}}, false,
                              shared) == 0);
    REQUIRE(M::OotPresetValue({{"gRandoSettings", nullptr}}, true, shared) ==
            0);
    Json preset = {
        {"ClearCVars", {"/gRando"}},
        {"CVars",
         {{"gRando", {{"Options", {{"RO_SHUFFLE_RITO_MASK", shared}}}}}}}};
    REQUIRE(M::MmPresetValue(preset, !shared) == shared);
    REQUIRE(M::MmPresetValue({{"ClearCVars", {"/gRando/Options"}}}, shared) ==
            0);
    REQUIRE(M::MmPresetValue({{"ClearCVars", {"/gEnhancements"}}}, shared) ==
            shared);
    Json cvars = Json::object();
    M::StampNativePreset(cvars, true, shared);
    M::StampNativePreset(cvars, false, shared);
    REQUIRE(cvars.at(Json::json_pointer(
                "/gRandoSettings/ShuffleRitoMask")) == shared);
    REQUIRE(cvars.at(Json::json_pointer(
                "/gRando/Options/RO_SHUFFLE_RITO_MASK")) == shared);
  }
  puts("PASS actual shared Rito policy: both tabs/OFF, native solo "
       "preservation, authored presets, migration and ordered snapshots");
}
