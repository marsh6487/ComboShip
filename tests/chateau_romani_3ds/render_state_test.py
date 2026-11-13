#!/usr/bin/env python3
"""Replay the shipped Chateau materials under the real game/renderer state contract.

The native Setup25 helpers use an opaque fog blender even on the XLU stream.
This check reads that setup from both games, then feeds the material commands
through the same fog/alpha predicates used by Fast3D. It does not substitute a
transparent preview material for the actual display list.
"""
import os
from pathlib import Path
import re
import shlex
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PREFIX = "objects/combo_chateau_romani_3ds/"
MATERIALS = {
    "Milk": (False, 40, 255),
    "Cork": (False, 56, 255),
    "GlassSurface": (True, 180, 46),
    "GlassReflection": (True, 180, 110),
    "Label": (False, 8, 255),
    "LabelReflection": (True, 8, 40),
}
HASH_COMMANDS = {0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42}


def setup_mode(game):
    source = (ROOT / game / "src/code/z_rcp.c").read_text()
    setup = source.split("/* SETUPDL_25 */", 1)[1].split("gsSPEndDisplayList", 1)[0]
    match = re.search(r"gsDPSetOtherMode\([^,]+,\s*([^)]*)\)", setup)
    assert match, "Cannot find the native Setup25 render mode"
    return match.group(1)


def renderer_probe(directory):
    source = (ROOT / "libultraship/src/fast/interpreter.cpp").read_text()
    start = source.index("use_alpha = ((mRdp->other_mode_l")
    end = source.index("bool texture_edge", start)
    predicates = source[start:end]
    fixture = r'''
#include <libultraship/libultra/gbi.h>
#include <cstdio>
#include <cstdlib>
int main(int argc, char** argv) {
    if (argc == 1) {
        std::printf("%u %u %u %u\n", unsigned(OOT_SETUP), unsigned(MM_SETUP),
                    unsigned(G_FOG), unsigned(Z_UPD));
        return 0;
    }
    for (int i = 1; i < argc; ++i) {
        struct { uint32_t other_mode_l; } rdp{uint32_t(std::strtoul(argv[i], nullptr, 10))};
        auto* mRdp = &rdp;
        bool use_alpha, use_blend_color, use_fog;
        PRODUCTION_PREDICATES
        std::printf("%u %d %d\n", rdp.other_mode_l, int(use_alpha), int(use_fog));
    }
}
'''.replace("OOT_SETUP", setup_mode("soh")).replace("MM_SETUP", setup_mode("mm"))
    fixture = fixture.replace("PRODUCTION_PREDICATES", predicates)
    source_path, binary = directory / "probe.cpp", directory / "probe"
    source_path.write_text(fixture)
    subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-DF3DEX_GBI_2",
                    "-I" + str(ROOT / "libultraship/include"), str(source_path), "-o", str(binary)], check=True)
    return binary


def triangle_states(raw, inherited_mode):
    assert raw[4:8] == b"TLDO" and raw[64] == 4, "Expected native F3DEX2 display list"
    offset, mode, geometry, prim_alpha = 72, inherited_mode, 0, 255
    states = []
    while offset + 8 <= len(raw):
        a, b = struct.unpack_from("<II", raw, offset)
        opcode = a >> 24
        if opcode == 0xE2:
            length = (a & 255) + 1
            shift = 32 - ((a >> 8) & 255) - length
            assert 0 <= shift and shift + length <= 32
            mask = ((1 << length) - 1) << shift
            mode = (mode & ~mask) | (b & mask)
        elif opcode == 0xD9:
            geometry = (geometry & (a & 0xFFFFFF)) | b
        elif opcode == 0xFA:
            prim_alpha = b & 255
        elif opcode == 0x05:
            states.append((mode, geometry, prim_alpha))
        elif opcode == 0xDF:
            assert offset + 8 == len(raw)
            return states
        offset += 16 if opcode in HASH_COMMANDS else 8
    raise AssertionError("Display list has no terminator")


with tempfile.TemporaryDirectory(prefix="chateau-render-state-") as temporary:
    probe = renderer_probe(Path(temporary))
    oot_mode, mm_mode, fog_flag, depth_write = map(int, subprocess.check_output([str(probe)], text=True).split())
    cases = []
    for host in ("soh", "mm"):
        for role, (translucent, count, alpha) in MATERIALS.items():
            resource = ROOT / host / "assets/custom" / (PREFIX + role + "DL")
            other = ROOT / ("mm" if host == "soh" else "soh") / "assets/custom" / (PREFIX + role + "DL")
            assert resource.read_bytes() == other.read_bytes(), "Host material copies differ: " + role
            for inherited in {oot_mode, mm_mode}:
                states = triangle_states(resource.read_bytes(), inherited)
                assert len(states) == count, "Geometry changed: " + role
                cases.append((host, role, translucent, alpha, states))
    modes = sorted({mode for _, _, _, _, states in cases for mode, _, _ in states})
    results = subprocess.check_output([str(probe), *map(str, modes)], text=True)
    renderer = {mode: (bool(alpha), bool(fog)) for mode, alpha, fog in
                (map(int, line.split()) for line in results.splitlines())}
    failures = []
    for host, role, translucent, expected_alpha, states in cases:
        for mode, geometry, alpha in set(states):
            use_alpha, use_fog = renderer[mode]
            if (use_fog or geometry & fog_flag or use_alpha != translucent or
                    bool(mode & depth_write) == translucent or alpha != expected_alpha):
                failures.append(f"{host}/{role}: render mode=0x{mode:08x}, fog={use_fog}, "
                                f"alpha blend={use_alpha}, depth write={bool(mode & depth_write)}, "
                                f"primitive alpha={alpha}; expected fog=False, "
                                f"alpha blend={translucent}, depth write={not translucent}, alpha={expected_alpha}")
    assert not failures, "Chateau material state is unsafe under native Setup25:\n" + "\n".join(failures)
    print("PASS Chateau render state: 12 host/material copies; native Setup25; production Fast3D fog/alpha predicates; "
          "opaque depth writes and translucent blending; geometry and authored alpha preserved")
