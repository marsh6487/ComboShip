#!/usr/bin/env python3
"""Exercise the configure-time scroll probe against complete, absent and partial integrations."""
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PATCH = Path('patches/libultraship-custom-tex-scroll-e3cd6591.patch')
FILES = [Path('libultraship')/name for name in (
    'include/fast/lus_gbi.h', 'include/libultraship/libultra/gbi.h',
    'src/fast/interpreter.cpp', 'src/fast/resource/factory/DisplayListFactory.cpp')]


def fixture(directory):
    for name in [PATCH, *FILES]:
        target = directory/name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT/name, target)


def probe(directory, success):
    before = {name: (directory/name).read_bytes() for name in FILES}
    result = subprocess.run(['cmake', '-P', str(ROOT/'CMake/GlobalSettingsInclude.cmake')],
                            cwd=directory, capture_output=True, text=True)
    if (result.returncode == 0) != success:
        raise AssertionError(result.stdout+result.stderr)
    if not success:
        assert before == {name: (directory/name).read_bytes() for name in FILES}, 'Rejected source was mutated'


with tempfile.TemporaryDirectory(prefix='scroll-probe-') as temporary:
    directory = Path(temporary)
    fixture(directory)
    # New cel/fog opcodes adjacent to scroll must not make a complete integration fail.
    probe(directory, True)
    baseline = {name: (directory/name).read_bytes() for name in FILES}
    # Also preserve the original apply-if-absent behavior.
    subprocess.run(['git', 'apply', '--reverse', '--unidiff-zero', '--directory=libultraship', str(PATCH)], cwd=directory, check=True)
    probe(directory, True)
    assert baseline == {name: (directory/name).read_bytes() for name in FILES}
    # A symbol alone cannot make a partial or damaged implementation pass.
    for name, old, new in [
        (FILES[1], '#define G_SCROLL_TEXTURE 0x4b', '#define G_SCROLL_TEXTURE 0x7f'),
        (FILES[1], '#define gsSPScrollTexture', '#define gsSPMissingScrollTexture'),
        (FILES[2], 'const int64_t tick = gfx->mGameTick;', 'const int64_t tick = 0;'),
        (FILES[3], 'g = gsSPScrollTexture(tile, speedX, speedY);', 'g = gsSPScrollTexture(tile, 0, 0);'),
    ]:
        target = directory/name
        content = target.read_text()
        assert old in content
        target.write_text(content.replace(old, new))
        probe(directory, False)
        for original, data in baseline.items():
            (directory/original).write_bytes(data)
print('PASS configure scroll probe: rebased opcodes, absent patch, complete implementation and four partial/damaged cases')
