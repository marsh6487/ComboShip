#!/usr/bin/env python3
"""Run real linked Solo MM entrance generation and independent native replay.

The SoH module initializes the shared native Context; all tested generation,
inventory, entrances and graph traversal belong to the supplied MM module.
Each profile uses a fresh process/config directory. No surrogate graph is used.
"""

import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/shuffle_parity/boss_keys"))
from native_boss_key_regression import Native, mm_replay

OPTIONS = {
    "temples": "SHUFFLE_DUNGEON_ENTRANCES",
    "expanded": "SHUFFLE_EXPANDED_DUNGEON_ENTRANCES",
    "bosses": "SHUFFLE_BOSS_ENTRANCES",
    "interiors": "SHUFFLE_INTERIOR_ENTRANCES",
    "shops": "SHUFFLE_SHOP_ENTRANCES",
    "door-interiors": "SHUFFLE_DOOR_INTERIORS",
    "grottos": "SHUFFLE_GROTTO_ENTRANCES",
    "overworld": "SHUFFLE_OVERWORLD_ENTRANCES",
}
POLICIES = {
    "decoupled": "DECOUPLED_ENTRANCES", "mixed": "MIX_ENTRANCE_POOLS",
    "mixDungeons": "MIX_DUNGEON_ENTRANCES", "mixBosses": "MIX_BOSS_ENTRANCES",
    "mixInteriors": "MIX_INTERIOR_ENTRANCES", "mixShops": "MIX_SHOP_ENTRANCES",
    "mixGrottos": "MIX_GROTTO_ENTRANCES", "mixOverworld": "MIX_OVERWORLD_ENTRANCES",
}
MULTI_POOL_CASES = ("all", "decoupled", "mixed", "mixed-all", "mixed-decoupled")
CASES = ("off", *OPTIONS, *MULTI_POOL_CASES)


def check(condition, message):
    if not condition:
        raise AssertionError(message)


def write(path, value):
    path.write_text(json.dumps(value, indent=2) + "\n")


def configured(case):
    settings = {
        "gRando.Options.RO_LOGIC": 0,
        "gRando.Options.RO_PLACEMENT_BOSS_KEYS": 1,
        "gRando.Options.RO_PLACEMENT_SMALL_KEYS": 1,
        "gRando.ExcludedChecks": [],
        "gRando.StartingItems": [],
    }
    for name, option in OPTIONS.items():
        settings["gRando.Options.RO_" + option] = int(case in MULTI_POOL_CASES or name == case)
    if case == "expanded":
        settings["gRando.Options.RO_SHUFFLE_DUNGEON_ENTRANCES"] = 1
    for name, option in POLICIES.items():
        if name == "decoupled":
            enabled = case in ("decoupled", "mixed-decoupled")
        elif case == "mixed":
            # The user's selected example mixes dungeons, bosses and grottos;
            # all other enabled categories must remain in their separate pools.
            enabled = name in ("mixed", "mixDungeons", "mixBosses", "mixGrottos")
        else:
            enabled = case in ("mixed-all", "mixed-decoupled")
        settings["gRando.Options.RO_" + option] = int(enabled)
    return settings


