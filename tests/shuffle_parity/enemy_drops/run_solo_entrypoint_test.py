#!/usr/bin/env python3
"""Compile complete SOH entrypoint lifecycle; no native graph/fill claim."""
import os,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
with tempfile.TemporaryDirectory(prefix='solo-entry-') as td:
 b=Path(td)/'test';subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-DCOMBO_BUILD','-Wall','-Wextra','-Werror','-I'+str(ROOT),'-I'+str(ROOT/'combo'),str(Path(__file__).with_name('solo_entrypoint_test.cpp')),'-o',str(b)],check=True);subprocess.run([str(b)],check=True)
print('PASS complete SOH solo entrypoint lifecycle/schema; Fill and graph are service boundaries')
