#!/usr/bin/env python3
"""Execute both shaft adapters with real native host structs and native C wrappers.

Adapter and adult eligibility bodies are extracted verbatim. Player_DrawGetItem,
Player_GetHeight, and MM's reference-vector copy compile as C, without translating
the C-only `this` identifier. Controlled boundaries are CVar storage, GI loading,
model submission, rig readiness, and final shaft rendering. No game/visual claim;
this does not validate hook registration, full translation units, or forced PCH.
Use --mutation-check to prove focused cases reject temporary broken source copies.
"""
import argparse
import hashlib
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
FIXTURE = Path(__file__).with_name("native_test.cpp")
COMMON_CASES = ("disabled", "identity", "receipt", "native_boundary", "pause_transition", "consume_reset", "heights")


def function(source, name):
    match = re.search(r"^[^\n;{}]*\b" + re.escape(name) + r"\([^;{}]*\)\s*\{", source, re.M)
    if not match:
        raise RuntimeError("Missing production definition: " + name)
    index, depth = match.end(), 1
    while depth:
        depth += (source[index] == "{") - (source[index] == "}")
        index += 1
    return source[match.start():index]


def marked_function(path, name):
    source = path.read_text()
    body = function(source, name)
    line = source[:source.index(body)].count("\n") + 1
    return f'#line {line} "{path}"\n{body}\n'


def sources(host):
    game = ROOT / host
    adapter_path = game / ("2s2h" if host == "mm" else "soh") / "Enhancements/Graphics/ItemGetLightShaft.cpp"
    adapter = adapter_path.read_text()
    begin, end = adapter.index("namespace {"), adapter.index("void Register()")
    # Keeps the actual state declarations and every runtime adapter body; only
    # engine hook registration and graphics submission are outside this probe.
    adapter_body = adapter[begin:end] + "}\n"
    adapter_body += marked_function(adapter_path, ("MM" if host == "mm" else "SoH") + "ItemGetLightShaft_MarkNative")
    adult = ""
    paths = [adapter_path, game / "src/code/z_player_lib.c", game / "src/code/z_actor.c",
             ROOT / "combo/render/ItemGetLightShaft.h"]
    if host == "mm":
        adult_path = game / "mods/items/logic/adult_link_render.cpp"
        paths.append(adult_path)
        adult = "\n".join(marked_function(adult_path, name) for name in (
            "AdultLink_UsesAdultPresentation", "AdultLink_UsesAdultItemPresentation",
            "AdultLink_GetItemPresentationHeight", "AdultLink_GetItemPresentationYOffset"))
    cpp = FIXTURE.read_text().replace("/* GI_SHAFT_ADULT_QUERIES */", adult)
    cpp = cpp.replace("/* GI_SHAFT_NATIVE_ADAPTER */", adapter_body)
    reference = "sPlayerGetItemRefPos" if host == "mm" else "sGetItemRefPos"
    c = '#include "z64.h"\n#include "functions.h"\n#include "variables.h"\n#include "macros.h"\n'
    c += '#include "combo/render/ItemGetLightShaftBridge.h"\n'
    c += f"Vec3f {reference};\n"
    c += "void Player_DrawGetItemImpl(PlayState*, Player*, Vec3f*, s32);\n"
    if host == "soh":
        c += "f32 TransformMasks_GetFormHeight(void);\n"
    c += marked_function(game / "src/code/z_actor.c", "Player_GetHeight")
    if host == "mm":
        c += marked_function(game / "src/code/z_lib.c", "Math_Vec3f_Copy")
    c += marked_function(game / "src/code/z_player_lib.c", "Player_DrawGetItem")
    return cpp, c, paths


