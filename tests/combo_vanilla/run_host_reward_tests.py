#!/usr/bin/env python3
"""Run the actual launcher transaction/IO callbacks against real atomic files."""
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
def function(source, signature):
    start = source.index(signature)
    begin = source.index('{', start)
    depth, end = 1, begin + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

host = (ROOT / 'combo/ComboShip.cpp').read_text()
functions = '\n'.join(function(host, signature) for signature in (
    'static std::string ComboReleaseMajorMinor(', 'static nlohmann::json& LoadOrCreateContainer(',
    'static bool WriteContainerDocument(', 'static bool FlushContainer(',
    'static const char* Combo_ReadGameSave(', 'static void Combo_WriteGameSave(',
    'static int Combo_VanillaReward(', 'static void Combo_CopyContainer(',
    'static int Combo_OnFinalBossDefeated('))
code = (ROOT / 'tests/combo_vanilla/host_reward_test.cpp').read_text().replace('/* HOST_FUNCTIONS */', functions)
flags = ['-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT)]
if '--sanitize' in sys.argv:
    flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all',
              '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
with tempfile.TemporaryDirectory(prefix='vanilla-host-reward-') as directory:
    cpp = Path(directory) / 'host.cpp'; cpp.write_text(code)
    binary = Path(directory) / 'host'
    subprocess.run([*shlex.split(os.environ.get('CXX', 'c++')), *flags, str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary), directory], check=True)
