#!/usr/bin/env python3
"""Run shared UI API and production GUI routing/notification boundaries."""
import ctypes
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
LUS = ROOT / "libultraship"
DEPS = Path(os.environ.get("UI_VISIBILITY_DEPS", os.environ.get("NLOHMANN_JSON_INCLUDE", "/usr/include")))
IMGUI = Path(os.environ.get("UI_VISIBILITY_IMGUI_DIR", os.environ.get("IMGUI_DIR", DEPS / "imgui")))
if not IMGUI.is_dir() and (DEPS / "imgui-src").is_dir():
    IMGUI = DEPS / "imgui-src"
CXX = os.environ.get("CXX", "c++")
CC = os.environ.get("CC", "cc")


def function(source, name):
    match = re.search(r"^[^\n;{}]*\b" + re.escape(name) + r"\([^;{}]*\)\s*\{", source, re.M)
    if not match:
        raise RuntimeError("Missing production function " + name)
    index, depth = match.end(), 1
    while depth:
        depth += (source[index] == "{") - (source[index] == "}")
        index += 1
    return source[match.start():index] + "\n"


def run(*args):
    subprocess.run(list(map(str, args)), cwd=ROOT, check=True,
                   env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


def api_test(work, flags):
    source = work / "cvars.cpp"
    source.write_text('''#include <cstdint>
#include <cassert>
#include <cstring>
static int enabled;
extern "C" void GuiTest_SetEnabled(int value) { enabled = value; }
extern "C" int32_t CVarGetInteger(const char* key, int32_t fallback) {
    assert(std::strcmp(key, "gEnhancements.TrueHiddenUI") == 0 && fallback == 0);
    return enabled;
}
''')
    exports = work / "exports.map"
    exports.write_text("{ global: UiVisibility_*; GuiTest_*; local: *; };\n")
    library = work / "libvisibility.so"
    run(CXX, *flags, "-fPIC", "-shared", "-ffunction-sections", "-fdata-sections",
        LUS / "src/libultraship/bridge/windowbridge.cpp", source, "-Wl,--gc-sections",
        "-Wl,--version-script=" + str(exports), "-o", library)
    api = ctypes.CDLL(str(library))
    assert hasattr(api, "UiVisibility_IsHidden"), "Shared UI visibility API is missing"
    assert not api.UiVisibility_IsEnabled() and not api.UiVisibility_IsHidden()
    api.UiVisibility_Toggle()  # Disabled calls must leave the initial hidden state intact.
    api.GuiTest_SetEnabled(1)
    assert api.UiVisibility_IsEnabled() and api.UiVisibility_IsHidden()
    api.UiVisibility_Toggle()
    assert not api.UiVisibility_IsHidden()
    api.GuiTest_SetEnabled(0)
    api.UiVisibility_Toggle()
    assert not api.UiVisibility_IsEnabled() and not api.UiVisibility_IsHidden()
    api.GuiTest_SetEnabled(1)
    assert not api.UiVisibility_IsHidden()
    # Separate C and C++ callers must observe one bit in the shared module.
    c = work / "caller.c"
    cpp = work / "caller.cpp"
    c.write_text('#include "libultraship/bridge/uivisibilitybridge.h"\n'
                 'int GuiTest_C(void) { UiVisibility_Toggle(); return UiVisibility_IsHidden(); }\n')
    cpp.write_text('#include "libultraship/bridge/uivisibilitybridge.h"\n'
                   'extern "C" int GuiTest_Cpp(void) { return UiVisibility_IsHidden(); }\n')
    run(CC, "-std=c11", "-fPIC", "-I" + str(LUS / "include"), "-c", c, "-o", work / "caller.o")
    run(CXX, "-std=c++20", "-shared", "-fPIC", "-I" + str(LUS / "include"), cpp, work / "caller.o",
        "-L" + str(work), "-lvisibility", "-Wl,-rpath," + str(work), "-o", work / "callers.so")
    callers = ctypes.CDLL(str(work / "callers.so"))
    assert callers.GuiTest_C() == 1 and callers.GuiTest_Cpp() == 1 and api.UiVisibility_IsHidden() == 1
    api.UiVisibility_Toggle()
    assert callers.GuiTest_Cpp() == 0
    print("PASS real shared API: C/C++ callers, disabled no-op, one transient bit, no unrelated CVar reads", flush=True)


def route_test(work, flags):
    source = (LUS / "src/ship/window/gui/Gui.cpp").read_text()
    fixture = (ROOT / "tests/ui_visibility/gui_route_test.cpp").read_text()
    policy = function(source, "GuiWindowAllowedWhenUiHidden") if "GuiWindowAllowedWhenUiHidden(" in source else ""
    fixture = fixture.replace("/* PRODUCTION_WINDOW_POLICY */", policy)
    fixture = fixture.replace("/* PRODUCTION_DRAW_MENU */", function(source, "Gui::DrawMenu") +
                              function(source, "Gui::EndDraw"))
    fixture = "#define TOGGLE_BTN ImGuiKey_F1\n#define TOGGLE_PAD_BTN ImGuiKey_GamepadBack\n" + fixture
    test = work / "route.cpp"
    test.write_text(fixture)
    binary = work / "route"
    run(CXX, *flags, "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie", "-no-pie",
        test, *imgui_objects(work, flags), "-o", binary)
    run(binary)


def overlay_test(work, flags):
    source = (LUS / "src/ship/window/gui/GameOverlay.cpp").read_text()
    fixture = (ROOT / "tests/ui_visibility/gui_overlay_test.cpp").read_text()
    fixture = fixture.replace("/* PRODUCTION_OVERLAY */", "\n".join(
        function(source, name) for name in ("GameOverlay::CleanupNotifications", "GameOverlay::Draw")))
    test = work / "overlay.cpp"
    test.write_text(fixture)
    binary = work / "overlay"
    run(CXX, *flags, "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie", "-no-pie",
        test, *imgui_objects(work, flags), "-o", binary)
    run(binary)


def imgui_objects(work, flags):
    objects = []
    # ImGui itself is an unmodified external dependency. Compile it once; the
    # production routes/lifetime functions above carry sanitizer instrumentation.
    for name in ("imgui", "imgui_draw", "imgui_tables", "imgui_widgets"):
        output = work / (name + ".o")
        if not output.is_file():
            run(CXX, *flags, "-g0", "-O0", "-c", IMGUI / (name + ".cpp"), "-o", output)
        objects.append(output)
    return objects


def compile_gui_objects(work, flags):
    # Compile full production translation units with their public headers and
    # the same CVar names CMake supplies; runtime services are not linked here.
    cvars = (LUS / "cmake/cvars.cmake").read_text()
    defines = ["-D" + name + '="' + value + '"' for name, value in re.findall(
        r'set\((CVAR_[A-Z_]+) "([^"]*)" CACHE', cvars)]
    for name in ("Gui", "GameOverlay"):
        run(CXX, *flags, *defines, "-c", LUS / ("src/ship/window/gui/" + name + ".cpp"),
            "-o", work / (name + ".o"))
    print("PASS full Gui/GameOverlay production-header compilation", flush=True)


def main():
    if not (IMGUI / "imgui.cpp").is_file():
        raise SystemExit("Set IMGUI_DIR to the ImGui source directory (UI_VISIBILITY_DEPS is optional)")
    fixture_paths = [path for path in os.environ.get("COMBO_NEI_FIXTURE_CPATH", "").split(os.pathsep) if path]
    flags = ["-std=c++20", "-g", "-O1", "-I" + str(LUS / "include"), "-I" + str(DEPS),
             "-I" + str(IMGUI), "-isystem", str(DEPS / "spdlog-src/include"),
             "-I" + str(DEPS / "thread-pool/include"),
             *["-I" + path for path in fixture_paths],
             "-idirafter", str(ROOT / "tests/mm_frame_timing/stubs"),
             *shlex.split(os.environ.get("UI_VISIBILITY_CXXFLAGS", ""))]
    with tempfile.TemporaryDirectory(prefix="gui-visibility-") as directory:
        work = Path(directory)
        cases = sys.argv[1:] or ["api", "route", "overlay"]
        if "api" in cases:
            api_test(work, flags)
        if "route" in cases:
            route_test(work, flags)
        if "overlay" in cases:
            overlay_test(work, flags)
        if not sys.argv[1:] or "compile" in cases:
            compile_gui_objects(work, flags)


if __name__ == "__main__":
    main()
