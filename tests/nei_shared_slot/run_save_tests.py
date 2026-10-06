#!/usr/bin/env python3
"""Exercise both hosts' NEI persistence with their native save data and real JSON.

The complete MM NeiSaveData JSON overloads and OoT NEI save/load functions are
extracted verbatim. OoT uses the production SaveManager declaration, templates,
and array traversal; a fixture binds its current section synchronously. Thread
startup and unrelated photo/trade sidecars are outside this test boundary.

--serializer-ref compiles older serializers against the current native data
headers to demonstrate that omitting the appended flags fails these checks.
Requires the normal nlohmann/json.hpp build dependency (or CPATH).
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from


def source_at(path, ref=None):
    if ref:
        return subprocess.check_output(["git", "show", f"{ref}:{path}"], cwd=ROOT, text=True)
    return (ROOT / path).read_text()


def extract(source, name):
    match = re.search(r'^(?:extern "C" )?(?:static )?[\w: *]+\b' + re.escape(name)
                      + r"\s*\([^;{}]*\)\s*\{", source, re.M)
    if not match:
        raise RuntimeError(f"Production function not found: {name}")
    return block_from(source, match.start())


def option_block(source, name):
    start = source.index(f"#define {name}(")
    end = source.index(f"#undef {name}", start) + len(f"#undef {name}")
    return source[start:end]


def production_for(host, serializer_ref):
    save = source_at(f"{host}/mods/nei_save.cpp")
    inventory = source_at(f"{host}/mods/extended_inventory.c")
    names = ("Nei_Save", "Nei_GetOwnedItem", "Nei_SetOwnedItem")
    parts = [extract(save, name) for name in names]
    if host == "mm":
        parts.append(extract(save, "Nei_InitNewSave"))
        conversions = source_at("mm/2s2h/BenJsonConversions.hpp", serializer_ref)
        for signature in ("inline void to_json(json& j, const NeiSaveData& n)",
                          "inline void from_json(const json& j, NeiSaveData& n)"):
            parts.append(block_from(conversions, conversions.index(signature)))
        fleet = source_at("mm/2s2h/FleetShipCombo/FleetSync.cpp")
    else:
        parts.insert(0, "static NeiSaveData gNeiSave;")
        parts.append(extract(save, "NeiSave_Init"))
        serializers = source_at("soh/mods/nei_save.cpp", serializer_ref)
        parts.extend(extract(serializers, name) for name in ("NeiSave_Save", "NeiSave_Load"))
        manager = source_at("soh/soh/SaveManager.cpp")
        parts.extend(extract(manager, name) for name in ("SaveManager::SaveArray", "SaveManager::LoadArray"))
        fleet = source_at("soh/soh/FleetShipCombo/FleetSync.cpp")
    parts.extend(extract(inventory, name) for name in
                 ("GraceHourglass_Heal", "GraceHourglass_IsOwned", "GraceHourglass_Grant",
                  "Slate_RuneOwned", "Slate_RuneCount", "Slate_RuneAt", "Slate_GetRune", "Slate_SetRune"))
    for name in ("FCO_EXTRACT", "FCO_APPLY"):
        qualifier = "" if name == "FCO_EXTRACT" else "const "
        parts.append(f"static void Fixture_{name}({qualifier}json& sh) {{\n"
                     "    auto* nei = Nei_Save();\n" + option_block(fleet, name) + "\n}")
    return "\n\n".join(parts) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--serializer-ref", help="git revision supplying only the NEI serializers")
    parser.add_argument("--host", choices=("mm", "soh"), help="test only one host")
    parser.add_argument("--sanitizers", action="store_true", help="enable AddressSanitizer and UBSan")
    args = parser.parse_args()
    hosts = (args.host,) if args.host else ("mm", "soh")
    failed = False
    for host in hosts:
        print(f"Testing {host}; serializers: {args.serializer_ref or 'working tree'}", flush=True)
        with tempfile.TemporaryDirectory(prefix=f"nei-save-{host}-") as directory:
            build = Path(directory)
            (build / "nei_save_production.inc").write_text(production_for(host, args.serializer_ref))
            # SaveManager holds only a shared_ptr to this type here. The fixture
            # constructor does not create threads or register unrelated sections.
            (build / "BS_thread_pool.hpp").write_text("#pragma once\nnamespace BS { class thread_pool; }\n")
            includes = ["-I" + str(p) for p in (build, ROOT, ROOT / host, ROOT / host / "include",
                        ROOT / host / "include/PR", ROOT / host / "2s2h", ROOT / host / "assets",
                        ROOT / "libultraship/include", ROOT / "combo")]
            flags = ["-std=c++20", "-O1", "-g", "-DF3DEX_GBI_2", "-DCOMBO_BUILD",
                     "-DLOG_LEVEL_GAME_PRINTS=0", "-DHOST_MM=" + str(int(host == "mm"))]
            if args.sanitizers:
                flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"]
            binary = build / "nei_save_test"
            subprocess.run([os.environ.get("CXX", "c++"), *flags, *includes,
                            str(ROOT / "tests/nei_shared_slot/save_test.cpp"), "-o", str(binary)],
                           cwd=ROOT, check=True)
            result = subprocess.run([str(binary)], cwd=ROOT)
            failed |= result.returncode != 0
    return int(failed)


if __name__ == "__main__":
    sys.exit(main())
