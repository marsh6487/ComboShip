#!/usr/bin/env python3
"""Run production sky/floor policy and menu registration; optionally syntax-check actual TUs."""
import argparse
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--sanitize", action="store_true")
parser.add_argument("--baseline-registration", help="negative control: use this ref's native menu registration")
parser.add_argument("--compile-commands", type=Path, help="native build compilation database for the policy/menu production TUs")
args = parser.parse_args()
graphics = ROOT / "soh/soh/Enhancements/Graphics"
required = [graphics / "ToonLighting.cpp", graphics / "WWSkyEnv.cpp",
            ROOT / "soh/soh/SohGui/SohMenuWindWakerStyle.cpp"]
missing = [str(p.relative_to(ROOT)) for p in required if not p.exists()]
if missing:
    sys.exit("FAIL: cel/sky production modules missing: " + ", ".join(missing))

source = (graphics / "ToonLighting.cpp").read_text()
signature = "static s16 ToonShadowFeetClamp("
if signature not in source:
    sys.exit("FAIL: missing wide-world shadow-floor transport policy")
shadow = block_from(source, source.index(signature))
env = (graphics / "WWSkyEnv.cpp").read_text()
env = env[env.index("// Shared horizon placement"):]
menu = required[2].read_text()
menu = block_from(menu, menu.index("void SohMenu::AddMenuWindWakerStyle()"))
registration = (ROOT / "soh/soh/SohGui/SohMenu.cpp").read_text()
if args.baseline_registration:
    registration = subprocess.check_output(
        ["git", "show", args.baseline_registration + ":soh/soh/SohGui/SohMenu.cpp"],
        cwd=ROOT, text=True, env=dict(os.environ, GIT_NO_LAZY_FETCH="1"))
registration = block_from(registration, registration.index("void SohMenu::AddMenuElements()"))
def function(path, signature):
    text = path.read_text()
    return block_from(text, text.index(signature))

cloud_texture = function(graphics / "WWClouds.cpp", "static WWCloudTexture FetchTex(")
cloud = function(graphics / "WWClouds.cpp", "static void WriteVtx(")
wisp = function(graphics / "WWWindWisps.cpp", "static void WriteWispVtx(")
stick_remove = function(graphics / "DekuStickLight.cpp", "void RemoveStickLight(")
stick_update = function(graphics / "DekuStickLight.cpp", "void UpdateDekuStickLight()")
stick_text = (graphics / "DekuStickLight.cpp").read_text()
stick_signature = 'extern "C" void DekuStickLight_SaveState('
stick_state = block_from(stick_text, stick_text.index(stick_signature)) if stick_signature in stick_text else 'extern "C" void DekuStickLight_SaveState(SaveStateCtx*) {}'

compiler = os.environ.get("CXX", "c++")
flags = ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"] if args.sanitize else []
with tempfile.TemporaryDirectory(prefix="cel-sky-") as temporary:
    build = Path(temporary)
    for name, marker, production in (("sky_environment", "PRODUCTION_SKY_ENV", env),
                                     ("shadow_floor", "PRODUCTION_SHADOW_FLOOR", shadow),
                                     ("menu", "PRODUCTION_MENU", menu),
                                     ("geometry", "PRODUCTION_CLOUD_WRITER", cloud),
                                     ("stick_light", "PRODUCTION_STICK_UPDATE", stick_update),
                                     ("cloud_texture", "PRODUCTION_CLOUD_TEXTURE", cloud_texture)):
        fixture = ROOT / "tests/cel_sky" / (name + "_test.cpp")
        cpp = build / (name + ".cpp")
        cpp.write_text(fixture.read_text().replace("/* " + marker + " */", production)
                       .replace("/* PRODUCTION_MENU_REGISTRATION */", registration)
                       .replace("/* PRODUCTION_WISP_WRITER */", wisp)
                       .replace("/* PRODUCTION_STICK_REMOVE */", stick_remove)
                       .replace("/* PRODUCTION_STICK_SAVESTATE */", stick_state))
        binary = build / name
        subprocess.run([compiler, "-std=c++20", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter",
                        *flags, "-I" + str(ROOT), str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)

if args.compile_commands:
    database = json.loads(args.compile_commands.read_text())
    units = [graphics / (name + ".cpp") for name in (
        "WWClouds", "WWNightSky", "WWSkyEnv", "WWSkyGradient", "WWWindWisps",
        "ToonLighting", "WorldLighting", "DekuStickLight")]
    units += [required[2], ROOT / "soh/soh/SohGui/SohMenu.cpp"]
    checked = 0
    for unit in units:
        entry = next((e for e in database if (Path(e["directory"]) / e["file"]).resolve() == unit.resolve()), None)
        if not entry:
            sys.exit("FAIL: compilation database lacks " + str(unit.relative_to(ROOT)))
        command = entry.get("arguments") or shlex.split(entry["command"])
        cleaned = []
        skip = False
        for part in command:
            if skip:
                skip = False
                continue
            if part in ("-o", "-MF", "-MT", "-MQ"):
                skip = True
            elif part not in ("-c", "-MD", "-MMD"):
                cleaned.append(part)
        subprocess.run([*cleaned, "-fsyntax-only"], cwd=entry["directory"], check=True)
        checked += 1
    print(f"cel sky native syntax: {checked} production TUs PASS")
