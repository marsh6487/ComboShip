#!/usr/bin/env python3
"""Compile the real multi-image room functions against native OoT declarations.

Run from any directory. Both standalone and COMBO_BUILD configurations are
checked by default. --output-dir preserves diagnostics; otherwise all generated
code, compiler adapters, and binaries are temporary. Requires a POSIX C host.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
CASES = (
    'fixed-remap', 'fixed-negative-remap', 'fixed-null-source',
    'door-hook-veto-control', 'door-protected-camera-data',
    'free-protected-camera-data', 'door-protected-image-list', 'fixed-hook-veto',
    'door-hook-restores-fixed', 'fixed-hook-switches-door',
)


def extract_function(path, name):
    text = path.read_text()
    pattern = (r'^(?:static\s+)?(?:[A-Za-z_][A-Za-z_0-9]*\s*\*?\s+)+'
               + re.escape(name) + r'\([^;{}]*\)\s*\{')
    match = re.search(pattern, text, re.M)
    if match is None:
        raise ValueError(f'Function {name} not found in {path}')
    end, depth = match.end(), 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    fragment = text[match.start():end]
    line = text.count('\n', 0, match.start()) + 1
    return fragment, {'name': name, 'source': str(path), 'line': line,
                      'sha256': hashlib.sha256(fragment.encode()).hexdigest()}


def run(args, output):
    root = args.source_root.resolve()
    headers = (args.headers_root or root).resolve()
    room = (args.room_source or root / 'soh/src/code/z_room.c').resolve()
    collision = root / 'soh/src/code/z_bgcheck.c'
    output.mkdir(parents=True, exist_ok=True)
    fragments, extracted = [], []
    for path, names in (
        (collision, ('DynaPoly_IsBgIdBgActor', 'DynaPoly_IsBgIdInTable',
                     'BgCheck_GetCollisionHeader', 'BgCheck_GetBgCamFuncDataImpl')),
        (room, ('func_80096A74', 'func_80096B6C')),
    ):
        for name in names:
            fragment, metadata = extract_function(path, name)
            fragments.append(f'#line {metadata["line"]} {json.dumps(str(path))}\n{fragment}\n')
            extracted.append(metadata)
    prefix, checks = HERE / 'fixture_prefix.c', HERE / 'fixture_checks.c'
    code = f'#line 1 {json.dumps(str(prefix))}\n' + prefix.read_text()
    code += '\n' + '\n'.join(fragments)
    code += f'\n#line 1 {json.dumps(str(checks))}\n' + checks.read_text()
    generated = output / 'native_room_test.c'
    generated.write_text(code)

    adapter_path = headers / 'scripts/ci/native_test_abi.py'
    spec = importlib.util.spec_from_file_location('room_native_test_abi', adapter_path)
    adapter = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(adapter)
    common_flags = [
        '-std=gnu11', '-g', '-O1', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie',
        '-DF3DEX_GBI_2', '-DCONTROLLERBUTTONS_T=uint32_t',
        '-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"', '-DCVAR_PREFIX_SETTING="gSettings"',
        '-w', '-Werror=implicit-function-declaration',
    ]
    if args.sanitize:
        common_flags += ['-fsanitize=address,undefined']
    includes = ['-I' + str(HERE / 'include')]
    includes += ['-I' + str(headers / path) for path in (
        'soh', 'soh/include', 'soh/include/PR', 'soh/src', 'soh/assets',
        'soh/soh', 'libultraship/include', 'combo', 'combo/menu')]
    report = {
        'room_source': str(room),
        'room_source_sha256': hashlib.sha256(room.read_bytes()).hexdigest(),
        'collision_source_sha256': hashlib.sha256(collision.read_bytes()).hexdigest(),
        'sanitizers': args.sanitize, 'abi_flags': list(adapter.ABI_FLAGS),
        'functions_extracted_verbatim': extracted,
        'fixture_boundaries': ['hook dispatch', 'graphics setup services',
                               'background rendering', 'skybox offsets',
                               'graph/interpolation bookkeeping', 'diagnostic output'],
        'actual_packet_builders': True, 'configurations': [],
    }
    failed = 0
    with adapter.NativeTestCompilers(os.environ) as compilers:
        for mode in args.mode or ('solo', 'combined'):
            directory = output / mode
            directory.mkdir(exist_ok=True)
            binary = directory / 'native_room_test'
            flags = common_flags + (['-DCOMBO_BUILD'] if mode == 'combined' else [])
            env = compilers.environment_for_task({'native_abi': 'combined'})
            command = [env.get('CC', 'cc'), *flags, *includes, str(generated), '-o', str(binary)]
            compiled = subprocess.run(command, capture_output=True, text=True, env=env)
            (directory / 'compile.stdout.txt').write_text(compiled.stdout)
            (directory / 'compile.stderr.txt').write_text(compiled.stderr)
            result = {'mode': mode, 'compile_returncode': compiled.returncode, 'cases': []}
            report['configurations'].append(result)
            if compiled.returncode:
                sys.stderr.write(compiled.stderr)
                failed += 1
                continue
            env.update(ASAN_OPTIONS='detect_leaks=0:abort_on_error=0',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
            layout = subprocess.run([str(binary), 'layout'], capture_output=True,
                                    text=True, env=env, check=True)
            result['native_abi_layout'] = json.loads(layout.stdout)
            for case in args.case or CASES:
                executed = subprocess.run([str(binary), case], capture_output=True, text=True, env=env)
                (directory / f'{case}.stdout.txt').write_text(executed.stdout)
                (directory / f'{case}.stderr.txt').write_text(executed.stderr)
                passed = executed.returncode == 0
                result['cases'].append({'name': case, 'returncode': executed.returncode, 'passed': passed})
                print(f'{"PASS" if passed else "FAIL"} {mode}/{case}')
                if not passed:
                    failed += 1
                    for line in executed.stderr.splitlines():
                        if any(token in line for token in ('ERROR:', 'runtime error:', '#0 ', '#1 ', 'SUMMARY:')):
                            print('  ' + line)
    report['failed'] = failed
    (output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'{"FAIL" if failed else "PASS"}: {failed} failures across native room configurations')
    return int(failed != 0)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=REPO)
    parser.add_argument('--headers-root', type=Path)
    parser.add_argument('--room-source', type=Path)
    parser.add_argument('--output-dir', type=Path)
    parser.add_argument('--mode', action='append', choices=('solo', 'combined'))
    parser.add_argument('--case', action='append', choices=CASES)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    if args.output_dir:
        return run(args, args.output_dir.resolve())
    with tempfile.TemporaryDirectory(prefix='oot-room-background-') as directory:
        return run(args, Path(directory))


if __name__ == '__main__':
    raise SystemExit(main())
