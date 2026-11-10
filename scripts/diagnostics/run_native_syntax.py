#!/usr/bin/env python3
"""Check full production units with real headers and native CMake definitions.

This is syntax/guard coverage, not a linked build or runtime proof. Dependencies
must already exist via CPATH or --toolkit. No generated stand-in headers or new
warning downgrades are supplied.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--game', choices=('soh', 'mm'), required=True)
    parser.add_argument('--mode', choices=('original', 'combo'), default='combo')
    parser.add_argument('--toolkit', type=Path, help='Optional pinned real dependency header directory')
    parser.add_argument('sources', nargs='+')
    args = parser.parse_args()
    root = args.repo.resolve()
    host = args.game
    common = ['-DLOG_LEVEL_GAME_PRINTS=6', '-DSPDLOG_ACTIVE_LEVEL=0', '-DIMGUI_DEFINE_MATH_OPERATORS',
              '-DImTextureID=void*', '-DF3DEX_GBI_2', '-DGBI_FLOAT_MTX', '-DGBI_S32_VTX',
              '-DCONTROLLERBUTTONS_T=uint32_t', '-D' + ('MM_BUILD_DLL' if host == 'mm' else 'SOH_BUILD_DLL')]
    if args.mode == 'combo':
        common.append('-DCOMBO_BUILD')
    settings = dict(re.findall(r'set\((CVAR_PREFIX_\w+) "([^"]+)"\)',
                               (root / 'CMake/soh-cvars.cmake').read_text()))
    for name, value in settings.items():
        common.append('-D' + name + '="' + value + '"')
    for name, value in re.findall(r'set\((CVAR_\w+) "([^"]+)"\)',
                                  (root / 'CMake/lus-cvars.cmake').read_text()):
        for key, resolved in settings.items():
            value = value.replace('${' + key + '}', resolved)
        common.append('-D' + name + '="' + value + '"')
    paths = ('.', host + '/include', host + '/include/PR', host + '/src', host, host + '/assets',
             host + ('/soh' if host == 'soh' else '/2s2h'), host + '/mods', 'libultraship/include',
             'libultraship/src', 'combo', 'combo/menu', 'libultraship/extern')
    common.extend('-I' + str(root / path) for path in paths)
    if args.toolkit:
        toolkit = args.toolkit.resolve()
        dependencies = (toolkit, toolkit / 'SDL2', toolkit / 'libzip-headers', toolkit / 'imgui-1.91.9b-docking',
                        toolkit / 'spdlog-1.16.0/include', toolkit.parent / 'recovered-tooling/deps',
                        toolkit.parent / 'recovered-tooling/SDL_net-2.2.0/include')
        common.extend('-I' + str(path) for path in dependencies if path.is_dir())
    common.extend(('-fmax-errors=5', '-fsyntax-only'))
    cmake = (root / host / 'CMakeLists.txt').read_text()
    pch = {'CXX': [], 'C': []}
    marker = 'target_precompile_headers(${PROJECT_NAME} PRIVATE'
    if marker in cmake:
        block = cmake[cmake.index(marker):]
        block = block[:block.index('\n)')]
        for line in block.splitlines():
            found = re.search(r'"\$<\$<COMPILE_LANGUAGE:(CXX|C)>:(.*)>"', line)
            if found:
                value = found[2].replace('${CMAKE_CURRENT_SOURCE_DIR}', str(root / host))
                value = value.replace('$<ANGLE-R>', '>').strip('<>')
                pch[found[1]].extend(('-include', value))
    for path in args.sources:
        cpp = Path(path).suffix.lower() in ('.cpp', '.cc', '.cxx')
        # Match the repository's existing GNU target policy and C resource-handle
        # allowances, including when checking the original preprocessor guards.
        if cpp:
            flags = ['-std=c++20', *common, '-fpermissive', *pch['CXX']]
        else:
            flags = ['-std=gnu17' if host == 'mm' else '-std=gnu2x', *common,
                     '-Wno-incompatible-pointer-types', '-Wno-int-conversion', *pch['C']]
        compiler = os.environ.get('CXX' if cpp else 'CC', 'g++-14' if cpp else 'gcc-14')
        subprocess.run([compiler, *flags, str(root / path)], check=True, cwd=root)
        print('PASS', host, args.mode, path, flush=True)


if __name__ == '__main__':
    main()
