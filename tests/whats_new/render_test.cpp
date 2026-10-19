// Exercise the production release helper and both unchanged native checkbox
// renderers against the project's real, patched Dear ImGui. No GPU is required.
#include "gui/WhatsNew.h"
#include <imgui_internal.h>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>

static void Check(bool condition, const char *why) {
  if (!condition)
    throw std::runtime_error(why);
}
static bool Ship_IsCStringEmpty(const char *text) { return !text || !text[0]; }
// Only native option/storage dependencies are fixtures; the checkbox drawing,
// input behavior, labels and release scope below are the exact production code.
// MM's existing WrappedText compares its signed line counter with an unsigned
// width. Keep that unrelated function unchanged in this extraction probe.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "native_checkboxes.inc"
#pragma GCC diagnostic pop

struct Result {
  ImGuiID id;
  ImVec2 min, max;
  float right;
  bool changed;
  bool tooltip;
};
template <typename Draw>
Result Frame(Draw draw, bool enabled, bool disabled, bool &value,
             ImVec2 mouse = ImVec2(-100, -100), bool mouseDown = false,
             float width = 400) {
  auto &io = ImGui::GetIO();
  io.AddMousePosEvent(mouse.x, mouse.y);
  io.AddMouseButtonEvent(0, mouseDown);
  ImGui::NewFrame();
  ImGui::SetNextWindowPos(ImVec2(20, 20));
  ImGui::SetNextWindowSize(ImVec2(width, 300));
  ImGui::Begin("Release test", nullptr,
               ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar);
  const char *label = "Always Thawed Zora's Domain";
  const ImGuiID original = ImGui::GetID(label);
  const ImVec4 normal = ImGui::GetStyle().Colors[ImGuiCol_Text];
  const int colors = GImGui->ColorStack.Size;
  Result result{};
  result.right = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
  {
    WhatsNew::Scope highlight("gEnhancements.AlwaysThawedZorasDomain", label,
                              enabled, disabled);
    Check((GImGui->ColorStack.Size == colors + 1) == (enabled && !disabled),
          "only active controls get an accent");
    result.changed = draw(label, &value, disabled);
    result.id = ImGui::GetItemID();
    result.min = ImGui::GetItemRectMin();
    result.max = ImGui::GetItemRectMax();
    Check(result.id == original,
          "release markers must preserve the original control ID");
  }
  Check(GImGui->ColorStack.Size == colors,
        "release scope must balance the real style stack");
  const ImVec4 restored = ImGui::GetStyle().Colors[ImGuiCol_Text];
  Check(restored.x == normal.x && restored.y == normal.y &&
            restored.z == normal.z && restored.w == normal.w,
        "later unrelated controls must retain their original text color");
  ImGui::End();
  ImGui::Render();
  for (auto *window : GImGui->Windows)
    result.tooltip |=
        window->Active && (window->Flags & ImGuiWindowFlags_Tooltip);
  return result;
}

template <typename Draw> void Probe(Draw draw) {
  ImGui::CreateContext();
  auto &io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.DisplaySize = ImVec2(900, 600);
  io.DeltaTime = 1.0f / 60;
  unsigned char *pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  bool value = false;
  Frame(draw, false, false, value);
  const auto original = Frame(draw, false, false, value);
  const auto highlighted = Frame(draw, true, false, value);
  Check(original.id == highlighted.id && !value,
        "styling alone must not toggle a setting");
  ImVec2 hit(highlighted.min.x + 8,
             (highlighted.min.y + highlighted.max.y) * 0.5f);
  Frame(draw, true, false, value, hit);
  Frame(draw, true, false, value, hit, true);
  const auto clicked = Frame(draw, true, false, value, hit, false);
  Check(clicked.changed && value,
        "highlighted native controls must still accept clicks");
  const auto disabled = Frame(draw, true, true, value);
  hit = ImVec2(disabled.min.x + 8, (disabled.min.y + disabled.max.y) * 0.5f);
  Frame(draw, true, true, value, hit, true);
  const auto locked = Frame(draw, true, true, value, hit, false);
  Check(!locked.changed && value,
        "release markers must not bypass native disabled controls");
  const auto off = Frame(draw, false, false, value);
  Check(off.min.x == original.min.x && off.min.y == original.min.y,
        "off restores the original layout");
  // Both original native labels fit here. The marker must respect a narrow
  // column.
  for (int windowWidth = 250; windowWidth <= 340; ++windowWidth) {
    const auto narrow =
        Frame(draw, true, false, value, ImVec2(-100, -100), false, windowWidth);
    Check(narrow.max.x <= narrow.right + 1,
          "a release marker must not push native controls outside a narrow "
          "column");
  }
  // Hover the marker, which is the first non-interactive text item in this
  // window.
  Frame(draw, true, false, value, ImVec2(30, 31));
  const auto hover = Frame(draw, true, false, value, ImVec2(30, 31));
  Check(hover.tooltip, "hovering the release marker shows its change note");
  ImGui::DestroyContext();
}

int main() {
  try {
    std::set<std::string> keys;
    for (const auto &entry : WhatsNew::kControls) {
      Check(keys.insert(entry.key).second && entry.note[0],
            "release keys must be unique and explain their change");
      Check(WhatsNew::Find(entry.key) == &entry,
            "every release key must resolve exactly");
    }
    Check(!WhatsNew::Find(nullptr) && !WhatsNew::Find("") &&
              !WhatsNew::Find("gSettings.Menu.Theme"),
          "unrelated and missing keys keep normal styling");
    Check(!WhatsNew::Find("gEnhancements.AlwaysThawedZorasDomain.Unrelated"),
          "release lookup is not a prefix match");
    Check(!WhatsNew::Find("gSettings.Menu.Theme", "Black Market Appearance"),
          "an unrelated CVar owns its identity");
    Check(WhatsNew::Find(nullptr, "Black Market Appearance"),
          "custom sections have explicit identities");
    Probe([](const char *label, bool *value, bool disabled) {
      OotWidgets::CheckboxOptions options;
      options.disabled = disabled;
      return OotWidgets::Checkbox(label, value, options);
    });
    Probe([](const char *label, bool *value, bool disabled) {
      MmWidgets::CheckboxOptions options;
      options.disabled = disabled;
      return MmWidgets::Checkbox(label, value, options);
    });
    Probe([](const char *label, bool *value, bool disabled) {
      ImGui::BeginDisabled(disabled);
      const bool changed = ImGui::Checkbox(label, value);
      ImGui::EndDisabled();
      return changed;
    });
    std::cout
        << "PASS: release inventory, native OoT/MM and combo fallback IDs, "
           "input, disabled state, layout, hover and style restoration\n";
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
}
