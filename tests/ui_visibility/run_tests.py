#!/usr/bin/env python3
"""Run the shared, OoT and MM UI visibility regression boundaries."""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]

subprocess.run([sys.executable, "-B", str(ROOT / "tests/ui_visibility/run_control_input_tests.py")],
               cwd=ROOT, check=True)

for host in ("gui", "combo", "oot", "mm"):
    subprocess.run([sys.executable, "-B", str(ROOT / "tests/ui_visibility" / (host + "_run_tests.py"))],
                   cwd=ROOT, check=True)

subprocess.run([sys.executable, "-B", str(ROOT / "tests/ui_visibility/gui_mm_sm64_lifetime.py")],
               cwd=ROOT, check=True)
subprocess.run([sys.executable, "-B", str(ROOT / "tests/ui_visibility/gui_harpoon_votes.py")],
               cwd=ROOT, check=True)
