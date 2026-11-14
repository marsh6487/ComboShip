#!/usr/bin/env python3
"""Compile unchanged production flight/dispatch/floor functions against real MM headers.

Engine resource, animation playback and effect boundaries are instrumented by the
fixture; this does not link the game or prove rendering. --ref tests a git parent.
"""
from pathlib import Path
import argparse
import os
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_scene_randomization_tests import function


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ref')
    parser.add_argument('--case', choices=('all', 'flight', 'reset'), default='all')
    parser.add_argument('--mode', choices=('all', 'original', 'combo'), default='all')
    args = parser.parse_args()

    def source(path):
        if args.ref:
            return subprocess.check_output(['git', 'show', f'{args.ref}:{path}'], cwd=ROOT, text=True)
        return (ROOT / path).read_text()

    flight = source('mm/mods/forms/rito_flight.inc.c')
    form = source('mm/mods/forms/form_rito.cpp')
    common = source('mm/mods/forms/custom_forms.cpp')
    actor = source('mm/src/code/z_actor.c')
    state = flight[flight.index('#define RITO_CHARGE_MIN'):flight.index('static struct {')]
    snippets = [state, function(common, 'Forms_OnGround'), function(common, 'Forms_CanAct'),
                function(actor, 'func_800B761C'), function(actor, 'func_800B7678'),
                function(form, 'MmRitoInputBlocked'), function(flight, 'MmRitoPlay'),
                function(flight, 'MmRitoOotTookOver'), function(flight, 'MmRitoAdvance')]
    if 'static void MmRitoClearFlightState(' in flight:
        snippets.append(function(flight, 'MmRitoClearFlightState'))
    snippets += [function(flight, name) for name in ('MmRitoEnterAir', 'MmRitoRelease',
                 'MmRitoSpendLaunchMagic', 'MmRitoGlideMove', 'MmRitoFlightUpdate', 'MmRitoTryHop')]
    snippets += [function(form, name) for name in ('RitoForm_ForgetContext', 'RitoForm_SyncContext',
                 'RitoForm_Reset', 'RitoForm_FilterInput', 'RitoForm_Update', 'RitoForm_OwnsAction')]
    with tempfile.TemporaryDirectory(prefix='mm-rito-flight-') as temp:
        build = Path(temp)
        (build / 'production.inc').write_text('\n'.join(snippets))
        includes = ('mm', 'mm/include', 'mm/include/PR', 'mm/src', 'mm/assets',
                    'mm/2s2h', 'libultraship/include', 'combo', 'combo/menu')
        for mode in (('original', 'combo') if args.mode == 'all' else (args.mode,)):
            executable = build / mode
            flags = ['-std=c++20', '-Wall', '-Wextra', '-Wno-unused-parameter',
                     '-Wno-unused-variable', '-Wno-missing-field-initializers',
                     '-DF3DEX_GBI_2', '-DGBI_FLOAT_MTX', '-DGBI_S32_VTX',
                     '-DCONTROLLERBUTTONS_T=uint32_t', '-DMM_BUILD_DLL']
            if mode == 'combo':
                flags.append('-DCOMBO_BUILD')
            subprocess.run([*shlex.split(os.environ.get('CXX', 'c++')), *flags,
                            '-I' + str(build), *['-I' + str(ROOT / p) for p in includes],
                            str(ROOT / 'tests/mm_rito_flight/native_test.cpp'),
                            '-o', str(executable)], check=True)
            subprocess.run([str(executable), args.case], check=True)
            print('PASS native MM Rito flight', mode, args.ref or 'candidate', flush=True)


if __name__ == '__main__':
    main()
