#!/usr/bin/env python3
"""Public CI gate for every soul identity, both receiving hosts and failure isolation.

Private native/TP archive checking is deliberately separate: see README.md. This
gate requires no game assets. --sanitize instruments the complete shared renderer.
"""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
tests = [
    'tests/soul_mini_models/run_settings_tests.py',
    'tests/soul_mini_models/run_accent_tests.py',
    'tests/soul_mini_models/run_renderer_cache_tests.py',
    'tests/soul_mini/run_oot_catalog_tests.py',
    'tests/soul_mini_mm/catalog_test.py',
    'tests/soul_mini_mm/run_recipe_tests.py',
    'tests/soul_mini_models/resource_validator_test.py',
    'tests/mm_texanim_validation/run_tests.py',
    'tests/soul_mini_models/run_renderer_owner_bridge_tests.py',
    'tests/soul_mini_models/run_renderer_recording_tests.py',
    'tests/soul_mini_models/run_renderer_tests.py',
]
for test in tests:
    command = [sys.executable, '-B', str(ROOT / test)]
    if test.endswith('run_renderer_tests.py') and '--sanitize' in sys.argv:
        command.append('--sanitize')
    print('RUN', test, flush=True)
    subprocess.run(command, cwd=ROOT, check=True)
print('PASS: soul miniature public regression gate')
