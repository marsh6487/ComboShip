#!/usr/bin/env python3
"""Verify real audio guard and additive emitters without changing accepted Chamber resources."""
import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/temple_sages_bundle'))
sys.path.insert(0, str(ROOT / 'tools/temple_sages'))
import temple_sages_pool
import resource_geometry as geometry


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    actor = ROOT / 'soh/src/overlays/actors/ovl_En_River_Sound'
    source = (actor / 'z_en_river_sound.c').read_text()
    definition = re.search(r'void EnRiverSound_Draw\([^;{}]*\)\s*\{', source)
    assert definition is not None
    guard = source[definition.end():source.index('    static s16 soundEffects[]', definition.end())]
    with tempfile.TemporaryDirectory(prefix='chamber-sfx-') as temporary:
        path, binary = Path(temporary) / 'test.c', Path(temporary) / 'test'
        fixture = (Path(__file__).parent / 'chamber_sfx_scope_test.c').read_text()
        path.write_text(fixture.replace('/* PRODUCTION GUARD */', guard))
        flags = ['-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter', '-I' + str(actor)]
        if args.sanitize:
            flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie']
        subprocess.run([os.environ.get('CC', 'cc'), *flags, str(path), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
    directory = ROOT / 'tools/temple_sages_bundle'
    manifest = json.loads((directory / 'manifest.json').read_text())
    resources = temple_sages_pool.decode(directory, manifest['bundle'])
    targets = {p for p in resources if p.endswith(('tokinoma_room_0', 'tokinoma_room_0ZeldaAdult'))}
    assert len(targets) == 4 and len(resources) == 4096
    emitter = (0x3b, 0, -120, -2400, 0, 0, 0, 0x10)
    for path in targets:
        data = resources[path]
        actors = [info['actors'] for command, _, _, info in geometry.room_commands(data) if command == 1]
        assert len(actors) == 1
        records = [struct.unpack_from('<8h', actors[0], i) for i in range(0, len(actors[0]), 16)]
        assert len(records) == 14 and records.count(emitter) == 1
        expected = ('127a9af4b2c8123219da9d5ae34134e30adf9051eb8c9bbc6dca03f748397fc8'
                    if path.endswith('ZeldaAdult') else
                    'b076249094a9a52a6a97953c3d174b620834056cf6232a3c42264347b7974825')
        assert hashlib.sha256(data).hexdigest() == expected
        if not path.endswith('ZeldaAdult'):
            assert (42, 0, -110, -2400, 0, 0, 0, 0x7d23) in records
    protected = {k: hashlib.sha256(v).hexdigest() for k, v in resources.items() if k not in targets}
    digest = hashlib.sha256(json.dumps(protected, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    assert digest == '37e8c132a1f314add4be095a9c8e1fb83f48f05e6389aac0f3681c673c813d6f'
    print('PASS production Chamber guard, four additive emitters, current child Zelda and4092 protected resource bytes')


if __name__ == '__main__':
    main()
