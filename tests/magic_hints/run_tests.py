#!/usr/bin/env python3
"""Compile the real MM hint policy with native types and original/Combo guards."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'tests/ui_visibility'))
from run_control_input_tests import compiler_flags, function
with tempfile.TemporaryDirectory(prefix='magic-hints-') as temporary:
    source = (ROOT/'mm/2s2h/Rando/MiscBehavior/SariasSongHint.cpp').read_text()
    (Path(temporary)/'candidates.inc').write_text(function(source, 'IsExcludedFromSariaPriorityItemCandidates') + function(source, 'Rando::GetSariaPriorityItemCandidates'))
    (Path(temporary)/'placement.inc').write_text(function((ROOT/'mm/2s2h/Rando/Rando.cpp').read_text(), 'Rando::FindMultiItemPlacement'))
    start = source.index('                RandoItemId rawItem =')
    end = source.index('                    otherChecks.push_back(randoCheckId);', start)
    end = source.index('}', end) + 1
    (Path(temporary)/'selection.inc').write_text(source[start:end])
    for combo in (False, True):
        flags = [s for s in compiler_flags('mm', combo) if not s.startswith('-Wno-')
                 and not s.startswith('-std=') and s != '-Werror=implicit-function-declaration']
        binary = str(Path(temporary)/'policy')
        subprocess.run(['c++', '-std=c++20', '-I'+temporary, *flags, str(ROOT/'tests/magic_hints/policy_test.cpp'),
                        '-o', binary], check=True)
        subprocess.run([binary], check=True, env={**os.environ, 'ASAN_OPTIONS':'detect_leaks=0'})
        print('PASS actual MM magic hint policy', 'Combo' if combo else 'original', flush=True)
