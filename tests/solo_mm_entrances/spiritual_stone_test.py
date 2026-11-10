#!/usr/bin/env python3
"""Execute actual stone summon/recall functions with the native Solo adapter."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent


def block(text, marker):
    start = text.index(marker)
    opening = text.index("{", start)
    depth, end = 1, opening + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


source = (ROOT / "mm/mods/spiritual_stones/spiritual_stones.cpp").read_text()
native = "\n".join(block(source, marker) for marker in (
    "void ExecuteWarp(PlayState* play, int stone)",
    "void SummonStatueHere(PlayState* play, Player* player, int stone)"))
with tempfile.TemporaryDirectory(prefix="solo-mm-stones-") as directory:
    fixture = Path(directory) / "test.cpp"
    fixture.write_text(Path(__file__).with_suffix(".cpp").read_text().replace("/* NATIVE_STONE_FUNCTIONS */", native))
    env = dict(os.environ, COMBO_SOLO_MM_RUNTIME_TEST_SOURCE=str(fixture))
    subprocess.run([sys.executable, "-B", str(HERE / "runtime_test.py")], env=env, check=True, cwd=ROOT)
