#!/usr/bin/env python3
"""Focused MM UI behavior and native compilation checks; no game runtime claim."""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
for name in ['mm_input_test.py','mm_draw_test.py','mm_interface_test.py','mm_buttons_test.py','mm_shops_test.py','mm_menu_test.py',
             'mm_autosave_test.py',
             'mm_title_card_test.py',
             'mm_daytelop_test.py',
             'mm_diagnostics_test.py',
             'mm_native_syntax.py']:
    subprocess.run([sys.executable,'-B',str(Path(__file__).with_name(name))],cwd=ROOT,check=True)
