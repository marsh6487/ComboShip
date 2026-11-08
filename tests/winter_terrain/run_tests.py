#!/usr/bin/env python3
"""Native winter material/cache tests and production renderer shading decisions.

Real Fast::Texture/Resource implementations are compiled. Only external logging
and renderer device boundaries are substituted; this is not GPU/runtime proof.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sanitize', action='store_true')
parser.add_argument('--fixtures', type=Path, help='JSON array of existing serialized OTEX file paths')
parser.add_argument('--only-terrain', action='store_true', help='Skip unchanged existing foliage regression suites')
args = parser.parse_args()

def compound(source, start):
    opening = source.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

with tempfile.TemporaryDirectory(prefix='winter-terrain-') as tmp:
    out = Path(tmp)
    (out / 'spdlog').mkdir()
    (out / 'spdlog/spdlog.h').write_text('#pragma once\n#define SPDLOG_TRACE(...) ((void)0)\n')
    flags = ['-std=c++20', '-O1', '-g', '-Wall', '-Wextra', '-I' + str(out), '-I' + str(ROOT / 'soh'),
             '-I' + str(ROOT / 'libultraship/include')]
    if args.sanitize:
        flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-no-pie']
    env = {**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'}
    for name in ['season_foliage_test', 'season_foliage_binding_test', 'season_foliage_cohort_test',
                 'season_ground_tint_polish_test', 'season_winter_terrain_test']:
        if args.only_terrain and name != 'season_winter_terrain_test':
            continue
        extra = ['-I' + str(ROOT / 'soh/tests/season_foliage_stubs')] if name not in (
                 'season_foliage_test', 'season_ground_tint_polish_test') else []
        exe = out / name
        subprocess.run([os.environ.get('CXX', 'c++'), *extra, *flags, str(ROOT / f'soh/tests/{name}.cpp'),
                        str(ROOT / 'libultraship/src/fast/resource/type/Texture.cpp'),
                        str(ROOT / 'libultraship/src/ship/resource/Resource.cpp'), '-o', str(exe)], check=True)
        fixture_args = json.loads(args.fixtures.read_text()) if args.fixtures and name == 'season_winter_terrain_test' else []
        subprocess.run([str(exe), *fixture_args], check=True, env=env)

    engine = (ROOT / 'libultraship/src/fast/interpreter.cpp').read_text()
    header = (ROOT / 'libultraship/include/fast/interpreter.h').read_text()
    primary = engine[engine.index('    const auto& primaryTexture ='):engine.index(
        '    // Keep the disabled specialization')]
    choice = engine[engine.index('        comb = LookupOrCreateColorCombiner(key);'):engine.index(
        '        for (int i = 0; i < 2; i++) {', engine.index('        comb = LookupOrCreateColorCombiner(key);'))]
    uniforms = engine[engine.index('            const auto color = neutralizeShading ?'):engine.index(
        '        // SOH [Enhancement] Toon lighting: world-space normal', engine.index('            const auto color = neutralizeShading ?'))]
    uniforms = uniforms[:uniforms.rindex('        }')]
    # Compile the actual renderer selection and attribute arithmetic. A fake
    # shader lookup/device observes its outputs; no second policy implementation.
    enum = compound(header, header.index('enum class ShaderOpts')) + ';\n'
    rgba = compound(header, header.index('struct RGBA {')) + ';\n'
    cpp = out / 'renderer.cpp'
    cpp.write_text(r'''
#include <cassert>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>
#include "fast/resource/type/Texture.h"
''' + enum + rgba + r'''
#define SHADER_OPT(opt) (uint64_t{1} << static_cast<int>(ShaderOpts::opt))
struct State {
    struct Tile { unsigned tmem_index = 0; } texture_tile[8];
    struct Loaded { struct Meta { std::shared_ptr<Fast::Texture> resource; } raw_tex_metadata; } loaded_texture[2];
    unsigned first_tile_index = 0;
    bool grayscale = false, grayscale_normalize = false, grayscale_frost = false;
    RGBA grayscale_color{70,120,180,100};
};
struct Key { uint64_t options = 0; };
struct Comb { bool usedTextures[2]{}; };
bool actualUse;
Comb* LookupOrCreateColorCombiner(Key) { static Comb c; c.usedTextures[0] = actualUse; return &c; }
int main() {
    for (unsigned bits = 0; bits < 256; ++bits) {
        State state; auto* mRdp = &state;
        const bool is_rect = bits & 1;
        const bool marked = bits & 2;
        actualUse = bits & 4;
        state.grayscale = bits & 8;
        state.grayscale_normalize = bits & 16;
        state.grayscale_frost = bits & 32;
        const unsigned slot = bool(bits & 64); state.texture_tile[0].tmem_index = slot;
        if (!(bits & 128)) {
            for (unsigned i = 0; i < 2; ++i) {
                state.loaded_texture[i].raw_tex_metadata.resource = std::make_shared<Fast::Texture>();
                state.loaded_texture[i].raw_tex_metadata.resource->NeutralizeShading = i == slot ? marked : !marked;
            }
        }
        bool use_grayscale = state.grayscale;
        Key key; if (use_grayscale) key.options |= SHADER_OPT(GRAYSCALE);
        Comb* comb;
''' + primary + choice + r'''
        assert(neutralizeShading == (!is_rect && marked && actualUse && !(bits & 128)));
        assert(use_grayscale == (state.grayscale || neutralizeShading));
        assert(bool(key.options & SHADER_OPT(GRAYSCALE)) == use_grayscale);
        assert(state.grayscale == bool(bits & 8));
        assert(state.grayscale_color.r == 70 && state.grayscale_color.g == 120 && state.grayscale_color.b == 180);
        if (use_grayscale) {
            float mBufVbo[4]{}; unsigned mBufVboLen = 0;
''' + uniforms + r'''
            assert(mBufVboLen == 4);
            if (neutralizeShading) {
                for (float value : mBufVbo) assert(value == 1.0f);
            } else {
                assert(std::fabs(mBufVbo[3] - 100 / 255.0f) < 1e-6f);
            }
        }
        // Cached pure triangle runs reuse exactly the same texture ownership;
        // normal command boundaries invalidate mTriangleState in the engine.
    }
}
''')
    exe = out / 'renderer'
    subprocess.run([os.environ.get('CXX', 'c++'), *flags, str(cpp),
                    str(ROOT / 'libultraship/src/fast/resource/type/Texture.cpp'),
                    str(ROOT / 'libultraship/src/ship/resource/Resource.cpp'), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True, env=env)
    print('PASS production renderer: 256 primary/secondary/unused/rectangle/grayscale cases; neutral shader inputs; no RDP state writes', flush=True)