def validate_map(case, seed):
    if case == "off":
        check(seed is None or seed.get("enabled") is False, "disabled controls generated entrance routes")
        return
    check(isinstance(seed, dict) and seed.get("enabled") is True,
          "Solo MM generation did not export its enabled entrance layout")
    entries = seed["dungeons"]
    permutation = seed["permutation"]
    reverse = seed["reversePermutation"]
    policy = seed["policy"]
    check(sorted(permutation) == list(range(len(entries))), "layout is not a bijection")
    check(sorted(reverse) == list(range(len(entries))), "return layout is not a bijection")
    expected_count = {"temples": 5, "expanded": 12, "bosses": 4, "interiors": 35, "shops": 7,
                      "door-interiors": 24, "grottos": 26, "overworld": 40}.get(case)
    if case in MULTI_POOL_CASES:
        expected_count = 124
    if expected_count is not None:
        check(len(entries) == expected_count, f"{case} pool contains {len(entries)} portals, expected {expected_count}")
    check(2 <= len(entries) <= 256, "empty or oversized enabled entrance catalogue")
    settings = configured(case)
    option_keys = {"temples": "dungeons", "door-interiors": "doorInteriors"}
    expected_options = {option_keys.get(name, name): bool(settings["gRando.Options.RO_" + option])
                        for name, option in OPTIONS.items()}
    # The saved broad pool owns its door subset even when the disabled menu value is remembered.
    expected_options["doorInteriors"] &= not expected_options["interiors"]
    expected_options["expanded"] &= expected_options["dungeons"]
    check(seed["options"] == expected_options, "saved categories do not match the requested profile")
    for name, option in POLICIES.items():
        check(policy[name] == bool(settings["gRando.Options.RO_" + option]), "saved policy mismatch: " + name)
    check(any(index != destination for index, destination in enumerate(permutation)),
          "profile did not exercise a shuffled forward route; use a nontrivial native seed")
    inverse = [permutation.index(i) for i in range(len(entries))]
    if policy["decoupled"]:
        check(reverse != inverse, "decoupled profile has no independently shuffled returns")
    else:
        check(reverse == inverse, "coupled returns do not reverse the saved ingress")
    mix_key = {0: "mixDungeons", 1: "mixBosses", 2: "mixInteriors", 3: "mixGrottos",
               4: "mixShops", 5: "mixInteriors", 6: "mixOverworld"}
    def group(entry):
        pool = entry.get("pool", 0)
        return -1 if policy["mixed"] and policy[mix_key[pool]] else pool
    mixed_links = 0
    for index, entry in enumerate(entries):
        check(group(entry) == group(entries[permutation[index]]), "forward mapping crossed an unselected pool")
        check(group(entry) == group(entries[reverse[index]]), "return mapping crossed an unselected pool")
        mixed_links += entry.get("pool", 0) != entries[permutation[index]].get("pool", 0)
        endpoints = [entry[key] for key in ("entrance", "exit", "returnTo", "warpReturn", "reverseCheckpoint",
                                           "warpCheckpoint", "deathCheckpoint") if key in entry]
        endpoints += entry.get("escapeExits", [])
        check(all(endpoint["game"] == 1 for endpoint in endpoints), "Solo MM seed contains an OoT endpoint")
    if policy["mixed"]:
        check(mixed_links > 0, "mixed profile did not mix any selected categories")


