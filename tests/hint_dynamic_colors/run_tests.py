#!/usr/bin/env python3
"""Run native MM sensor and moon-child hint bodies with the real color encoder.

MM layouts, save flags, item slots, callback bodies and color conversion are
production code. Desired-item CVars, location/name lookup and the font/window
services are controlled boundaries; this does not launch the game.
"""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from


def body(text, signature):
    return block_from(text, text.index(signature))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", choices=("all", "sensor", "remains", "forms", "selectors", "picker"), default="all")
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--standalone", action="store_true")
    args = parser.parse_args()
    rando = (ROOT / "mm/2s2h/Rando/Rando.cpp").read_text()
    moon = (ROOT / "mm/2s2h/Rando/ActorBehavior/EnJs.cpp").read_text()
    message = (ROOT / "mm/2s2h/CustomMessage/CustomMessage.cpp").read_text()
    menu = (ROOT / "mm/2s2h/BenGui/BenMenu.cpp").read_text()
    functions = "static std::string sSensorHintText;\n" + "\n".join(body(rando, signature) for signature in (
        "std::vector<RandoCheckId> Rando::FindMultiItemPlacement(",
        "static RandoItemId SensorGetDesire(", "static RandoCheckId SensorFindOutstandingCheck(",
        "Rando::HintTarget Rando::GetDesiredItemHintTarget(",
        'extern "C" u8 SensorRune_PrepareHint(', 'extern "C" void SensorRune_OpenHint('))
    functions += "\n" + body(moon, "void OverrideSubJsText(")
    functions += "\n" + "\n".join(body(menu, signature) for signature in (
        "const std::vector<std::pair<int32_t, std::string>>& SensorItemChoices(",
        "std::string SensorDesireName(", "void DrawSensorDesirePicker("))
    encoder = "\n".join(body(message, signature) for signature in (
        "void CustomMessage::Replace(", "void CustomMessage::ReplaceColorChars("))
    fixture = (ROOT / "tests/hint_dynamic_colors/test.cpp").read_text()
    fixture = fixture.replace("/* PRODUCTION */", functions).replace("/* COLOR_ENCODER */", encoder)
    includes = (".", "mm", "mm/include", "mm/include/PR", "mm/src", "mm/assets", "mm/2s2h",
                "combo", "combo/menu", "libultraship/include")
    with tempfile.TemporaryDirectory(prefix="mm-dynamic-hint-colors-") as temporary:
        source = Path(temporary) / "test.cpp"
        source.write_text(fixture)
        binary = Path(temporary) / "test"
        command = [*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-DF3DEX_GBI_2",
                   *([] if args.standalone else ["-DCOMBO_BUILD"]), "-DMM_BUILD_DLL",
                   "-I" + os.environ.get("NLOHMANN_JSON_INCLUDE", "/usr/include"),
                   *["-I" + str(ROOT / path) for path in includes]]
        if args.sanitize:
            command += ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie", "-no-pie"]
        subprocess.run([*command, str(source), "-o", str(binary)], check=True)
        failed = False
        for case in (("sensor", "remains", "forms", "selectors", "picker") if args.case == "all" else (args.case,)):
            result = subprocess.run([str(binary), case], env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
            failed |= result.returncode != 0
        return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
