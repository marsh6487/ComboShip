#!/usr/bin/env python3
"""Execute actual combo update bodies while the GUI suppresses passive Draw calls."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
DEPS = Path(os.environ.get("UI_VISIBILITY_DEPS", os.environ.get("NLOHMANN_JSON_INCLUDE", "/usr/include")))
IMGUI = Path(os.environ.get("UI_VISIBILITY_IMGUI_DIR", os.environ.get("IMGUI_DIR", DEPS / "imgui")))
if not IMGUI.is_dir() and (DEPS / "imgui-src").is_dir():
    IMGUI = DEPS / "imgui-src"


def block(source, pattern):
    match = re.search(pattern, source, re.M)
    if not match:
        raise RuntimeError("Missing production block: " + pattern)
    index, depth = match.end(), 1
    while depth:
        depth += (source[index] == "{") - (source[index] == "}")
        index += 1
    return source[match.start():index]


def function(source, name):
    return block(source, r"^[^\n;{}]*\b" + re.escape(name) + r"\([^;{}]*\)\s*\{") + "\n"


def update(source, name):
    # The original classes implement an empty inline UpdateElement; keep that
    # behavior in the pre-fix fixture so failures are runtime contract assertions.
    qualified = name + "::UpdateElement"
    if qualified in source:
        return function(source, qualified)
    return "void " + qualified + "() {}\n"


def main():
    if not (IMGUI / "imgui.cpp").is_file():
        raise SystemExit("Set IMGUI_DIR to the ImGui source directory (UI_VISIBILITY_DEPS is optional)")
    tracker = (ROOT / "combo/gui/ComboTrackerSwap.cpp").read_text()
    sync = (ROOT / "combo/gui/ComboSettingsSync.cpp").read_text()
    timer = (ROOT / "combo/gui/ComboTimersWindow.cpp").read_text()
    notes = (ROOT / "combo/gui/ComboNotesWindow.cpp").read_text()
    fixture = (ROOT / "tests/ui_visibility/combo_state_test.cpp").read_text()
    fixture = fixture.replace("/* TRACKER_FUNCTIONS */", "\n".join(
        function(tracker, name) for name in ("DormantPausedOnly", "ApplyDerived", "Reconcile")) +
        update(tracker, "SwapWindow"))
    fixture = fixture.replace("/* SYNC_FUNCTIONS */", "\n".join(
        function(sync, name) for name in ("ReadSide", "TranslateToPeer", "WriteSide", "GroupEnabled", "Reconcile")))
    fixture = fixture.replace("/* SYNC_CLASS */", block(sync, r"^class SyncWindow[^\n]*\{") + ";\n")
    fixture = fixture.replace("/* TIMER_FUNCTIONS */", function(timer, "SuppressNativeOverlays") +
        update(timer, "ComboTimersWindow"))
    fixture = fixture.replace("/* NOTES_FUNCTIONS */", function(notes, "ComboNotes::FlushPending") +
        update(notes, "ComboNotesWindow"))
    constants = "\n".join(re.findall(r"^constexpr int kOotTextSpeed\w+ = .*?;", sync, re.M))
    fixture = fixture.replace("namespace sync_case {", "namespace sync_case {\n" + constants)
    fixture_paths = [path for path in os.environ.get("COMBO_NEI_FIXTURE_CPATH", "").split(os.pathsep) if path]
    flags = ["-std=c++20", "-g", "-O0", "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
             "-fno-pie", "-no-pie", "-I" + str(IMGUI),
             *["-I" + path for path in fixture_paths],
             *shlex.split(os.environ.get("UI_VISIBILITY_CXXFLAGS", ""))]
    with tempfile.TemporaryDirectory(prefix="combo-hidden-ui-") as directory:
        source = Path(directory) / "state.cpp"
        binary = Path(directory) / "state"
        source.write_text(fixture)
        imgui = [IMGUI / name for name in ("imgui.cpp", "imgui_draw.cpp", "imgui_tables.cpp", "imgui_widgets.cpp")]
        subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), *flags, str(source),
                        *map(str, imgui), "-o", str(binary)], check=True, cwd=ROOT)
        results = [subprocess.run([str(binary), case], cwd=ROOT,
                                 env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"}).returncode
                   for case in (sys.argv[1:] or ["all"])]
        if any(results):
            raise SystemExit(1)


if __name__ == "__main__":
    main()
