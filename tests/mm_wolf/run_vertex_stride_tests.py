#!/usr/bin/env python3
"""Traverse both production Wolf lists through the native s32 vertex resolver."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def function(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

with tempfile.TemporaryDirectory(prefix='wolf-stride-') as td:
    for game in ('soh', 'mm'):
        builder = function((ROOT / game / 'mods/transformation_masks/wolf_link_form.cpp').read_text(),
                           'static void BuildMeshDisplayList(')
        resolver = function((ROOT / 'libultraship/src/fast/interpreter.cpp').read_text(),
                            'F3DVtx* Interpreter::SegAddrVtx(')
        source = r'''
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <libultraship/libultra/gbi.h>
#include <fast/lus_gbi.h>
using u32 = uint32_t;
using F3DVtx = Fast::F3DVtx;
static std::vector<Gfx> sMeshDl;
struct Interpreter {
    uintptr_t mSegmentPointers[16]{};
    F3DVtx* SegAddrVtx(uintptr_t);
};
''' + builder + '\n' + resolver + r'''
int main() {
    static_assert(sizeof(Vtx) == 24 && sizeof(F3DVtx) == sizeof(Vtx));
    for (u32 vertices : {8112u, 8640u}) {
        BuildMeshDisplayList(vertices);
        std::vector<F3DVtx> buffer(vertices);
        Interpreter interpreter;
        interpreter.mSegmentPointers[8] = reinterpret_cast<uintptr_t>(buffer.data());
        size_t visited = 0;
        for (const auto& cmd : sMeshDl) {
            if ((cmd.words.w0 >> 24) != G_VTX) continue;
            const size_t count = (cmd.words.w0 >> 12) & 0xff;
            auto* resolved = interpreter.SegAddrVtx(cmd.words.w1);
            if (resolved != buffer.data() + visited || visited + count > vertices) {
                std::fprintf(stderr, "FAIL native s32 Wolf: batch start=%zu resolved=%zu count=%zu vertices=%u\n",
                             visited, (reinterpret_cast<uintptr_t>(resolved) - reinterpret_cast<uintptr_t>(buffer.data())) / sizeof(F3DVtx), count, vertices);
                return 1;
            }
            for (size_t vertex = 0; vertex < count; ++vertex) resolved[vertex].v.ob[0] = visited + vertex;
            visited += count;
        }
        if (visited != vertices) return 2;
        for (size_t vertex = 0; vertex < vertices; ++vertex)
            if (buffer[vertex].v.ob[0] != vertex) return 3;
        std::printf("PASS native s32 Wolf %u vertices: every batch resolves consecutively within the buffer\n", vertices);
    }
}
'''
        cpp = Path(td) / (game + '.cpp'); cpp.write_text(source)
        binary = str(Path(td) / game)
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-DGBI_S32_VTX=1',
                        '-DF3DEX_GBI_2', '-I' + str(ROOT / 'libultraship/include'),
                        '-fsanitize=undefined,address', '-fno-sanitize-recover=all',
                        str(cpp), '-o', binary], check=True)
        subprocess.run([binary], env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"}, check=True)
