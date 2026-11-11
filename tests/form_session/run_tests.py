#!/usr/bin/env python3
"""Exercise the production imported-form lifetime and real save-session entries.

The full form Init, native interruption, Fleet setter, save-selection and
save-creation bodies are compiled. Update is compiled through its pending-form
consumer; resource loading, audio and native disk I/O are service boundaries.
The small engine structs in session_test.cpp explicitly model that boundary;
this is a state/lifecycle test, not an engine ABI or rendering proof.
"""
from pathlib import Path
import argparse
import os
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
CASES = ("discard", "file-switch", "same-slot-load", "new-native", "new-rando",
         "same-slot-recreate", "preflight-refusal", "pending", "native-before-flash",
         "native-after-flash", "scene", "fleet", "aim", "owned-model", "external-model")


def function(source, name):
    match = re.search(r"^(?:extern \"C\" )?(?:static )?[^\n;{}]*\b" + re.escape(name) +
                      r"\([^;{}]*\)\s*\{", source, re.M)
    if not match:
        raise ValueError("Missing production function: " + name)
    begin = source.index("{", match.start())
    depth, end = 1, begin + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


def build_source():
    form = (ROOT / "soh/mods/transformation_masks/mm_player_form.cpp").read_text()
    native = (ROOT / "soh/mods/transformation_masks/mm_native_transform.inc.h").read_text()
    player = (ROOT / "soh/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
    choose = (ROOT / "soh/src/overlays/gamestates/ovl_file_choose/z_file_choose.c").read_text()
    sram = (ROOT / "soh/src/code/z_sram.c").read_text()
    fixture = (ROOT / "tests/form_session/session_test.cpp").read_text()
    # A missing discard is deliberately a no-op for the RED run: the test then
    # fails on the leaked form at a real session entry, not a missing symbol.
    try:
        discard = function(form, "MmForm_DiscardSaveSession")
    except ValueError:
        discard = "void MmForm_DiscardSaveSession(void) {}"
    start = form.index("enum FDHandDLIndex {")
    fd_enum = form[start:form.index("};", start) + 2]
    cache_region = form[form.index("// Pre-loaded & Validated DL Pointers"):form.index("// OTR paths for FD hand DLs")]
    cache_declarations = re.findall(r"^static (?:std::vector<Gfx>|size_t|Gfx\*)[^\n]*;", cache_region, re.M)
    fixture = fixture.replace("/* PRODUCTION_CACHE_DECLARATIONS */", fd_enum + "\n" + "\n".join(cache_declarations))
    definitions = [function(form, name) for name in
                   ("MmForm_FreeRootMotion", "MmForm_ClearCachedDLs", "MmForm_UnpinFormResources")]
    definitions += [discard, function(native, "MmForm_InterruptNativeMaskForScene"),
                    function(form, "MmForm_DiscardUnownedRitoReload"), function(form, "MmForm_Init"),
                    function(form, "MmForm_FleetApplyForm")]
    update = function(form, "MmForm_Update")
    definitions.append(update[:update.index("    // === Pikachu Mode")] + "}\n")
    fixture = fixture.replace("/* PRODUCTION_FORM_FUNCTIONS */", "\n".join(definitions))
    for label, body in (("SELECT", function(choose, "FileChoose_LoadGame")),
                        ("CREATE", function(sram, "Sram_InitSave"))):
        fixture = fixture.replace("/* PRODUCTION_" + label + " */", re.sub(r"\bthis\b", "self", body))
    init = function(player, "Player_Init")
    init = init[:init.index("    TransformMasks_Init(play, this);")] + "    TransformMasks_Init(play, this);\n}\n"
    fixture = fixture.replace("/* PRODUCTION_PLAYER_INIT */", re.sub(r"\bthis\b", "self", init))
    return fixture


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", action="append", choices=CASES)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    flags = ["-std=c++20", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function", "-Wno-unused-variable",
             "-Wno-unused-parameter", "-I", str(ROOT)]
    if args.sanitize:
        flags += ["-O1", "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"]
    else:
        flags += ["-O2"]
    with tempfile.TemporaryDirectory(prefix="form-session-") as temp:
        path = Path(temp)
        source = path / "session.cpp"
        source.write_text(build_source())
        for combined in (False, True):
            binary = path / ("combo" if combined else "solo")
            defines = ["-DCOMBO_BUILD"] if combined else []
            subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), *flags, *defines,
                            str(source), "-o", str(binary)], check=True)
            for case in args.case or CASES:
                subprocess.run([str(binary), case], check=True,
                               env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
            print("PASS production form-session lifetime: " + ("ComboShip OoT" if combined else "solo SoH"), flush=True)


if __name__ == "__main__":
    main()
