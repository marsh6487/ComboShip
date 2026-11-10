"""Exercise spring's production combiner through the real renderer generator.

Regression: a two-cycle spring material must never sample the unowned
second texture slot while multiplying the existing primitive fade alpha.
No game/GPU fixture is needed for the renderer's texture-dependency decision.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
renderer = (ROOT / 'libultraship/src/fast/interpreter.cpp').read_text()
header = (ROOT / 'libultraship/include/fast/interpreter.h').read_text()
leaf = (ROOT / 'combo/NeiSpringBlossoms.h').read_text()

def block(text, start):
    begin = text.index(start)
    opening = text.index('{', begin)
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[begin:end]

generator = block(renderer, 'void Interpreter::GenerateCC').replace('Interpreter::GenerateCC', 'GenerateCC')
definitions = block(header, 'enum {\n    SHADER_0') + ';\n'
definitions += block(header, 'enum class ShaderOpts') + ';\n'
definitions += '#define SHADER_OPT(opt) ((uint64_t)(1 << static_cast<int>(ShaderOpts::opt)))\n'
definitions += block(header, 'struct ColorCombinerKey') + ';\n'
definitions += block(header, 'struct ColorCombiner {') + ';\n'
call = re.search(r'gDPSetCombineLERP\(cursor\+\+,\s*(.*?)\);', leaf, re.S).group(1)
source = r'''
#include <cstdint>
#include <cstring>
#include <cstdio>
#include "fast/lus_gbi.h"
using namespace Fast;
''' + definitions + generator + r'''
static uint32_t rgb(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    return (a & 15) | ((b & 15) << 4) | ((c & 31) << 8) | ((d & 7) << 13);
}
static uint32_t alpha(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    return (a & 7) | ((b & 7) << 3) | ((c & 7) << 6) | ((d & 7) << 9);
}
#define MAKE(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) \
    (uint64_t(rgb(G_CCMUX_##a,G_CCMUX_##b,G_CCMUX_##c,G_CCMUX_##d)) | \
    (uint64_t(alpha(G_ACMUX_##e,G_ACMUX_##f,G_ACMUX_##g,G_ACMUX_##h)) << 16) | \
    (uint64_t(rgb(G_CCMUX_##i,G_CCMUX_##j,G_CCMUX_##k,G_CCMUX_##l)) << 28) | \
    (uint64_t(alpha(G_ACMUX_##m,G_ACMUX_##n,G_ACMUX_##o,G_ACMUX_##p)) << 44))
#define EXPAND(a,b) MAKE(a,b)
int main() {
    ColorCombinerKey key{};
    key.options = SHADER_OPT(_2CYC) | SHADER_OPT(ALPHA) | SHADER_OPT(FOG);
    key.combine_mode = MAKE(''' + call + r''');
    ColorCombiner comb{};
    GenerateCC(&comb, key);
    printf("Production spring material: texture0=%d texture1=%d\n", comb.usedTextures[0], comb.usedTextures[1]);
    if (!comb.usedTextures[0] || comb.usedTextures[1]) {
        puts("FAIL: spring depends on an uninitialized second texture slot");
        return 1;
    }
    // Texture RGB/alpha are modulated by vertex shade in cycle0, and
    // the particle primitive tint/fade in cycle1. The generator must retain
    // these dependencies without aliasing cycle1 to an unowned TEXEL1.
    if (((comb.shader_id0 >> 32) & 15) != SHADER_COMBINED ||
        ((comb.shader_id0 >> 48) & 15) != SHADER_COMBINED) {
        puts("FAIL: second cycle lost combined color or alpha");
        return 1;
    }
    puts("PASS: spring uses owned TEXEL0 with shade and particle fade alpha");
}
'''
with tempfile.TemporaryDirectory(prefix='spring-combiner-') as directory:
    cpp = Path(directory) / 'test.cpp'
    binary = Path(directory) / 'test'
    cpp.write_text(source)
    subprocess.run(['c++', '-std=c++20', '-I' + str(ROOT / 'libultraship/include'), str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
