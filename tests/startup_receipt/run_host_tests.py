#!/usr/bin/env python3
"""Execute actual launcher receipt/IO callbacks against atomic save files."""
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
signatures = (
    'static void Combo_PrepareStartupReceiptSeed() {',
    'static std::string ComboReleaseMajorMinor(', 'static nlohmann::json& LoadOrCreateContainer(',
    'static bool WriteContainerDocument(', 'static bool FlushContainer(',
    'static const char* Combo_ReadGameSave(', 'static void Combo_WriteGameSave(',
    'static void Combo_CompleteStartupReceipt(', 'static bool Combo_PreserveStartupMmState(',
    'static int32_t Combo_StartupReceipt(',
    'static void Combo_CopyContainer(',
)
try:
    functions = '\n'.join(function(host, signature) for signature in signatures)
except ValueError:
    raise SystemExit('FAIL launcher startup receipt callbacks are missing')
code = (ROOT / 'tests/startup_receipt/host_test.cpp').read_text().replace('/* HOST_FUNCTIONS */', functions)
flags = ['-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT)]
if '--json-include' in sys.argv:
    flags += ['-I', sys.argv[sys.argv.index('--json-include') + 1]]
if '--sanitize' in sys.argv:
    flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all',
              '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
with tempfile.TemporaryDirectory(prefix='startup-host-') as directory:
    cpp = Path(directory) / 'host.cpp'
    cpp.write_text(code)
    binary = Path(directory) / 'host'
    subprocess.run([*shlex.split(os.environ.get('CXX', 'c++')), *flags, str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary), directory], check=True)
