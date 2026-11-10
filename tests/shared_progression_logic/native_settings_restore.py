#!/usr/bin/env python3
"""Check the native MM array-setting restore contract in one isolated process.

The sweep prepares the directory and bounds this worker's execution. Loading,
initialization, restoration and snapshots use the actual native module exports.
"""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import sys


ARRAY_KEYS = ("gRando.StartingItems", "gRando.ExcludedChecks")


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def requested_arrays(settings):
    arrays = {}
    for key in ARRAY_KEYS:
        value = settings["mm"]["settings"].get(key)
        if not isinstance(value, list) or any(not isinstance(item, str) for item in value):
            raise ValueError("Native MM settings require a string array for " + key)
        arrays[key] = value
    return arrays


def bind(module, name, result=None, arguments=()):
    function = getattr(module, name)
    function.restype = result
    function.argtypes = list(arguments)
    return function


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime", required=True, type=Path)
    parser.add_argument("--settings", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--library-dir", action="append", default=[], type=Path)
    args = parser.parse_args()
    result = {"status": "ERROR", "settingsSha256": sha256(args.settings)}
    try:
        settings = json.loads(args.settings.read_text(encoding="utf-8"))
        expected = requested_arrays(settings)
        result["requestedArrays"] = expected
        # Python's Windows loader needs explicit dependency directories. Keep
        # their handles alive until both native modules have finished using them.
        dll_directories = []
        if os.name == "nt":
            for path in (args.runtime, args.runtime / "lib", args.runtime / "lib64", *args.library_dir):
                if path.is_dir():
                    dll_directories.append(os.add_dll_directory(str(path.resolve())))
        names = ("soh.dll", "2ship.dll") if os.name == "nt" else ("libsoh.so", "lib2ship.so")
        soh, mm = [ctypes.CDLL(str(args.runtime / name), mode=ctypes.RTLD_GLOBAL) for name in names]
        bind(soh, "SOH_InitRandoHeadless")()
        bind(mm, "MM_InitRandoHeadless")()
        dump_mm = bind(mm, "MM_DumpRandoSettings", ctypes.c_char_p)
        before = json.loads(dump_mm())
        result["beforeArrays"] = {key: before.get(key) for key in ARRAY_KEYS}
        bind(soh, "SOH_RestoreRandoSettings", arguments=(ctypes.c_char_p,))(
            json.dumps(settings["oot"]["settings"]).encode())
        bind(mm, "MM_RestoreRandoSettings", arguments=(ctypes.c_char_p,))(
            json.dumps(settings["mm"]["settings"]).encode())
        snapshots = {
            "oot": json.loads(bind(soh, "SOH_DumpRandoSettings", ctypes.c_char_p)()),
            "mm": json.loads(dump_mm()),
        }
        snapshot_path = args.output.with_name("restored-settings.json")
        write_json(snapshot_path, snapshots)
        result["restoredSettingsSha256"] = sha256(snapshot_path)
        result["restoredArrays"] = {key: snapshots["mm"].get(key) for key in ARRAY_KEYS}
        result["mismatches"] = {key: {"expected": expected[key], "actual": snapshots["mm"].get(key)}
                                for key in ARRAY_KEYS if snapshots["mm"].get(key) != expected[key]}
        # Excluded checks are natively sorted by check ID. Ignore their order
        # while preserving identities and multiplicity; starting items stay exact.
        excluded = "gRando.ExcludedChecks"
        actual_excluded = snapshots["mm"].get(excluded)
        if isinstance(actual_excluded, list) and sorted(actual_excluded) == sorted(expected[excluded]):
            result["mismatches"].pop(excluded, None)
        result["status"] = "FAIL" if result["mismatches"] else "PASS"
        if result["mismatches"]:
            result["reason"] = "Native MM array settings differ after restore"
    except Exception as error:
        result["reason"] = type(error).__name__ + ": " + str(error)
    write_json(args.output, result)
    print("NATIVE SETTINGS " + result["status"] + ": " + json.dumps(result, sort_keys=True), flush=True)
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    sys.exit(main())
