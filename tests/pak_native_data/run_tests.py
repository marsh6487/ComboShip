#!/usr/bin/env python3
"""Compile each real PAK translator against native libultraship GBI types, legacy/wide.

--source-ref can demonstrate the same fixture failing before the boundary fix.
Unrelated archive discovery, CVar/UI and resource-manager services are excluded.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source-ref')
parser.add_argument('--mode', choices=('legacy', 'wide', 'both'), default='both')
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()

with tempfile.TemporaryDirectory(prefix='pak-native-data-') as directory:
    temporary = Path(directory)
    for game in ('soh', 'mm'):
        path = f'{game}/mods/pak_loader/pak_loader.cpp'
        source = subprocess.check_output(['git', 'show', f'{args.source_ref}:{path}'], cwd=ROOT).decode() if args.source_ref else (ROOT / path).read_text()
        helpers = source[source.index('static inline u32 BE_U32'):source.index('// Texture Format Constants')]
        conversion = source[source.index('struct SwapContext {'):source.index('// ZOBJ Skeleton Parser')]
        # Trailing separator comments belong to the following section.
        (temporary / 'translator.inc').write_text(helpers + '\n' + conversion)
        for mode in (('legacy', 'wide') if args.mode == 'both' else (args.mode,)):
            binary = temporary / f'{game}-{mode}'
            flags = ['-DGBI_FLOAT_MTX=1', '-DGBI_S32_VTX=1'] if mode == 'wide' else []
            if 'nativeBuffers' in source:
                flags += ['-DPAK_CONVERTED_NATIVE_DATA=1']
            if args.sanitize:
                flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-no-pie']
            subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-O1', '-g', '-DF3DEX_GBI_2',
                            *flags, '-I' + str(temporary), '-I' + str(ROOT),
                            '-I' + str(ROOT / 'libultraship/include'),
                            str(ROOT / 'tests/pak_native_data/translator_test.cpp'), '-o', str(binary)], check=True)
            print(f'{game} {mode}', flush=True)
            subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
