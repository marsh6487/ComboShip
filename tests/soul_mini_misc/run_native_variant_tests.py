#!/usr/bin/env python3
"""Compare the GI gold-variant limb mapping with each actual EnSw callback."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def block(text, start):
    begin = text.index('{', start)
    depth, end = 1, begin + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


with tempfile.TemporaryDirectory(prefix='soul-misc-native-') as temporary:
    work = Path(temporary)
    for host in ('soh', 'mm'):
        source = (ROOT / host / 'src/overlays/actors/ovl_En_Sw/z_en_sw.c').read_text()
        callback = source.index('s32 EnSw_OverrideLimbDraw(')
        native_switch = block(source, source.index('switch (limbIndex)', callback))
        fixture = '''#include <cassert>
#include <cstring>
#include "combo/menu/ComboGoldSkulltulaLimbs.h"
#include "objects/object_st/object_st.h"
static const char* NativeGoldMesh(int limbIndex) {
    const char* result = nullptr;
    const char** dList = &result;
    NATIVE_SWITCH
    return result;
}
int main() {
    assert(sizeof(ComboGoldSkulltula::Limbs) / sizeof(ComboGoldSkulltula::Limbs[0]) == 10);
    int changes = 0;
    for (int limb = 0; limb <= 30; ++limb) {
        const char* native = NativeGoldMesh(limb);
        const char* soul = ComboGoldSkulltula::LimbPath(limb);
        assert((native == nullptr) == (soul == nullptr));
        if (native) { assert(!std::strcmp(native, soul)); ++changes; }
    }
    assert(changes == 10);
}
'''.replace('NATIVE_SWITCH', native_switch)
        cpp, binary = work / (host + '.cpp'), work / host
        cpp.write_text(fixture)
        flags = ['-std=c++17', '-Wall', '-Wextra', '-Werror',
                 '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie']
        includes = ['-I' + str(ROOT), '-I' + str(ROOT / host / 'assets'),
                    '-I' + str(ROOT / host / 'include')]
        subprocess.run([os.environ.get('CXX', 'c++'), *flags, *includes, str(cpp), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
        print('PASS', host, 'gold soul mesh mapping equals all ten native EnSw substitutions; remaining limbs untouched')
