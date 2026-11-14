#!/usr/bin/env python3
"""Exercise production combined magic policy, MM oracle import and logical costs."""
import os
from pathlib import Path
import re
import subprocess
import tempfile
from run_time_pedestal_tests import functions

ROOT = Path(__file__).resolve().parents[2]


def macro(source, name):
    lines = source.splitlines(keepends=True)
    for index, line in enumerate(lines):
        if re.match(r"#define " + name + r"\b", line):
            out = line
            while out.rstrip().endswith("\\"):
                index += 1
                out += lines[index]
            return out
    raise RuntimeError("missing production macro: " + name)


def main():
    port = (ROOT / "mm/2s2h/BenPort.cpp").read_text()
    native = port[port.index("static void GiveItemForOracle"):]
    native = native[native.index("        case RI_OOT_EXT_MAGIC_CAPE:"):native.index("        case RI_DOUBLE_DEFENSE:")]
    adapter = functions(port.replace('extern "C" COMBO_EXPORT ', ''))["Combo_MM_Rando_SetOwnedItems"]
    logic = (ROOT / "mm/2s2h/Rando/Logic/Logic.h").read_text()
    helpers = functions(logic)
    helper = helpers["CanUseMagicAmount"] + "\n" + helpers["CanUseMagicArrowAmount"]
    capacity = (ROOT / "mm/include/rando_capacity.h").read_text()
    infinite = functions(capacity)["RandoCapacity_InfiniteMagic"]
    production = "static void GiveItemForOracle(RandoItemId ri) { switch (ri) {\n" + native + "} }\n"
    production += adapter + "\n" + infinite + "\nnamespace Rando::Logic {\n" + helper + "\n}\n"
    production += macro(logic, "HAS_MAGIC") + macro(logic, "CAN_USE_MAGIC_ARROW")
    production += macro(logic, "CAN_USE_DEKU_BUBBLE") + macro(logic, "CAN_GORON_SPIKE_ROLL")
    with tempfile.TemporaryDirectory(prefix="combo-rpg-magic-") as tmp:
        path = Path(tmp)
        (path / "rpg_magic_oracle_production.inc").write_text(production)
        flags = [os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-g", "-fsanitize=undefined",
                 "-fno-sanitize-recover=undefined", "-Icombo", "-Imm", "-I" + str(ROOT.parent / "deps"), "-I" + tmp]
        for test, extra in [("rpg_magic_oracle_test", ["mm/mods/combo_rpg.cpp"]),
                            ("rpg_magic_generation_test", [])]:
            binary = path / test
            subprocess.run(flags + ["combo/tests/" + test + ".cpp"] + extra + ["-o", str(binary)],
                           cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
