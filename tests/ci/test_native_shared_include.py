"""Compile shared contracts using the native targets' actual include blocks."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


def native_include_block(game):
    source = (ROOT / game / 'CMakeLists.txt').read_text()
    start = source.index('target_include_directories(${PROJECT_NAME} PRIVATE')
    opening = source.index('(', start)
    end, depth = opening + 1, 1
    while depth:
        depth += (source[end] == '(') - (source[end] == ')')
        end += 1
    return source[start:end]


class NativeSharedHeaderTests(unittest.TestCase):
    def test_actual_native_include_contracts_compile_shared_headers(self):
        for game in ('soh', 'mm'):
            with self.subTest(game=game), tempfile.TemporaryDirectory(prefix='native-shared-include-') as directory:
                work = Path(directory)
                (work / 'probe.cpp').write_text(
                    '#include "combo/rando/SongEvents.h"\n'
                    'static_assert(ComboRando::SongEvents::OOT_TEMPLE_GANON == 17);\n'
                    'int main() { return 0; }\n')
                (work / 'CMakeLists.txt').write_text(
                    'cmake_minimum_required(VERSION 3.20)\n'
                    'project(NativeSharedInclude LANGUAGES CXX)\n'
                    'set(CMAKE_CXX_STANDARD 20)\n'
                    'add_executable(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/probe.cpp")\n'
                    f'set(CMAKE_SOURCE_DIR "{ROOT.as_posix()}")\n'
                    f'set(CMAKE_CURRENT_SOURCE_DIR "{(ROOT / game).as_posix()}")\n'
                    + native_include_block(game) + '\n')
                env = dict(os.environ)
                # The native target must provide this path itself. A developer's
                # ambient fixture CPATH must not hide a missing CMake include.
                for key in ('CPATH', 'CPLUS_INCLUDE_PATH', 'C_INCLUDE_PATH'):
                    env.pop(key, None)
                command = ['cmake', '-S', str(work), '-B', str(work / 'build')]
                if shutil.which('ninja'):
                    command += ['-G', 'Ninja']
                for invocation in (command, ['cmake', '--build', str(work / 'build')]):
                    result = subprocess.run(invocation, env=env, text=True, capture_output=True)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == '__main__':
    unittest.main()
