#!/usr/bin/env python3
"""Run MM's production pause-ammo renderer with native headers and GBI commands."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
FIXTURE = Path(__file__).with_name('ammo_color_test.c')

def function(source, name):
    match = re.search(r'^[\w* ]+\b' + re.escape(name) + r'\([^;]*?\)\s*\{.*?^}', source, re.M | re.S)
    if match is None:
        raise RuntimeError('Cannot find production function ' + name)
    return match[0]

def array(source, name):
    match = re.search(r'(?m)^(?:static )?(?:const )?[\w* ]+\b' + re.escape(name) + r'\[[^;]+?\n};', source, re.S)
    if match is None:
        raise RuntimeError('Cannot find production array ' + name)
    return match[0]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-ref')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    def read(path):
        if args.source_ref:
            return subprocess.check_output(['git', 'show', args.source_ref+':'+path], cwd=ROOT, text=True)
        return (ROOT/path).read_text()
    parameter = read('mm/src/code/z_parameter.c')
    inventory = read('mm/src/code/z_inventory.c')
    pause = read('mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_item.c')
    graph = read('mm/src/code/graph.c')
    production = '\n\n'.join([
        *[array(inventory, name) for name in ('gBitFlags','gItemSlots','gUpgradeMasks','gUpgradeShifts','gUpgradeCapacities')],
        array(parameter,'gPlayerFormItemRestrictions'), array(pause,'gPlayerFormSlotRestrictions'),
        function(graph,'Graph_OpenDisps'), function(graph,'Graph_CloseDisps'),
        function(pause,'KaleidoScope_DrawAmmoCount'),
    ])
    with tempfile.TemporaryDirectory(prefix='mm-ammo-color-') as temporary:
        folder = Path(temporary)
        (folder/'ammo_production.inc').write_text(production)
        binary = folder/'test'
        flags = ['-std=gnu11','-w','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX',
                 '-DCONTROLLERBUTTONS_T=uint32_t','-Werror=implicit-function-declaration',
                 '-Werror=incompatible-pointer-types']
        if args.sanitize:
            flags += ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer',
                      '-fno-pie','-no-pie','-g']
        includes = ['-I'+str(ROOT/p) for p in (
            'mm','mm/include','mm/include/PR','mm/src','mm/assets','mm/2s2h',
            'libultraship/include','combo','combo/menu')]
        for combined in (False,True):
            subprocess.run([*shlex.split(os.environ.get('CC','cc')),*flags,*includes,
                            *(['-DCOMBO_BUILD'] if combined else []),'-I'+str(folder),
                            str(FIXTURE),'-o',str(binary)],check=True,cwd=ROOT)
            result = subprocess.run([str(binary)],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0',
                                                      'UBSAN_OPTIONS':'halt_on_error=1'})
            print('mode='+('combo' if combined else 'original')+' exit='+str(result.returncode),flush=True)
            if result.returncode:
                return result.returncode
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
