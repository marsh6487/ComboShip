#!/usr/bin/env python3
"""Execute both actual Harpoon map-select methods with hidden Draw omitted."""
import os
from pathlib import Path
import subprocess
import tempfile

import gui_run_tests as gui

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / "soh/soh/Network/Harpoon"


def class_block(source, start):
    end, depth = source.index("{", start) + 1, 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def main():
    flags = ["-std=c++20", "-g", "-O1", "-I" + str(gui.DEPS), "-I" + str(gui.IMGUI)]
    flags += ["-I" + path for path in os.environ.get("COMBO_NEI_FIXTURE_CPATH", "").split(os.pathsep) if path]
    with tempfile.TemporaryDirectory(prefix="harpoon-vote-ui-") as directory:
        work = Path(directory)
        objects = gui.imgui_objects(work, flags)
        failures = []
        for mode, actor_class, prefix, cursor in (
                ("prop_hunt", "PropHuntMapSelectWindow", "", "PROP_HUNT.MAP_CURSOR"),
                ("triforce_thief", "TriforceThiefMapSelectWindow", "TT", "TRIFORCE_THIEF.MAP_HOVER")):
            folder = "PropHunt" if not prefix else "TriforceThief"
            source = (BASE / folder / (folder + "MapSelect.cpp")).read_text()
            globals_begin = source.index("struct MapDef" if not prefix else "struct TTMapDef")
            globals_end = source.index("void SafeLoadTexture")
            actor = class_block(source, source.index("class " + actor_class))
            fixture = (ROOT / "tests/ui_visibility/gui_harpoon_votes_test.cpp").read_text()
            fixture = fixture.replace("/* PRODUCTION_GLOBALS */", source[globals_begin:globals_end])
            fixture = fixture.replace("/* PRODUCTION_CLASS */", actor + ";\nusing MapSelectWindow = " + actor_class)
            fixture = ('#define TEST_MODE "' + mode + '"\n#define TEST_CURSOR "' + cursor +
                       '"\n#define TEST_VOTE "' + mode.upper() + '.MAP_VOTE"\n' + fixture)
            for alias, name in (("TexturesLoaded", "TexturesLoaded"), ("HasVoted", "HasVoted"),
                                ("StickDebounce", "StickDebounce")):
                fixture = "#define " + alias + " s" + prefix + name + "\n" + fixture
            path = work / (mode + ".cpp")
            path.write_text(fixture)
            binary = work / mode
            gui.run(gui.CXX, *flags, "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                    "-fno-pie", "-no-pie", path, *objects, "-o", binary)
            try:
                gui.run(binary)
            except subprocess.CalledProcessError:
                failures.append(mode)
        if failures:
            raise SystemExit("Failed hidden vote input: " + ", ".join(failures))


if __name__ == "__main__":
    main()