def compile_probe(host, cpp, c, directory, sanitize):
    directory.mkdir(parents=True, exist_ok=True)
    cpp_path, c_path = directory / "native.cpp", directory / "native.c"
    cpp_path.write_text(cpp)
    c_path.write_text(c)
    include_dirs = ("", host, host + "/include", host + "/include/PR", host + "/src", host + "/assets",
                    host + "/mods", host + ("/2s2h" if host == "mm" else "/soh"),
                    "libultraship/include", "combo", "combo/menu")
    flags = ["-DCOMBO_BUILD", "-DF3DEX_GBI_2", "-DGBI_FLOAT_MTX=1", "-DGBI_S32_VTX=1",
             "-DCONTROLLERBUTTONS_T=uint32_t", "-DLOG_LEVEL_GAME_PRINTS=0", "-g",
             *["-I" + str(ROOT / path) for path in include_dirs]]
    if host == "mm":
        flags += ["-DGI_SHAFT_TEST_MM"]
    if sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie"]
    cc = shlex.split(os.environ.get("CC", "gcc-14"))
    cxx = shlex.split(os.environ.get("CXX", "g++-14"))
    obj, binary = directory / "native.o", directory / "native-test"
    subprocess.run([*cc, "-std=gnu2x", *flags, "-w", "-c", str(c_path), "-o", str(obj)], check=True, cwd=ROOT)
    subprocess.run([*cxx, "-std=c++20", *flags, "-Wno-narrowing", "-Wno-write-strings",
                    str(cpp_path), str(obj), "-no-pie", "-lm", "-o", str(binary)], check=True, cwd=ROOT)
    return binary


def replace_once(source, old, new):
    if source.count(old) != 1:
        raise RuntimeError(f"Mutation boundary changed: expected one occurrence of {old!r}")
    return source.replace(old, new, 1)


def mutations(host, cpp, c):
    yield "enabled-fallback", "disabled", cpp.replace(
        "CVarGetInteger(ItemGetLightShaft::kCVar, 0)", "CVarGetInteger(ItemGetLightShaft::kCVar, 1)"), c
    yield "peer-guard", "identity", replace_once(cpp, "player == GET_PLAYER(play) &&", ""), c
    prefix = "MM" if host == "mm" else "SoH"
    broken_c, count = re.subn(r"^\s*" + prefix + r"ItemGetLightShaft_MarkNative\([^;]+;", "", c, flags=re.M)
    if count != 1:
        raise RuntimeError("Native marker call-site mutation did not identify one statement")
    yield "native-callsite", "native_boundary", cpp, broken_c
    if host == "mm":
        yield "early-custom", "custom", replace_once(cpp, "reinterpret_cast<EnItem00*>(actor)->unk152 < 20",
                                                     "reinterpret_cast<EnItem00*>(actor)->unk152 < 19"), c


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", choices=("both", "soh", "mm"), default="both")
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--mutation-check", action="store_true")
    args = parser.parse_args()
    hosts = ("soh", "mm") if args.host == "both" else (args.host,)
    environment = {**os.environ, "ASAN_OPTIONS": "detect_leaks=0"}
    total = killed = 0
    print("Native adapter probe: production bodies + native structs/C wrappers; no forced PCH, hooks, or Gfx execution.",
          flush=True)
    print("SoH preserves its existing commented-out GI queue wait; MM tests the actual async ready gate.", flush=True)
    with tempfile.TemporaryDirectory(prefix="gi-shaft-native-") as temporary:
        root = Path(temporary)
        for host in hosts:
            cpp, c, paths = sources(host)
            for path in paths:
                print(f"SOURCE {path.relative_to(ROOT)} sha256={hashlib.sha256(path.read_bytes()).hexdigest()}", flush=True)
            binary = compile_probe(host, cpp, c, root / host, args.sanitize)
            cases = COMMON_CASES + (("custom", "custom_scope") if host == "mm" else ())
            for case in cases:
                subprocess.run([str(binary), case], check=True, env=environment, timeout=15)
                total += 1
            if args.mutation_check:
                for name, case, broken_cpp, broken_c in mutations(host, cpp, c):
                    mutant = compile_probe(host, broken_cpp, broken_c, root / (host + "-" + name), args.sanitize)
                    result = subprocess.run([str(mutant), case], capture_output=True, text=True, env=environment, timeout=15)
                    output = result.stdout + result.stderr
                    if result.returncode != 1 or "FAIL [" + host + "][" + case + "]" not in output:
                        raise RuntimeError(f"Mutation {host}/{name} did not fail at its intended assertion:\n{output}")
                    print(f"PASS mutation {host}/{name}: {output.strip()}", flush=True)
                    killed += 1
    print(f"PASS native adapter cases: {total}; expected source mutations rejected: {killed}", flush=True)


if __name__ == "__main__":
    main()