def probe(case, seed_number, output, repeat):
    native = Native("mm", True)
    settings = configured(case)
    native.restore(settings)
    native.call("SetComboRandoSeed", seed_number, args=(ctypes.c_uint64,))
    data = native.dump("DumpRandoStaticData")
    check("error" not in data, "native item pool failed: " + repr(data.get("error")))
    filled = json.loads(native.call("FillSoloRandoSeed", seed_number, result=ctypes.c_char_p,
                                    args=(ctypes.c_uint32,)))
    write(output, {"case": case, "seed": seed_number, "nativeFill": filled})
    check(filled.get("success"), "native Solo MM fill failed: " + repr(filled.get("error")))
    for key, value in settings.items():
        check(filled["settings"].get(key) == value, "native setting was not preserved: " + key)
    mapping = filled.get("soloEntrances")
    validate_map(case, mapping)
    if case != "off":
        check(mapping["masterSeed"] == filled["finalSeed"], "layout belongs to another native seed")
    install = native.bind("MM_SetSoloEntranceSeed", result=ctypes.c_int, args=(ctypes.c_char_p,))
    check(install(json.dumps(mapping).encode()) == 1, "actual native oracle rejected generated mapping")
    replay = mm_replay(native, filled, data)
    collected = {name for sphere in replay["spheres"] for name in sphere}
    shuffled = {name for name, row in filled["checks"].items() if row["shuffled"]}
    expected_shuffled = {row["name"] for row in data["checks"]}
    expected_shuffled.update(row["check"] for row in data["fixed"] if row.get("hintable"))
    check(shuffled == expected_shuffled,
          "native fill changed the independently exported shuffled-check pool: missing=" +
          repr(sorted(expected_shuffled - shuffled)) + " unexpected=" + repr(sorted(shuffled - expected_shuffled)))
    check(expected_shuffled <= filled["placements"].keys(), "native fill omitted eligible check placements")
    check(not shuffled - collected,
          "independent native replay missed shuffled checks: " + repr(sorted(shuffled - collected)))
    replay["shuffledChecksVisited"] = len(shuffled)
    # A Solo oracle must ignore access metadata from the combined coordinator,
    # including foreign arrival roots that would otherwise grant early boss access.
    scenes = dict((name, int(value, 16)) for value, name in re.findall(
        r"/\*\s*(0x[0-9A-Fa-f]+)\s*\*/\s*(ENTR_SCENE_\w+)",
        (ROOT / "mm/include/z64scene.h").read_text()))
    boss_arrivals = [scenes["ENTR_SCENE_" + name] << 9 for name in
                     ("ODOLWAS_LAIR", "GOHTS_LAIR", "GYORGS_LAIR", "TWINMOLDS_LAIR")]
    access = native.bind("MM_SetCrossDungeonAccess", args=(ctypes.c_char_p,))
    set_mode = native.bind("MM_SetGameMode", args=(ctypes.c_int,))
    cross_install = native.bind("MM_SetCrossDungeonSeed", result=ctypes.c_int, args=(ctypes.c_char_p,))
    cross_options = {"dungeons": True, "bosses": True}
    cross_catalogue = json.loads(native.call("DumpCrossDungeonCatalogueForOptions",
        json.dumps(cross_options).encode(), result=ctypes.c_char_p, args=(ctypes.c_char_p,)))
    check(len(cross_catalogue) == 9, "native stale-map probe requires five temples and four bosses")
    stale_map = {"version": 4, "enabled": True, "options": cross_options, "dungeons": cross_catalogue,
                 "permutation": list(range(len(cross_catalogue)))}
    try:
        # Populate the real combined HostState before returning to Solo. With
        # empty routes, the oracle's legacy fallback would keep the root even
        # if its explicit Solo guard regressed, weakening the false-world probe.
        set_mode(0)
        check(cross_install(json.dumps(stale_map).encode()) == 1, "native stale combined map was rejected")
        set_mode(2)
        access(json.dumps({"overworld": False, "arrivals": boss_arrivals, "logicalClears": 0xfff}).encode())
        contaminated = mm_replay(native, filled, data)
        check(contaminated["spheres"] == replay["spheres"], "stale combined state contaminated Solo reachability")
    finally:
        set_mode(0)
        cleared = cross_install(b"null")
        set_mode(2)
        check(cleared == 1, "native stale combined map did not clear")
    replay["ignoresCombinedAccess"] = True
    replay["staleCombinedPortals"] = len(cross_catalogue)
    malformed = []
    if case != "off":
        bad = json.loads(json.dumps(mapping))
        bad["permutation"][0] = bad["permutation"][1]
        check(install(json.dumps(bad).encode()) == 0, "native oracle accepted duplicate entrance destinations")
        malformed.append("duplicate destination")
        bad = json.loads(json.dumps(mapping))
        bad["reversePermutation"][0] = bad["reversePermutation"][1]
        check(install(json.dumps(bad).encode()) == 0, "native oracle accepted duplicate return destinations")
        malformed.append("duplicate return destination")
        bad = json.loads(json.dumps(mapping))
        bad["dungeons"][0]["entrance"]["game"] = 0
        check(install(json.dumps(bad).encode()) == 0, "native oracle accepted foreign-game source")
        malformed.append("foreign endpoint")
        check(install(json.dumps(mapping).encode()) == 1, "valid mapping did not reinstall after rejected input")
    if repeat:
        repeated = json.loads(native.call("FillSoloRandoSeed", seed_number, result=ctypes.c_char_p,
                                         args=(ctypes.c_uint32,)))
        check(repeated.get("success"), "repeat native fill failed: " + repr(repeated.get("error")))
        for key in ("soloEntrances", "placements", "prices"):
            check(repeated.get(key) == filled.get(key), "same seed/settings changed " + key)
    write(output, {"case": case, "seed": seed_number, "settings": settings, "nativeFill": filled,
                   "nativeReplay": replay, "malformedRejected": malformed, "deterministicRepeat": repeat})
    print(f"{case}: native fill, saved layout and native replay PASS ({replay['reachableChecks']} checks)", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime-dir", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--case", action="append", choices=CASES)
    parser.add_argument("--seed", type=int, default=20261110)
    parser.add_argument("--repeat", action="store_true")
    # Repeating a native fill can use two complete ten-by-ten-second budgets.
    parser.add_argument("--timeout", type=int, default=300)
    parser.add_argument("--probe", choices=CASES, help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.probe:
        probe(args.probe, args.seed, args.output, args.repeat)
        return
    check(args.runtime_dir is not None, "--runtime-dir is required")
    runtime = args.runtime_dir.resolve()
    out = args.output.resolve()
    check(not out.exists(), "output must be fresh to retain earlier evidence")
    out.mkdir(parents=True)
    modules = [path for path in runtime.iterdir() if path.is_file() and
               (".so" in path.name or path.suffix in (".dll", ".dylib"))]
    check(any("2ship" in path.name for path in modules), "native MM module missing")
    requested_cases = args.case or list(CASES)
    manifest = {"seed": args.seed, "source": str(ROOT), "status": "running", "cases": [],
                "requestedCases": requested_cases, "deterministicRepeat": args.repeat,
                "moduleSha256": {path.name: hashlib.sha256(path.read_bytes()).hexdigest() for path in modules}}
    write(out / "manifest.json", manifest)
    for case in requested_cases:
        folder = out / case
        folder.mkdir()
        work = folder / "runtime"
        work.mkdir()
        for path in modules:
            (work / path.name).symlink_to(path)
        result_file = folder / "result.json"
        command = [sys.executable, str(Path(__file__).resolve()), "--probe", case, "--seed", str(args.seed),
                   "--output", str(result_file)]
        if args.repeat:
            command.append("--repeat")
        env = dict(os.environ, SHIP_HOME=str(work))
        case_result = {"case": case, "status": "running", "result": str(result_file)}
        manifest["cases"].append(case_result)
        write(out / "manifest.json", manifest)
        try:
            with (folder / "native.log").open("w") as log:
                result = subprocess.run(command, cwd=work, env=env, stdout=log, stderr=subprocess.STDOUT,
                                        timeout=args.timeout)
        except subprocess.TimeoutExpired:
            case_result.update(status="timeout", timeoutSeconds=args.timeout)
            manifest["status"] = "failed"
            write(out / "manifest.json", manifest)
            raise
        except Exception as error:
            case_result.update(status="error", error=str(error))
            manifest["status"] = "failed"
            write(out / "manifest.json", manifest)
            raise
        case_result.update(status="passed" if result.returncode == 0 else "failed", exitCode=result.returncode)
        if result.returncode:
            manifest["status"] = "failed"
        write(out / "manifest.json", manifest)
        check(result.returncode == 0, f"{case} failed; see {folder / 'native.log'}")
        print(f"{case} PASS", flush=True)
    manifest["status"] = "passed"
    write(out / "manifest.json", manifest)


if __name__ == "__main__":
    main()
