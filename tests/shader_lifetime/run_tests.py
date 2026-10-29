#!/usr/bin/env python3
"""Compile real XML ownership and shader handlers, then exercise teardown under sanitizers."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import block_from


def configured_imgui_root():
    # Standalone CI groups prepare the same pinned headers without configuring a build tree.
    candidates = [os.environ.get('IMGUI_DIR', '')]
    candidates += os.environ.get('COMBO_NEI_FIXTURE_CPATH', '').split(os.pathsep)
    candidates += os.environ.get('CPATH', '').split(os.pathsep)
    for candidate in candidates:
        if candidate and (Path(candidate) / 'imconfig.h').is_file():
            return Path(candidate)
    for build in ('build-cmake', 'build'):
        cache = ROOT / build / 'CMakeCache.txt'
        if cache.exists():
            for line in cache.read_text().splitlines():
                if line.startswith('FETCHCONTENT_SOURCE_DIR_IMGUI:PATH='):
                    path = Path(line.split('=', 1)[1])
                    if (path / 'imconfig.h').exists():
                        return path
        fetched = ROOT / build / '_deps/imgui-src'
        if (fetched / 'imconfig.h').exists():
            return fetched
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--case', choices=('xml', 'cache', 'invalid'))
    parser.add_argument('--tinyxml2-root', type=Path, default=ROOT / 'ZAPDTR/lib/tinyxml2')
    parser.add_argument('--imgui-root', type=Path, default=configured_imgui_root())
    args = parser.parse_args()
    if args.imgui_root is None or not (args.imgui_root / 'imconfig.h').exists():
        parser.error('supply --imgui-root with the configured engine ImGui dependency')
    interpreter = (ROOT / 'libultraship/src/fast/interpreter.cpp').read_text()
    handlers = '\n'.join(block_from(interpreter, interpreter.index(signature)) for signature in (
        'bool gfx_push_shader(', 'bool gfx_pop_shader(', 'const char* gfx_get_shader('))
    with tempfile.TemporaryDirectory(prefix='shader-lifetime-') as temporary:
        build = Path(temporary)
        (build / 'spdlog').mkdir()
        (build / 'spdlog/spdlog.h').write_text(
            '#pragma once\n#define SPDLOG_ERROR(...) ((void)0)\n#define SPDLOG_TRACE(...) ((void)0)\n')
        cpp = build / 'lifetime.cpp'
        cpp.write_text((ROOT / 'tests/shader_lifetime/lifetime_test.cpp').read_text().replace(
            '/* PRODUCTION_SHADER_HANDLERS */', handlers))
        sources = [str(cpp), *(str(ROOT / path) for path in (
            'libultraship/src/fast/resource/factory/DisplayListFactory.cpp',
            'libultraship/src/fast/resource/type/DisplayList.cpp',
            'libultraship/src/ship/resource/ResourceFactoryXML.cpp',
            'libultraship/src/ship/resource/ResourceFactoryBinary.cpp',
            'libultraship/src/ship/resource/Resource.cpp',
            'libultraship/src/ship/utils/binarytools/BinaryReader.cpp',
            'libultraship/src/ship/utils/binarytools/MemoryStream.cpp',
            'libultraship/src/ship/utils/binarytools/Stream.cpp'))]
        flags = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                 '-fno-omit-frame-pointer', '-fno-pie', '-no-pie'] if args.sanitize else []
        binary = build / 'lifetime'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-DF3DEX_GBI_2',
                        '-include', 'unordered_map', '-ffunction-sections', '-fdata-sections', *flags,
                        '-I' + str(build), '-I' + str(ROOT / 'libultraship/include'),
                        '-I' + str(args.tinyxml2_root), '-I' + str(args.imgui_root), *sources,
                        str(args.tinyxml2_root / 'tinyxml2.cpp'), '-Wl,--gc-sections',
                        '-Wl,--wrap=malloc', '-o', str(binary)], check=True)
        for case in ([args.case] if args.case else ['xml', 'cache', 'invalid']):
            subprocess.run([str(binary), case], check=True,
                           env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})


if __name__ == '__main__':
    main()
