#pragma once
// GUI declaration boundary only. No GUI code is executed by the native solver
// fixture.
struct ImVec2 {
  float x, y;
  constexpr ImVec2(float a = 0, float b = 0) : x(a), y(b) {}
};
struct ImVec4 {
  float x, y, z, w;
  constexpr ImVec4(float a = 0, float b = 0, float c = 0, float d = 0)
      : x(a), y(b), z(c), w(d) {}
};
using ImGuiComboFlags = int;
using ImGuiSliderFlags = int;
using ImGuiInputFlags = int;
