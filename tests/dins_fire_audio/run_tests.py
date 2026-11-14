#!/usr/bin/env python3
"""Exercise production Din audio adapters and native post-synth seams on both hosts."""
import argparse
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ui_visibility'))
from run_control_input_tests import compiler_flags,function

def run(command):
    try:
        result=subprocess.run(list(map(str,command)),cwd=ROOT,text=True,capture_output=True,timeout=120,
                              env={**os.environ,'ASAN_OPTIONS':os.environ.get('ASAN_OPTIONS','detect_leaks=0')})
    except subprocess.TimeoutExpired:
        raise SystemExit('Din audio command exceeded 120 seconds (possible mixer lock/lifetime deadlock)')
    if result.returncode:
        print(result.stdout+result.stderr)
        raise SystemExit(result.returncode)
    if result.stdout:print(result.stdout,end='')

def check_context_sites(host):
    source=(ROOT/host/'src/code/z_play.c').read_text()
    var='this' if host=='mm' else 'play'
    needle='DinsFireSfx_UpdateContext('+var+');'
    body=function(source,'Play_Update')
    gate='if (!sBombersNotebookOpen)' if host=='mm' else 'if ((SREG(1)'
    assert needle in body and body.index(needle)<body.index(gate),'entry publication precedes outer pause/actor gates'
    main=function(source,'Play_UpdateMain') if host=='mm' else body
    mutation=main.index('KaleidoSetup_Update('+var+');')
    pause_gate=main.index('sp5C = IS_PAUSED' if host=='mm' else 'isPaused = (')
    assert needle in main[mutation:pause_gate], 'pause state must be republished after Kaleido mutation before actor gate'
    destroy=function(source,'Play_Destroy')
    assert 'DinsFireSfx_UpdateContext(NULL);' in destroy, 'actual Play_Destroy must synchronously stop context before actor teardown'
    assert destroy.index('DinsFireSfx_UpdateContext(NULL);') < destroy.index('Actor'), 'teardown publication precedes actor destruction'
    assert '#include "../../../combo/audio/DinsFireSfx.h"' in source

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--host',choices=('soh','mm'))
parser.add_argument('--source-only',action='store_true',help='Verify lifecycle publication in actual native Play functions.')
parser.add_argument('--export',type=Path,help='Write stereo production-mixer audition WAV (OoT gain defaults overridden to 100%%).')
parser.add_argument('--sanitize',action='store_true')
parser.add_argument('--dependency-root',type=Path,action='append',default=[],
                    help='Existing native dependency include root (repeatable); no downloads.')
parser.add_argument('--baseline',action='store_true',help='Run behavior tests against the pre-implementation silent bridge.')
args=parser.parse_args()
if not args.baseline:
    for host in ('soh','mm'):
        if not args.host or args.host==host:check_context_sites(host)
    print('PASS actual Play_Update entry, post-Kaleido pause publication and Play_Destroy teardown')
if args.source_only:raise SystemExit(0)
# Use real installed native headers, including configured compiler include roots
# and the adjacent recovery dependency cache. Never generate substitutes/fetch.
candidates=[*args.dependency_root]
# Direct gate tasks receive the prepared CI headers through the same variable
# used by the override runner, without the NEI umbrella's CPATH assignment.
for variable in ('DINS_AUDIO_DEPENDENCY_ROOTS','CPATH','CPLUS_INCLUDE_PATH','COMBO_NEI_FIXTURE_CPATH','NLOHMANN_JSON_INCLUDE_DIR'):
    candidates += [Path(value) for value in os.environ.get(variable,'').split(os.pathsep) if value]
candidates += [ROOT.parent/'dins_audio_native_dependencies',ROOT/'deps',Path('/usr/include'),Path('/usr/local/include')]
candidates += [p.parents[1] for p in sorted(ROOT.parents[1].glob('*/deps/nlohmann/json.hpp'))]
include_roots=list(dict.fromkeys(path.expanduser().resolve() for path in candidates if path.expanduser().is_dir()))
required=('nlohmann/json.hpp','spdlog/async.h','BS_thread_pool.hpp','SDL2/SDL.h')
missing=[header for header in required if not any((path/header).is_file() for path in include_roots)]
if missing and not args.baseline:
    parser.error('Missing real native headers: '+', '.join(missing)+
                 '. Pass --dependency-root (repeatable) or set DINS_AUDIO_DEPENDENCY_ROOTS to existing include roots.')
# Check standard roots above, but leave their ordering to the compiler. Adding
# /usr/include explicitly can put it before libstdc++ and break include_next.
native_includes=[argument for path in include_roots if path not in (Path('/usr/include'),Path('/usr/local/include'))
                 for argument in ('-isystem',str(path))]
with tempfile.TemporaryDirectory(prefix='dins-fire-audio-') as temp:
    temp=Path(temp)
    shared_objects=[]
    if not args.baseline:
        # These sources have no host or Combo conditionals; compile the actual
        # fixture boundary/registry once, then link them into all four adapters.
        common=['-g','-O1','-pthread','-ffunction-sections','-fdata-sections',
                '-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie',
                '-isystem',str(ROOT/'libultraship/include'),*native_includes]
        if args.sanitize:common+=['-fsanitize=address,undefined,float-cast-overflow']
        for source in (ROOT/'tests/dins_fire_audio/archive_fixture.cpp',
                       ROOT/'libultraship/src/ship/resource/CrossRMRegistry.cpp'):
            obj=temp/(source.stem+'.o')
            run([os.environ.get('CXX','c++'),'-std=c++20',*common,'-c',source,'-o',obj])
            shared_objects.append(obj)
    for host in ('soh','mm'):
        if args.host and host!=args.host:continue
        source=ROOT/host/('src/audio/lib/thread.c' if host=='mm' else 'src/code/code_800E4FE0.c')
        (temp/'native_audio_buffer.inc').write_text(function(source.read_text(),'AudioMgr_CreateNextAudioBuffer'))
        adapter=ROOT/host/('2s2h/Enhancements/Audio/DinsFireSfx.cpp' if host=='mm' else 'soh/Enhancements/audio/DinsFireSfx.cpp')
        for combo in (False,True):
            flags=[f for f in compiler_flags(host,combo) if not f.startswith(('-std=','-Wno-','-Werror=','-fsanitize='))]
            if args.sanitize:flags+=['-fsanitize=address,undefined,float-cast-overflow']
            flags=['-I'+str(temp),*flags]
            flags+=native_includes
            flags+=['-pthread','-ffunction-sections','-fdata-sections']
            if host=='mm':flags+=['-DTEST_HOST_MM']
            if args.baseline:flags+=['-DTEST_BASELINE_SILENT_BRIDGE']
            binary=temp/f'{host}-{combo}'
            sources=[ROOT/'tests/dins_fire_audio/runtime_test.cpp']+([] if args.baseline else [adapter,*shared_objects])
            run([os.environ.get('CXX','c++'),'-std=c++20',*flags,*sources,'-Wl,--gc-sections','-o',binary])
            command=[binary]
            if args.export and host==(args.host or 'soh') and combo:
                args.export.parent.mkdir(parents=True,exist_ok=True)
                command+=['--export',args.export.resolve()]
            run(command)
            print(f'PASS host={host} combo={combo}',flush=True)
    print('PASS actual Play_Update publication before all pause/actor gating')
