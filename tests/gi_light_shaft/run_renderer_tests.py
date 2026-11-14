#!/usr/bin/env python3
"""Exercise the production shaft renderer with each host's native graphics ABI.

The generated C translation unit contains verbatim production matrix, allocation,
debug-display, setup-DL and scroll functions. The C++ test includes the production
renderer directly. Only archive lookups, interpolation recording and logging are
fixture boundaries. No render, matrix or allocation algorithm is reimplemented.
This is command/CPU evidence, not an execution of the GPU interpreter or a visual
acceptance test. Both original and COMBO_BUILD modes use the shared native
GBI_FLOAT_MTX / GBI_S32_VTX ABI.
"""

import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def extract_function(path, name):
    source = (ROOT / path).read_text()
    match = re.search(
        r'^(?:extern "C" )?(?:static )?[A-Za-z_][\w *]*\b' + re.escape(name)
        + r'\s*\([^;{}]*\)\s*\{', source, re.M)
    if not match:
        raise ValueError(f"missing production function {name} in {path}")
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    # C linkage has its native meaning when this exact body is compiled as C.
    function = source[match.start():end].removeprefix('extern "C" ')
    line = source.count("\n", 0, match.start()) + 1
    return f'#line {line} "{path}"\n{function}\n'


def extract_setup_table(path, name):
    source = (ROOT / path).read_text()
    start = source.index(f"Gfx {name}[SETUPDL_MAX][6] = {{")
    end, depth = source.index("{", start) + 1, 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    assert source[end] == ";"
    line = source.count("\n", 0, start) + 1
    return f'#line {line} "{path}"\n{source[start:end + 1]}\n'


def write_native(build, host):
    mm = host == "mm"
    prefix = "mm" if mm else "soh"
    matrix = f"{prefix}/src/code/sys_matrix.c"
    graph = f"{prefix}/src/code/graph.c"
    rcp = f"{prefix}/src/code/z_rcp.c"
    wrappers = "mm/src/code/stubs.c" if mm else "soh/soh/GbiWrap.cpp"
    interpolation = ("2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
                     if mm else "soh/frame_interpolation.h")
    source = [
        '#include "z64.h"\n#include "functions.h"\n#include "variables.h"\n'
        '#include "macros.h"\n#include <assert.h>\n#include <string.h>\n',
        f'#include "{interpolation}"\n',
        "int ResourceMgr_OTRSigCheck(char*);\n"
        "Gfx* ResourceMgr_LoadGfxByName(char*);\n"
        "Vtx* ResourceMgr_LoadVtxByName(char*);\n"
        "Gfx* PakLoader_GetDLOverride(const char*);\n",
        ("MtxF* sMatrixStack;\nMtxF* sCurrentMatrix;\n" if mm else
         "static MtxF* sMatrixStack;\nstatic MtxF* sCurrentMatrix;\n"),
    ]
    if not mm:
        source += [
            "static GameInfo rendererGameInfo;\nGameInfo* gGameInfo = &rendererGameInfo;\n",
            extract_function("soh/src/code/TwoHeadArena.c", "THA_AllocEnd"),
            extract_function("soh/src/code/TwoHeadArena.c", "THGA_AllocEnd"),
            extract_function(graph, "Graph_Alloc"),
        ]
    source += [extract_function(graph, name)
               for name in ("Graph_OpenDisps", "Graph_CloseDisps")]
    source += [extract_function(f"{prefix}/src/code/z_skin_matrix.c", "SkinMatrix_SetTranslate")]
    source += [extract_function(matrix, name)
               for name in ("Matrix_MtxFCopy", "Matrix_Push", "Matrix_Pop", "Matrix_Translate")]
    if not mm:
        source += [extract_function(matrix, "Matrix_CheckFloats")]
    source += [extract_function("mm/2s2h/gu_pc.c" if mm else "soh/soh/gu_pc.c", "guMtxF2L")]
    source += [extract_function(matrix, "Matrix_ToMtx"),
               extract_function(matrix, "Matrix_Finalize" if mm else "Matrix_NewMtx")]
    source += [extract_setup_table(rcp, "gSetupDLs" if mm else "sSetupDL")]
    source += [extract_function(rcp, "Gfx_SetupDL25_Xlu" if mm else "Gfx_SetupDL_25Xlu"),
               extract_function(rcp, "Gfx_TexScrollEx")]
    source += [extract_function(wrappers, name)
               for name in ("gSPDisplayList", "gSPVertex", "gDPSetTileSizeLerp")]
    source += ['''
#line 1 "renderer_fixture_stack_access"
/* Test-only stack storage/access; all drawing operations above are native. */
static MtxF rendererStack[20];
void RendererFixture_ResetNativeStack(const MtxF* caller, int depth) {
    assert(depth >= 0 && depth < 19);
    memset(rendererStack, 0, sizeof(rendererStack));
    sMatrixStack = rendererStack;
    sCurrentMatrix = sMatrixStack + depth;
    *sCurrentMatrix = *caller;
}
MtxF* RendererFixture_CurrentNativeMatrix(void) { return sCurrentMatrix; }
int RendererFixture_NativeStackDepth(void) { return (int)(sCurrentMatrix - sMatrixStack); }
''']
    native = build / f"renderer_native_{host}.c"
    native.write_text("\n".join(source))
    return native


def run(command):
    result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True,
                            env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
    if result.returncode:
        print(result.stdout + result.stderr, end="", flush=True)
        raise subprocess.CalledProcessError(result.returncode, command)
    if result.stdout:
        print(result.stdout, end="", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    cc = os.environ.get("CC", "gcc-14")
    cxx = os.environ.get("CXX", "g++-14")
    common = ["-DF3DEX_GBI_2", "-DGBI_FLOAT_MTX=1", "-DGBI_S32_VTX=1",
              "-DLOG_LEVEL_GAME_PRINTS=6", "-O2", "-ffast-math", "-fno-strict-aliasing"]
    if args.sanitize:
        common += ["-O1", "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                   "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    with tempfile.TemporaryDirectory(prefix="gi-shaft-renderer-") as directory:
        build = Path(directory)
        for host in ("soh", "mm"):
            include_paths = (host, f"{host}/include", f"{host}/src", f"{host}/assets",
                             f"{host}/2s2h" if host == "mm" else f"{host}/soh",
                             "libultraship/include", "combo", "combo/menu", "")
            flags = common + [f"-I{ROOT / path}" for path in include_paths]
            if host == "mm":
                flags += ["-DSHAFT_TEST_MM", f"-I{ROOT / 'mm/include/PR'}"]
            native = write_native(build, host)
            for combo in (False, True):
                label = host + ("-combo" if combo else "-original")
                configuration = flags + (["-DCOMBO_BUILD"] if combo else [])
                obj = build / (label + ".o")
                binary = build / label
                run([cc, "-std=gnu2x", "-Wno-incompatible-pointer-types", *configuration,
                     "-c", str(native), "-o", str(obj)])
                run([cxx, "-std=c++20", *configuration,
                     str(ROOT / "tests/gi_light_shaft/renderer_test.cpp"), str(obj), "-o", str(binary)])
                run([str(binary)])
    print("PASS renderer suite: exact native helpers, four host/build configurations"
          + (", ASan/UBSan" if args.sanitize else ", native fast-math"))


if __name__ == "__main__":
    main()
