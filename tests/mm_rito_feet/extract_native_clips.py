#!/usr/bin/env python3
"""Rebuild only the pinned MM player-frame fixture, leaving the source archive read only."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

ARCHIVE_SHA256 = 'f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59'
CLIPS = ('normal_wait', 'normal_wait_free', 'normal_walk', 'normal_walk_free', 'fighter_walk_long',
         'normal_run', 'normal_run_free', 'fighter_run', 'fighter_run_long')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archive', type=Path)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    original = args.archive.read_bytes()
    assert hashlib.sha256(original).hexdigest() == ARCHIVE_SHA256
    fixture_path = Path(__file__).with_name('native_gait_fixture.json')
    fixture = json.loads(fixture_path.read_text())
    clips = {}
    with zipfile.ZipFile(args.archive, 'r') as archive:
        for clip in CLIPS:
            header_path = 'objects/gameplay_keep/gPlayerAnim_link_' + clip
            payload_path = 'misc/link_animetion/gPlayerAnim_link_' + clip + '_Data'
            header, payload = archive.read(header_path), archive.read(payload_path)
            count, = struct.unpack_from('<I', payload, 64)
            assert count % 67 == 0 and len(payload) == 68 + count * 2
            header_count, = struct.unpack_from('<h', header, 68)
            assert header_count == count // 67
            frames = [list(struct.unpack_from('<67h', payload, 68 + frame * 134)) for frame in range(count // 67)]
            record = {'resource': payload_path, 'resource_sha256': hashlib.sha256(payload).hexdigest(),
                      'header_resource': header_path, 'header_sha256': hashlib.sha256(header).hexdigest(),
                      'frames': frames, 'packed_frames_sha256': hashlib.sha256(payload[68:]).hexdigest()}
            if clip in fixture['clips']:
                assert fixture['clips'][clip]['packed_frames_sha256'] == record['packed_frames_sha256']
            clips[clip] = record
    fixture['clips'] = clips
    rebuilt = json.dumps(fixture, separators=(',', ':')) + '\n'
    if args.verify:
        assert fixture_path.read_text() == rebuilt, 'fixture differs from deterministic source extraction'
    else:
        fixture_path.write_text(rebuilt)
    assert hashlib.sha256(args.archive.read_bytes()).hexdigest() == ARCHIVE_SHA256
    print('PASS nine exact native idle/gait clips, read-only source archive SHA256 unchanged')


if __name__ == '__main__':
    main()
