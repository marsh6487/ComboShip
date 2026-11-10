#!/usr/bin/env python3
"""Run isolated native combined-fill cases against public exact-build CI fixtures.

Each configuration runs in a fresh process and writable directory without an
existing config. SHIP_HOME points to that directory. Only libraries and assets
are linked from --runtime-dir; its configuration, logs and saves are never used.
Native array-setting restoration must pass before cases run. --driver must
support --fill-only.
The two public fixture snapshots contain no uploaded user settings or game logs.

PASS means native fill plus its empty-inventory validation completed. Hints,
consolidated spoiler writing, independent replay and in-game play are separate
gates. No Logic cases retain their stated weaker accessibility contract.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import sys
import time


CONFIG_DIR = Path(__file__).resolve().with_name("configs")
DEFAULT_SEEDS = (20261109, 20261110)
NATIVE_TRIO = ("bows", "magic", "lightArrows")
SHARED_SOULS = ("sharedSoulsEnemy", "sharedSoulsNpc", "sharedSoulsAnimal", "sharedSoulsMisc")
CROSS_FLAGS = ("dungeon", "boss", "expanded", "interior", "grotto", "shop", "door")
MM_ARRAY_KEYS = ("gRando.StartingItems", "gRando.ExcludedChecks")
SETTINGS_PREFLIGHT_TIMEOUT = 30
CROSS_CLI = {
    "dungeon": "--cross-dungeon-entrances", "boss": "--cross-boss-entrances",
    "expanded": "--expanded-dungeon-pool", "interior": "--cross-interior-entrances",
    "grotto": "--cross-grotto-entrances", "shop": "--cross-shop-entrances",
    "door": "--cross-door-interior-entrances",
}


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def patches(oot=None, mm=None):
    return {"oot": {"gRandoSettings." + key: value for key, value in (oot or {}).items()},
            "mm": {"gRando.Options.RO_" + key: value for key, value in (mm or {}).items()}}


def merged(*values):
    result = {"oot": {}, "mm": {}}
    for value in values:
        for game in result:
            result[game].update(value.get(game, {}))
    return result


def make_cases():
    """A bounded interaction matrix, not the Cartesian product of every setting."""
    cases = []
    mixed = patches({"NoLogic": 1, "AllChecksReachable": 0})
    nei = patches({"SkijerCustomItems": 1, "NeiWeaponUpgrades": 1}, {"SHUFFLE_NEI_ITEMS": 1})
    # The all-on CI fixture deliberately provides real optional check capacity.
    # Apply the same capacity to individual large item-family probes explicitly.
    capacity = patches({"ShuffleTokens": 3, "ShufflePots": 3, "ShuffleFreestanding": 3},
                       {"SHUFFLE_" + key + "_DROPS": 1
                        for key in ("ICICLE", "ROCK", "BOULDER", "RED_BOULDER", "SOIL")})
    rings = patches(mm={"PLACEMENT_SMALL_KEYS": 1, "SHUFFLE_KEY_RINGS": 15})
    rusty = patches({"LockOverworldDoors": 1, "RustyDoorKeyCatalogVersion": 1},
                    {"SHUFFLE_RUSTY_KEYS": 1})
    enemy_boss = patches({"ShuffleEnemySouls": 1, "ShuffleBossSouls": 1},
                         {"SHUFFLE_ENEMY_SOULS": 1, "SHUFFLE_BOSS_SOULS": 1})
    new_souls = patches({"Shuffle" + kind + "Souls": 1 for kind in ("Npc", "Animal", "Misc")},
                        {"SHUFFLE_" + kind + "_SOULS": 1 for kind in ("NPC", "ANIMAL", "MISC")})
    events = patches({"ShuffleClock": 1, "ClockStart": 0, "ShuffleSongEvents": 1},
                     {"CLOCK_SHUFFLE": 1, "SHUFFLE_SONG_EVENTS": 1})
    native_entrances = patches({"ShuffleDungeonsEntrances": 1}, {"SHUFFLE_DUNGEON_ENTRANCES": 1})

    def add(name, description, delta=None, shared=(), start="oot", cross=(), fixture="all-off",
            tags=(), logic_scope="both games use native logic"):
        cases.append({"name": name, "description": description, "fixture": fixture + ".json",
                      "patches": delta or patches(), "sharedItems": list(shared), "startingGame": start,
                      "crossEntrances": {key: int(key in cross) for key in CROSS_FLAGS},
                      "tags": list(tags), "expected": "native fill PASS", "logicScope": logic_scope})

    def mix(name, description, delta=None, shared=NATIVE_TRIO, **kwargs):
        add(name, description, merged(mixed, delta or patches()), shared=shared,
            logic_scope="OoT No Logic; MM progression still requires native reachability", **kwargs)

    add("ci-all-off", "Public CI all-off: both logic engines, native own-dungeon boss keys",
        tags=("before-new-shuffles", "ci-fixture", "all-reachable", "own-boss-keys"))
    mix("mixed-unshared", "OoT No Logic with all native and NEI sharing disabled", shared=(),
        tags=("mixed-logic", "no-sharing", "own-boss-keys"))
    for key in NATIVE_TRIO:
        mix("mixed-" + key.lower(), "Only native " + key + " sharing with mixed logic", shared=(key,),
            tags=("mixed-logic", "individual-native-sharing", "own-boss-keys"))
    mix("mixed-native-trio", "Shared Bow, Magic and Light Arrows; NEI disabled",
        tags=("mixed-logic", "native-sharing", "own-boss-keys"))
    mix("mixed-nei", "NEI sharing only; native shared families disabled", nei, shared=(),
        tags=("mixed-logic", "nei-sharing", "own-boss-keys"))
    mix("mixed-native-nei", "Native trio and NEI aliases share the same progression pass", nei,
        tags=("mixed-logic", "native-sharing", "nei-sharing", "own-boss-keys"))
    add("beatable-native-nei", "OoT beatable-only with native and NEI sharing",
        merged(patches({"NoLogic": 0, "AllChecksReachable": 0}), nei), shared=NATIVE_TRIO,
        tags=("beatable-only", "native-sharing", "nei-sharing"))
    add("allreachable-native-nei", "OoT all-reachable with native and NEI sharing", nei, shared=NATIVE_TRIO,
        tags=("all-reachable", "native-sharing", "nei-sharing"))
    mix("mixed-mm-start", "Mixed logic and shared progression with an explicit MM start", nei, start="mm",
        tags=("mixed-logic", "mm-start", "native-sharing", "nei-sharing"))
    mix("mixed-random-start", "Random start, including the driver's documented retry fallback", nei, start="random",
        tags=("mixed-logic", "random-start", "native-sharing", "nei-sharing"))
    add("both-no-logic", "Both games explicitly request No Logic; never label this a logic proof",
        merged(mixed, patches(mm={"LOGIC": 1})), shared=NATIVE_TRIO,
        tags=("both-no-logic", "limited-proof"),
        logic_scope="both native oracles use No Logic; fill/inventory invariants only")
    for label, value in (("anywhere", 0), ("startwith", 2), ("vanilla", 3)):
        mix("boss-" + label, "MM boss keys " + label + " with shared native progression",
            patches(mm={"PLACEMENT_BOSS_KEYS": value}), tags=("mixed-logic", "boss-keys-" + label))
    mix("own-small-key-rings", "All four MM key rings with native own-dungeon small keys", rings,
        tags=("mixed-logic", "own-small-keys", "key-rings", "own-boss-keys"))
    mix("native-entrances", "Native OoT dungeon entrances; the Solo MM entrance toggle stays inactive in Combo", native_entrances,
        tags=("mixed-logic", "native-entrances", "native-oot-entrances", "solo-mm-toggle-inactive", "own-boss-keys"))
    mix("cross-dungeon-boss", "Cross-game dungeon and boss pools without expanded dungeons",
        cross=("dungeon", "boss"), start="mm", tags=("mixed-logic", "cross-dungeon", "cross-boss", "mm-start"))
    mix("cross-expanded-mixed", "Expanded dungeon, boss, grotto and shop pools with native and NEI sharing", nei,
        cross=("dungeon", "boss", "expanded", "grotto", "shop"), start="random",
        tags=("mixed-logic", "expanded-dungeons", "cross-grotto", "cross-shop", "random-start", "nei-sharing"))
    mix("cross-door-interiors", "Door and shop cross-game topology from the CI smoke settings",
        cross=("shop", "door"), tags=("mixed-logic", "cross-door", "cross-shop", "ci-topology"))
    mix("rusty-keys", "Both rusty-key systems plus own-dungeon MM key rings; explicit extra checks",
        merged(capacity, rusty, rings), tags=("mixed-logic", "rusty-keys", "key-rings", "extra-check-capacity"))
    mix("enemy-boss-souls", "Both enemy/boss soul families; shared enemy souls; explicit extra checks",
        merged(capacity, enemy_boss), shared=NATIVE_TRIO + ("sharedSoulsEnemy",),
        tags=("mixed-logic", "enemy-souls", "boss-souls", "shared-souls", "extra-check-capacity"))
    mix("npc-animal-misc-souls", "New NPC, animal and misc soul families in both games; explicit extra checks",
        merged(capacity, new_souls), shared=NATIVE_TRIO + SHARED_SOULS[1:],
        tags=("mixed-logic", "npc-souls", "animal-souls", "misc-souls", "shared-souls", "extra-check-capacity"))
    mix("clock-song-events", "Both clocks and song-event shuffles with shared progression", merged(capacity, events),
        tags=("mixed-logic", "clock-shuffle", "song-events", "extra-check-capacity"))
    add("all-parity-unshared", "Public CI all-parity with sharing disabled and MM clock enabled",
        patches(mm={"CLOCK_SHUFFLE": 1}), fixture="all-parity",
        tags=("all-reachable", "all-new-shuffles", "no-sharing", "extra-check-capacity", "ci-fixture"))
    mix("all-parity-shared-mixed", "All parity families plus native/NEI sharing and expanded cross-game pools",
        merged(nei, capacity, events), fixture="all-parity", shared=NATIVE_TRIO + SHARED_SOULS,
        start="random", cross=("dungeon", "boss", "expanded", "grotto", "shop"),
        tags=("mixed-logic", "all-new-shuffles", "shared-souls", "nei-sharing", "expanded-dungeons",
              "extra-check-capacity", "random-start"))
    add("all-parity-native-allreachable", "All parity families and shared native items with native OoT dungeon entrances",
        merged(native_entrances, events), fixture="all-parity", shared=NATIVE_TRIO + SHARED_SOULS, start="mm",
        tags=("all-reachable", "all-new-shuffles", "native-entrances", "native-oot-entrances", "solo-mm-toggle-inactive",
              "shared-souls", "mm-start", "ci-fixture"))
    add("native-cross-allreachable", "Native OoT dungeon entrances followed by cross-game dungeon and boss pools",
        native_entrances, shared=NATIVE_TRIO, start="mm", cross=("dungeon", "boss"),
        tags=("all-reachable", "native-oot-entrances", "cross-dungeon", "cross-boss", "mm-start"))
    mix("native-cross-mixed", "Native OoT entrances with expanded cross-game pools and native/NEI sharing",
        merged(native_entrances, nei), start="random", cross=("dungeon", "boss", "expanded", "grotto", "shop"),
        tags=("mixed-logic", "native-oot-entrances", "expanded-dungeons", "cross-boss", "nei-sharing", "random-start"))
    for label, value in (("vanilla", 0), ("anywhere", 5)):
        add("native-rewards-" + label, "Native OoT dungeon entrances with dungeon rewards " + label,
            merged(native_entrances, patches({"ShuffleDungeonReward": value})), shared=NATIVE_TRIO,
            tags=("all-reachable", "native-oot-entrances", "reward-mode-" + label))
    return cases


def classify_run(exit_code, timed_out, log):
    if timed_out:
        return "TIMEOUT", "native process exceeded the per-run timeout; no passing verdict"
    if exit_code != 0 and "[comborando] failed to load" in log:
        return "BLOCKED", "native modules or their runtime dependencies could not be loaded"
    if exit_code != 0:
        return "FAIL", "native process exited with code " + str(exit_code)
    summary = re.findall(r"^\[comborando\] RESULT: (PASS|FAIL).*?(\d+)/(\d+) completable,\s*\d+ ms\s*$",
                         log, re.MULTILINE)
    if summary != [("PASS", "1", "1")]:
        return "FAIL", "missing or inconsistent final native 1/1 PASS summary"
    if len(re.findall(r"^\[comborando\]\s+seed .*?\(\d+\) PASS \(", log, re.MULTILINE)) != 1:
        return "FAIL", "expected exactly one completed seed PASS"
    if "fill-only: native fill validation enabled; hints and spoiler output skipped" not in log:
        return "FAIL", "driver did not acknowledge --fill-only"
    if not re.search(r"all \d+ advancement items reachable from scratch \(pass \d+, \d+ ms;", log):
        return "FAIL", "native empty-inventory final validation success was not recorded"
    return "PASS", "native fill and its empty-inventory validation completed"


def load_settings(case):
    result = json.loads((CONFIG_DIR / case["fixture"]).read_text(encoding="utf-8"))
    for game in ("oot", "mm"):
        result[game]["settings"].update(case["patches"][game])
    result["sharedItems"] = case["sharedItems"]
    return result


def prepare_work(runtime, driver, work, library_dirs):
    work.mkdir(parents=True, exist_ok=False)
    # Never link runtime configuration/save/log folders. Generator writes remain local.
    for source in runtime.iterdir():
        name = source.name
        is_library = source.is_file() and (name.endswith((".dll", ".dylib")) or ".so" in name)
        is_archive = source.is_file() and name.endswith((".o2r", ".otr"))
        is_resource_dir = source.is_dir() and name in ("assets", "lib", "lib64")
        if is_library or is_archive or is_resource_dir:
            (work / name).symlink_to(source.resolve(), target_is_directory=is_resource_dir)
    executable = work / ("comborando.exe" if os.name == "nt" else "comborando")
    # A local executable also keeps executable-relative config discovery isolated.
    shutil.copy2(driver, executable)
    # Native first-run restore creates its own config. An artificial {CVars:{}}
    # flattens to CVars:null, blocking SetBlock from restoring MM array settings.
    env = os.environ.copy()
    for name in ("APPIMAGE", "APPDIR", "LD_PRELOAD"):
        env.pop(name, None)
    env["SHIP_HOME"] = str(work)
    for key in ("LD_LIBRARY_PATH", "DYLD_LIBRARY_PATH"):
        paths = [str(path) for path in (work, work / "lib", work / "lib64", *library_dirs)]
        if env.get(key):
            paths.append(env[key])
        env[key] = os.pathsep.join(paths)
    return executable, env


def work_provenance(work, env):
    config = work / "comboship.json"
    initial = {"present": config.exists()}
    if config.exists():
        initial["sha256"] = sha256(config)
    return {"cwd": str(work), "SHIP_HOME": env.get("SHIP_HOME"),
            "clearedEnvironment": ["APPIMAGE", "APPDIR", "LD_PRELOAD"],
            "libraryPath": env["LD_LIBRARY_PATH"], "initialConfig": initial}


def settings_preflight_groups(cases):
    groups = {}
    for case in cases:
        settings = load_settings(case)
        arrays = {key: settings["mm"]["settings"].get(key) for key in MM_ARRAY_KEYS}
        key = json.dumps(arrays, sort_keys=True)
        if key not in groups:
            groups[key] = {"name": "arrays-" + str(len(groups) + 1), "requestedArrays": arrays,
                           "representativeSettings": settings, "cases": [], "fixtures": []}
        groups[key]["cases"].append(case["name"])
        if case["fixture"] not in groups[key]["fixtures"]:
            groups[key]["fixtures"].append(case["fixture"])
    return list(groups.values())


def run_settings_preflight(groups, runtime, driver, output, library_dirs, input_hashes):
    """Run each distinct requested array pair through the actual native exports."""
    worker = Path(__file__).resolve().with_name("native_settings_restore.py")
    results = []
    for group in groups:
        folder = output / "settings-preflight" / group["name"]
        started = time.monotonic()
        result = {key: group[key] for key in ("name", "cases", "fixtures", "requestedArrays")}
        result.update(status="BLOCKED", exitCode=None, timedOut=False, inputSha256=input_hashes,
                      workerSha256=sha256(worker))
        try:
            folder.mkdir(parents=True, exist_ok=False)
            settings_path = folder / "settings.json"
            write_json(settings_path, group["representativeSettings"])
            result["settingsSha256"] = sha256(settings_path)
            work = folder / "runtime"
            _, env = prepare_work(runtime, driver, work, library_dirs)
            proof_path = folder / "native-result.json"
            command = [sys.executable, "-B", str(worker), "--runtime", str(work), "--settings", str(settings_path),
                       "--output", str(proof_path)]
            for path in library_dirs:
                command += ["--library-dir", str(path)]
            write_json(folder / "command.json", {"argv": command, "timeoutSeconds": SETTINGS_PREFLIGHT_TIMEOUT,
                                                 **work_provenance(work, env)})
            result["status"] = "RUNNING"
            write_json(folder / "result.json", result)
            with (folder / "stdout.log").open("wb") as stdout, (folder / "stderr.log").open("wb") as stderr:
                process = subprocess.Popen(command, cwd=work, env=env, stdout=stdout, stderr=stderr,
                                           start_new_session=(os.name == "posix"))
                try:
                    result["exitCode"] = process.wait(timeout=SETTINGS_PREFLIGHT_TIMEOUT)
                except subprocess.TimeoutExpired:
                    result["timedOut"] = True
                    if os.name == "posix":
                        try:
                            os.killpg(process.pid, signal.SIGKILL)
                        except ProcessLookupError:
                            pass
                    else:
                        process.kill()
                    result["exitCode"] = process.wait()
            proof = json.loads(proof_path.read_text(encoding="utf-8")) if proof_path.is_file() else {}
            result["nativeResult"] = proof
            if result["timedOut"]:
                result.update(status="TIMEOUT", reason="Native settings restoration exceeded its bounded timeout")
            elif result["exitCode"] == 0 and proof.get("status") == "PASS" and not proof.get("mismatches") and \
                    proof.get("requestedArrays") == group["requestedArrays"]:
                result.update(status="PASS", reason="Native MM starting items and excluded checks match their request")
            else:
                result.update(status="FAIL", reason=proof.get("reason", "Native settings restoration did not pass"))
        except Exception as error:
            result.update(status="BLOCKED", reason=type(error).__name__ + ": " + str(error))
        result["evidenceSha256"] = {name: sha256(folder / name)
                                     for name in ("stdout.log", "stderr.log", "native-result.json", "restored-settings.json")
                                     if (folder / name).is_file()}
        result["elapsedSeconds"] = round(time.monotonic() - started, 3)
        if folder.is_dir():
            write_json(folder / "result.json", result)
        results.append(result)
        print("SETTINGS PREFLIGHT " + group["name"] + " " + result["status"], flush=True)
        write_json(output / "settings-preflight.json", {"status": "RUNNING", "results": results})
    summary = {"status": "PASS" if results and all(row["status"] == "PASS" for row in results) else "FAIL",
               "distinctArraySets": len(groups), "results": results}
    write_json(output / "settings-preflight.json", summary)
    return summary


def run_case(case, seed, runtime, driver, output, timeout, library_dirs):
    folder = output / case["name"] / str(seed)
    started = time.monotonic()
    result = {"case": case["name"], "baseSeed": seed, "status": "BLOCKED", "exitCode": None,
              "timedOut": False, "logicScope": case["logicScope"], "expected": case["expected"],
              "settingsSha256": None, "command": [], "elapsedSeconds": 0.0}
    try:
        folder.mkdir(parents=True, exist_ok=False)
        settings_path = folder / "settings.json"
        write_json(settings_path, load_settings(case))
        result["settingsSha256"] = sha256(settings_path)
        work = folder / "runtime"
        executable, env = prepare_work(runtime, driver, work, library_dirs)
        command = [str(executable), "--fill-only", "--settings", str(settings_path), "--master-seed", str(seed),
                   "--count", "1", "--starting-game", case["startingGame"], "--shared-items",
                   ",".join(case["sharedItems"]) if case["sharedItems"] else "none", "--triforce-required", "0"]
        for key, flag in CROSS_CLI.items():
            command += [flag, str(case["crossEntrances"][key])]
        result["command"] = command
        write_json(folder / "command.json", {"argv": command, "timeoutSeconds": timeout,
                                             **work_provenance(work, env)})
        result["status"] = "RUNNING"
        write_json(folder / "result.json", result)
        log_path = folder / "native.log"
        with log_path.open("wb") as log_stream:
            process = subprocess.Popen(command, cwd=work, env=env, stdout=log_stream, stderr=subprocess.STDOUT,
                                       start_new_session=(os.name == "posix"))
            try:
                result["exitCode"] = process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                result["timedOut"] = True
                if os.name == "posix":
                    try:
                        os.killpg(process.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                else:
                    process.kill()
                result["exitCode"] = process.wait()
        log = log_path.read_text(encoding="utf-8", errors="replace")
        result["status"], result["reason"] = classify_run(result["exitCode"], result["timedOut"], log)
        result["resolvedSeeds"] = [int(value) for value in re.findall(
            r"^\[comborando\]\s+seed .*?\((\d+)\) PASS \(", log, re.MULTILINE)]
        result["validation"] = re.findall(r"^.*all \d+ advancement items reachable from scratch.*$", log, re.MULTILINE)
        result["nativeSummary"] = re.findall(r"^\[comborando\] RESULT:.*$", log, re.MULTILINE)
        error_lines = [line[:1200] for line in log.splitlines()
                       if re.search(r"dead end|failed|FAIL|rejected|VIOLATED|imbalance|no remaining|exhausted", line, re.I)]
        result["errorLines"] = error_lines[-30:]
        result["lastLogLines"] = [line[:1200] for line in log.splitlines()[-12:]]
    except Exception as error:
        result["status"] = "BLOCKED"
        result["reason"] = type(error).__name__ + ": " + str(error)
    result["elapsedSeconds"] = round(time.monotonic() - started, 3)
    if folder.is_dir():
        write_json(folder / "result.json", result)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--runtime-dir", type=Path, help="Existing native modules/assets; read only")
    parser.add_argument("--driver", type=Path, help="Candidate comborando with --fill-only (defaults to runtime/comborando)")
    parser.add_argument("--library-dir", action="append", type=Path, default=[],
                        help="Additional dependency directory, e.g. AppImage usr/lib; repeat as needed")
    parser.add_argument("--output", type=Path, help="New directory for coverage manifest, logs and results")
    parser.add_argument("--case", action="append", default=[], help="Case name; repeat to select a subset")
    parser.add_argument("--seed", action="append", type=int, help="Master seed; repeat (default: 20261109, 20261110)")
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--timeout", type=float, default=300, help="Seconds per case and seed; timeout is never PASS")
    parser.add_argument("--list", action="store_true", help="List the bounded public matrix without native execution")
    parser.add_argument("--self-test", action="store_true", help="Check harness verdict rules without native execution")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    cases = make_cases()
    if len(cases) != 32 or len({case["name"] for case in cases}) != len(cases):
        raise RuntimeError("matrix must contain exactly 32 uniquely named configurations")
    unknown = set(args.case) - {case["name"] for case in cases}
    if unknown:
        parser.error("unknown --case: " + ", ".join(sorted(unknown)))
    if args.case:
        cases = [case for case in cases if case["name"] in args.case]
    seeds = list(dict.fromkeys(args.seed or DEFAULT_SEEDS))
    if any(seed < 0 or seed > 0xFFFFFFFF for seed in seeds):
        parser.error("--seed must be an unsigned 32-bit integer")
    if args.list:
        for case in cases:
            print(case["name"] + "\t" + case["description"])
        print(str(len(cases)) + " configurations x " + str(len(seeds)) + " seeds = " + str(len(cases) * len(seeds)) + " native runs")
        return 0
    if not args.runtime_dir or not args.output:
        parser.error("--runtime-dir and --output are required unless --list or --self-test is used")
    if args.jobs < 1 or args.timeout <= 0:
        parser.error("--jobs and --timeout must be positive")
    runtime = args.runtime_dir.resolve()
    driver = (args.driver or runtime / ("comborando.exe" if os.name == "nt" else "comborando")).resolve()
    library_dirs = [path.resolve() for path in args.library_dir]
    if not runtime.is_dir() or not driver.is_file():
        parser.error("runtime directory or driver does not exist")
    if any(not path.is_dir() for path in library_dirs):
        parser.error("one or more --library-dir paths do not exist")
    required = ("soh.dll", "2ship.dll") if os.name == "nt" else ("libsoh.so", "lib2ship.so")
    if not all((runtime / name).is_file() for name in required):
        parser.error("runtime is missing one or both native game modules")
    output = args.output.resolve()
    if output.exists():
        parser.error("--output must be a new directory so stale evidence cannot produce a pass")
    output.mkdir(parents=True)
    input_hashes = {"driver": sha256(driver)}
    for name in required + (("libultraship.so",) if (runtime / "libultraship.so").is_file() else ()):
        input_hashes[name] = sha256(runtime / name)
    fixtures = {name: sha256(CONFIG_DIR / name) for name in sorted({case["fixture"] for case in cases})}
    preflight_groups = settings_preflight_groups(cases)
    planned = [(case, seed) for case in cases for seed in seeds]
    manifest = {"schemaVersion": 1, "createdUtc": datetime.now(timezone.utc).isoformat(),
                "runtimeDirectory": str(runtime), "driver": str(driver), "inputSha256": input_hashes,
                "libraryDirectories": [str(path) for path in library_dirs],
                "fixtureSha256": fixtures, "seeds": seeds, "jobs": args.jobs, "timeoutSeconds": args.timeout,
                "runCount": len(planned), "configurations": cases,
                "coverageTags": sorted({tag for case in cases for tag in case["tags"]}),
                "settingsPreflight": {"timeoutSeconds": SETTINGS_PREFLIGHT_TIMEOUT,
                                      "groups": [{key: group[key] for key in ("name", "cases", "fixtures", "requestedArrays")}
                                                 for group in preflight_groups]},
                "harnessSha256": {name: sha256(Path(__file__).resolve().with_name(name))
                                  for name in ("native_sweep.py", "native_settings_restore.py")},
                "passContract": "Native MM array restoration preflight PASS; then exit 0, exactly one 1/1 native PASS summary and native empty-inventory validation success for every case",
                "limits": ["Bounded settings interaction coverage, not exhaustive",
                           "Fill only: optional hints, output writing, independent replay and gameplay are separate",
                           "OoT No Logic permits OoT-only advancement off logic; MM remains constrained",
                           "Both-No-Logic case verifies native fill invariants, not logical beatability",
                           "No expected-invalid cases: every failure, timeout or blocked run remains nonpassing"],
                "fixtureProvenance": json.loads((CONFIG_DIR / "provenance.json").read_text(encoding="utf-8"))}
    write_json(output / "coverage-manifest.json", manifest)
    results = []
    sweep_started = time.monotonic()
    preflight = run_settings_preflight(preflight_groups, runtime, driver, output, library_dirs, input_hashes)
    if preflight["status"] != "PASS":
        counts = {status: 0 for status in ("PASS", "FAIL", "TIMEOUT", "BLOCKED")}
        write_json(output / "results.json", {"completed": 0, "planned": len(planned), "counts": counts,
                                             "results": [], "settingsPreflight": preflight["status"]})
        write_json(output / "summary.json", {"status": "FAIL", "stage": "settings-preflight",
                                             "settingsPreflight": preflight["status"],
                                             "elapsedSeconds": round(time.monotonic() - sweep_started, 3),
                                             "planned": len(planned), "completed": 0, "counts": counts,
                                             "reason": "Native settings restore preflight failed; no fill cases started"})
        print("SWEEP FAIL: native settings restore preflight failed; no fill cases started", flush=True)
        print("Evidence: " + str(output))
        return 1
    with ThreadPoolExecutor(max_workers=args.jobs) as executor:
        futures = [executor.submit(run_case, case, seed, runtime, driver, output, args.timeout, library_dirs)
                   for case, seed in planned]
        for future in as_completed(futures):
            result = future.result()
            results.append(result)
            print(f"[{len(results)}/{len(planned)}] {result['case']} seed={result['baseSeed']} "
                  f"{result['status']} {result['elapsedSeconds']:.1f}s", flush=True)
            ordered = sorted(results, key=lambda value: (value["case"], value["baseSeed"]))
            counts = {status: sum(value["status"] == status for value in results)
                      for status in ("PASS", "FAIL", "TIMEOUT", "BLOCKED")}
            write_json(output / "results.json", {"completed": len(results), "planned": len(planned),
                                                 "counts": counts, "results": ordered})
    elapsed = round(time.monotonic() - sweep_started, 3)
    success = (preflight["status"] == "PASS" and len(results) == len(planned) and
               all(result["status"] == "PASS" for result in results))
    summary = {"status": "PASS" if success else "FAIL", "elapsedSeconds": elapsed,
               "settingsPreflight": preflight["status"],
               "planned": len(planned), "completed": len(results), "counts": counts,
               "nonpassing": [{key: result.get(key) for key in ("case", "baseSeed", "status", "reason", "exitCode")}
                              for result in ordered if result["status"] != "PASS"]}
    write_json(output / "summary.json", summary)
    print("SWEEP " + summary["status"] + ": " + json.dumps(counts, sort_keys=True) + f"; {elapsed:.1f}s")
    print("Evidence: " + str(output))
    return 0 if success else 1


def self_test():
    """Protect the harness verdict: an incomplete or timed-out native run cannot pass."""
    complete = ("[comborando] fill-only: native fill validation enabled; hints and spoiler output skipped\n"
                "[ComboShip] CrossWorldCombinedFill: all 10 advancement items reachable from scratch "
                "(pass 1, 100 ms; 2 junk, 0 junk on oracle-unreachable checks [oot=0 mm=0])\n"
                "[comborando]   seed 'test' (20261109) PASS (3 cross-game placements)\n"
                "[comborando] RESULT: PASS — 1/1 completable, 101 ms\n")
    assert classify_run(0, False, complete)[0] == "PASS", "complete validated native run must pass"
    assert classify_run(1, False, complete)[0] == "FAIL", "exit failure overrides a stale PASS line"
    assert classify_run(-9, True, complete)[0] == "TIMEOUT", "timeout overrides a printed PASS line"
    assert classify_run(0, False, "")[0] == "FAIL", "exit zero without native evidence must fail"
    assert classify_run(0, False, complete.replace("1/1 completable", "0/1 completable"))[0] == "FAIL"
    assert classify_run(0, False, complete.replace("reachable from scratch", "validation skipped"))[0] == "FAIL"
    assert classify_run(0, False, complete.replace("fill-only:", "old-driver:"))[0] == "FAIL"
    assert classify_run(2, False, "[comborando] failed to load libsoh.so: missing dependency")[0] == "BLOCKED"
    print("PASS: native sweep verdict self-test (8 checks)")


if __name__ == "__main__":
    raise SystemExit(main())
