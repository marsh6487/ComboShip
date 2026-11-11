#!/usr/bin/env python3
"""Execute both production Din draw paths with native host structures and GBI."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--host", choices=("soh", "mm"))
    parser.add_argument("--standalone", action="store_true", help="compile the original host without COMBO_BUILD")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="dins-remake-") as temp:
        for host in ((args.host,) if args.host else ("soh", "mm")):
            flags = ["-std=gnu2x", "-O1", "-g", "-ffunction-sections", "-fdata-sections",
                     "-DF3DEX_GBI_2", "-DGBI_FLOAT_MTX", "-DGBI_S32_VTX",
                     "-D" + ("MM_BUILD_DLL" if host == "mm" else "SOH_BUILD_DLL"),
                     "-DCONTROLLERBUTTONS_T=uint32_t", "-DNON_EQUIVALENT", "-DNON_MATCHING",
                     "-DLOG_LEVEL_GAME_PRINTS=0", "-Werror=implicit-function-declaration",
                     "-Wno-incompatible-pointer-types", "-Wno-int-conversion"]
            if not args.standalone:
                flags.append("-DCOMBO_BUILD")
            if args.sanitize:
                flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer",
                          "-fno-pie", "-no-pie"]
            flags += ["-I" + str(ROOT / p) for p in
                      (".", host + "/include", host + "/include/PR", host + "/src", host + "/assets", host,
                       host + ("/2s2h" if host == "mm" else "/soh"), "libultraship/include", "libultraship/src", "combo")]
            flags += ["-I" + temp]
            for name, value in re.findall(r'set\((CVAR_PREFIX_\w+) "([^"]+)"\)',
                                          (ROOT / "CMake/soh-cvars.cmake").read_text()):
                flags.append(f'-D{name}="{value}"')
            path = ("mm/expansions/sw97/actors/spells/z_magic_fire.inc.c" if host == "mm" else
                    "soh/src/overlays/actors/ovl_Magic_Fire/z_magic_fire.c")
            source = (ROOT / path).read_text()
            indices = re.search(r"static u8 sVertexIndices\[\] = \{.*?\};", source, re.S)[0]
            helper = ('#define CW_DINS_HOST_MM\n' if host == "mm" else "")
            if "DinsFireRemakeDraw.h" in source:
                helper += '#include "combo/render/DinsFireRemakeDraw.h"\n'
            if host == "mm":
                flags.append("-DTEST_HOST_MM")
                struct = source[source.index("typedef struct MagicFire {"):source.index("} MagicFire;") + len("} MagicFire;")]
                names = re.findall(r"^static const .*sMagicFire(?:Oot|Medallion).*;$", source, re.M)
                draws = [function(source, n) for n in ("MagicFire_GetOotMaterialDL", "MagicFire_GetOotModelDL",
                         "MagicFire_GetOotTex", "MagicFire_GetOotSphereVtx", "MagicFireDins_Draw")]
                body = "\n".join([struct, indices, *names, helper, *draws])
            else:
                body = '\n'.join(['#include "overlays/actors/ovl_Magic_Fire/z_magic_fire.h"',
                                  '#include "overlays/ovl_Magic_Fire/ovl_Magic_Fire.h"', indices, helper,
                                  function(source, "MagicFire_Draw")])
            (Path(temp) / "dins_draw_production.inc").write_text(body)
            graph = (ROOT / host / "src/code/graph.c").read_text()
            (Path(temp) / "dins_graph_production.inc").write_text("\n".join(
                function(graph, name) for name in ("Graph_OpenDisps", "Graph_CloseDisps")))
            rcp = (ROOT / host / "src/code/z_rcp.c").read_text()
            table_name = "gSetupDLs" if host == "mm" else "sSetupDL"
            table = re.search(r"^Gfx " + table_name + r"\[SETUPDL_MAX\]\[6\] = \{.*?^\};", rcp, re.M | re.S)[0]
            setup_names = (("Gfx_SetupDL25_Xlu", "Gfx_SetupDL57") if host == "mm" else
                           ("Gfx_SetupDL_25Xlu", "Gfx_SetupDL_57"))
            (Path(temp) / "dins_setup_production.inc").write_text("\n".join(
                [table, *(function(rcp, name) for name in setup_names)]))
            binary = Path(temp) / host
            command = [*shlex.split(os.environ.get("CC", "cc")), *flags,
                       "combo/tests/dins_fire_remake_draw_test.c", "-Wl,--gc-sections", "-lm", "-o", str(binary)]
            compiled = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
            if compiled.returncode:
                raise RuntimeError(compiled.stdout + compiled.stderr)
            subprocess.run([str(binary)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "scripts/diagnostics/check_dins_fire_native_geometry.py")], check=True)


if __name__ == "__main__":
    main()
