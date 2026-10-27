"""Verify every published Windows resource against pinned source or baseline bytes.

Existing resources must remain identical except the six source-defined palette
and Mirror-icon replacements. Every new path must exactly match the verified
chamber, sand or Mirror source pools. This is package proof, not gameplay proof.
"""
import argparse
import hashlib
import importlib.util
import io
import json
from pathlib import Path, PurePosixPath
import stat
import struct
import subprocess
import sys
import tempfile
import zipfile

SOURCE = '0cd35e4d25f853dc68995dd88e8b1c177840e301'
TREE = '30c2a6d0ccce1587866df567560d48e9a3baf967'
BASE_SHA = 'eec89cc0ef6dde94005f58d1e9e5a977469857a7666dac604dbbf02fea915c2e'
GRAY_SHA = '47b23d881d69fe07b9a776ed45f3ea219d1e6a4340de947d8aa186a1029268f5'
GRAY = ['custom/skiejer_market/room_magic_v1/texture/arcane_stone',
        'custom/skiejer_market/texture/arcane_stone']
ICON = 'textures/icon_item_custom/gItemIconMagicMirrorTex'
COLLISION = 'scenes/shared/tokinoma_scene/tokinoma_sceneCollisionHeader_0032F8'
PORTS = {'soh': 'soh.o2r', 'mm': '2ship.o2r'}
COUNTS = {'soh': 11591, 'mm': 7124}
IMAGES = {'Fleet.exe': False, 'soh.dll': True, '2ship.dll': True,
          'comboui.dll': True, 'libultraship.dll': True}

def digest(data):
    return hashlib.sha256(data).hexdigest()

def file_digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as f:
        for b in iter(lambda: f.read(1024 * 1024), b''):
            h.update(b)
    return h.hexdigest()

def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    sys.modules[name] = result
    spec.loader.exec_module(result)
    return result

def members(z):
    result = {}
    folded = set()
    for info in z.infolist():
        n = info.filename.replace('\\', '/')
        if info.is_dir():
            continue
        assert n and not n.startswith('/') and ':' not in n
        assert not any(p in ('.', '..') for p in n.split('/'))
        assert n.casefold() not in folded and n not in result
        assert not stat.S_ISLNK(info.external_attr >> 16)
        result[n] = info
        folded.add(n.casefold())
    return result

def index(z, names):
    result = {}
    for n in sorted(names):
        h = hashlib.sha256()
        count = 0
        with z.open(names[n]) as f:
            for b in iter(lambda: f.read(1024 * 1024), b''):
                h.update(b)
                count += len(b)
        assert count == names[n].file_size
        result[n] = {'bytes': count, 'sha256': h.hexdigest()}
    return result

def pe(z, entry, dll):
    with z.open(entry) as f:
        dos = f.read(64)
        assert dos[:2] == b'MZ'
        offset = struct.unpack_from('<I', dos, 60)[0]
        assert 64 <= offset <= entry.file_size - 26
        f.seek(offset)
        header = f.read(26)
    assert header[:4] == b'PE\0\0'
    machine, sections = struct.unpack_from('<HH', header, 4)
    optional, flags, magic = struct.unpack_from('<HHH', header, 20)
    assert machine == 0x8664 and magic == 0x20b and sections and optional >= 112
    assert bool(flags & 0x2000) == dll
    return {'machine': 'AMD64 0x8664', 'optional_header': 'PE32+ 0x20b',
            'dll': dll, 'bytes': entry.file_size, 'sections': sections}

class Package:
    def __init__(self, path, directory):
        directory.mkdir(parents=True)
        self.outer = zipfile.ZipFile(path)
        self.names = members(self.outer)
        self.index = index(self.outer, self.names)
        self.paths = {}
        for basename in [*IMAGES, *PORTS.values()]:
            matches = [n for n in self.names if PurePosixPath(n).name.casefold() == basename.casefold()]
            assert len(matches) == 1, (basename, matches)
            self.paths[basename] = matches[0]
        assert len({str(PurePosixPath(n).parent) for n in self.paths.values()}) == 1
        extra = [n for n in self.names if n.lower().endswith(('.otr', '.o2r')) and n not in self.paths.values()]
        assert not extra, extra
        self.images = {n: pe(self.outer, self.names[self.paths[n]], dll) for n, dll in IMAGES.items()}
        self.zips, self.indexes, self.port_paths = {}, {}, {}
        for game, name in PORTS.items():
            out = directory / name
            with self.outer.open(self.names[self.paths[name]]) as incoming, out.open('wb') as f:
                for b in iter(lambda: incoming.read(1024 * 1024), b''):
                    f.write(b)
            z = zipfile.ZipFile(out)
            self.zips[game] = z
            self.indexes[game] = index(z, members(z))
            self.port_paths[game] = out

    def close(self):
        for z in getattr(self, 'zips', {}).values():
            z.close()
        self.outer.close()

def png_expected(data, game, fmt):
    from PIL import Image
    with Image.open(io.BytesIO(data)) as source:
        old_mode = source.mode
        image = source.convert('RGBA')
        width, height = image.size
        pixels = bytearray(image.tobytes())
    # The real legacy MM exporter defaults absent RGB alpha to zero.
    if game == 'mm' and old_mode == 'RGB':
        pixels[3::4] = bytes(width * height)
    if fmt == 'rgba32':
        return (1, width, height, bytes(pixels))
    assert fmt == 'rgb5a1'
    packed = bytearray()
    for p in range(width * height):
        r, g, b, a = pixels[p * 4:p * 4 + 4]
        packed.extend(struct.pack('>H', ((r >> 3) << 11) | ((g >> 3) << 6) | ((b >> 3) << 1) | bool(a)))
    return (2, width, height, bytes(packed))

def png_matches(data, expected):
    if len(data) < 80 or data[4:8] != b'XETO' or struct.unpack_from('<I', data, 8)[0] != 0:
        return False
    kind, width, height, payload = expected
    actual = struct.unpack_from('<IIII', data, 64)
    return actual == (kind, width, height, len(payload)) and len(data) == 80 + len(payload) and data[80:] == payload

def source_resources(repo):
    chamber_loader = module('package_chamber', repo / 'scripts/assets/prepare_temple_sages.py')
    chamber = chamber_loader.verified_resources()
    assert len(chamber) == 4096
    metadata = json.loads(chamber_loader.DEFAULT_MANIFEST.read_text())
    assert metadata.get('external_mod_dependencies') == []
    sand = module('package_sand', repo / 'scripts/assets/prepare_lake_tunnel_sand.py').verified_resources()
    cumulative = module('package_cumulative', repo / 'scripts/assets/prepare_cumulative_assets.py').verified_files()
    direct = {game: dict(chamber) for game in PORTS}
    direct['soh'].update(sand)
    pngs = {game: {} for game in PORTS}
    for name, data in cumulative.items():
        for game in PORTS:
            prefix = game + '/assets/custom/'
            if not name.startswith(prefix):
                continue
            relative = name[len(prefix):]
            if relative.endswith('.png'):
                stem, fmt, _ = relative.rsplit('.', 2)
                assert fmt in ('rgba32', 'rgb5a1')
                pngs[game][stem] = png_expected(data, game, fmt)
            else:
                direct[game][relative] = data
    assert all(len(pngs[g]) == 2 for g in PORTS)
    assert len(direct['soh']) == 4100 and len(direct['mm']) == 4098
    return direct, pngs, chamber, metadata

def check_chamber(repo, z, entries, chamber, metadata):
    missing = sorted(set(chamber) - set(entries))
    different = [n for n, b in chamber.items() if n in entries and entries[n] != {'bytes': len(b), 'sha256': digest(b)}]
    retired = sorted(set(metadata.get('retired_resources', [])) & set(entries))
    readiness_missing = []
    for prefix in ('', 'alt/'):
        for route in ('forest', 'fire', 'water', 'spirit', 'shadow'):
            for suffix in ('effect', 'material', 'texture_flow', 'texture_detail', 'geometry', 'geometry_vtx'):
                n = prefix + 'custom/temple_sages/portals/' + route + '/' + suffix
                if n not in entries:
                    readiness_missing.append(n)
    audio = module('package_acoustics', repo / 'tools/temple_sages_bundle/collision_audio.py')
    acoustics = {}
    for prefix in ('', 'alt/'):
        n = prefix + COLLISION
        if n in entries:
            data = z.read(n)
            a = audio.inspect(data)
            a['pass'] = (a['walkable_polygons'] == 229 and audio.apply(data) == data and
                         all(t['sound'] == 2 and t['echo'] == 63 for t in a['floor_types']))
            acoustics[n] = a
    return {'expected_resources': 4096, 'missing': missing, 'different': different,
            'retired_resources_present': retired, 'external_mod_dependencies': [],
            'portal_readiness': {'required_entries': 60, 'missing': readiness_missing},
            'collision_acoustics': acoustics,
            'pass': not missing and not different and not retired and not readiness_missing and
                    len(acoustics) == 2 and all(a['pass'] for a in acoustics.values())}

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('artifact', type=Path)
    p.add_argument('--repo', type=Path, required=True)
    p.add_argument('--baseline', type=Path)
    p.add_argument('--source-commit', default=SOURCE)
    p.add_argument('--expected-tree', default=TREE)
    p.add_argument('--artifact-sha256')
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    report = {'status': 'failed', 'expected_publication_commit': args.source_commit,
              'expected_source_tree': args.expected_tree, 'runtime_acceptance': 'pending',
              'limitations': ['No Windows binary or gameplay is executed.',
                 'Unchanged resource bytes are compared to the pinned previously verified Windows artifact; new/changed resources are checked against source pools or pinned palette transformations.']}
    opened = []
    try:
        repo = args.repo.resolve()
        tree = subprocess.check_output(['git', 'rev-parse', 'HEAD^{tree}'], cwd=repo, text=True).strip()
        assert tree == args.expected_tree == TREE and args.source_commit == SOURCE
        subprocess.run(['git', 'diff', '--quiet', 'HEAD', '--'], cwd=repo, check=True)
        report['source_content_tree'] = tree
        report['artifact_sha256'] = file_digest(args.artifact)
        if args.artifact_sha256:
            assert report['artifact_sha256'] == args.artifact_sha256.removeprefix('sha256:')
        direct, pngs, chamber, metadata = source_resources(repo)
        with tempfile.TemporaryDirectory(prefix='cumulative-package-') as temp:
            package = Package(args.artifact, Path(temp) / 'candidate')
            opened.append(package)
            baseline = None
            if args.baseline:
                assert file_digest(args.baseline) == BASE_SHA
                baseline = Package(args.baseline, Path(temp) / 'baseline')
                opened.append(baseline)
            report['package'] = {'members_crc_verified': len(package.index),
                                 'x64_runtime_images': package.images,
                                 'additional_external_chamber_archives': []}
            report['ports'] = {}
            baseline_ports, details = {}, {}
            for game in PORTS:
                z, entries = package.zips[game], package.indexes[game]
                required = set(direct[game]) | set(pngs[game]) | set(GRAY)
                missing = sorted(required - set(entries))
                mismatches = [n for n, b in direct[game].items() if n in entries and
                              entries[n] != {'bytes': len(b), 'sha256': digest(b)}]
                mismatches += [n for n, e in pngs[game].items() if n in entries and not png_matches(z.read(n), e)]
                mismatches += [n for n in GRAY if n in entries and entries[n]['sha256'] != GRAY_SHA]
                unexpected = []
                if baseline:
                    old = baseline.indexes[game]
                    added = sorted(set(entries) - set(old))
                    removed = sorted(set(old) - set(entries))
                    changed = sorted(n for n in set(old) & set(entries) if old[n] != entries[n])
                    expected_new = required - set(GRAY) - {ICON}
                    unexpected = sorted(set(added) - expected_new)
                    assert expected_new.isdisjoint(old), 'New source identities must not overwrite baseline resources'
                    allowed = set(GRAY) | {ICON}
                    unverified = sorted(set(changed) - allowed)
                    baseline_ports[game] = {'prior_resources': len(old), 'candidate_resources': len(entries),
                        'byte_identical': len(set(old) & set(entries)) - len(changed),
                        'added': len(added), 'changed': len(changed), 'removed': len(removed),
                        'removed_outside_explicit_retirement': removed,
                        'changed_not_verified_against_current_source': unverified,
                        'pass': not removed and not unverified and set(changed) == allowed and
                                set(added) == expected_new and not mismatches and not missing}
                    details[game] = {'added': {n: entries[n] for n in added},
                        'changed': {n: {'before': old[n], 'after': entries[n]} for n in changed}, 'removed': removed}
                inventory = {'expected_resources': COUNTS[game], 'missing': missing, 'unexpected': unexpected,
                    'mismatches': sorted(set(mismatches)),
                    'pass': not missing and not unexpected and not mismatches and len(entries) == COUNTS[game]}
                chamber_report = check_chamber(repo, z, entries, chamber, metadata)
                dedicated_names = required - set(chamber) - set(GRAY) - {ICON}
                dedicated = {'resources': sorted(dedicated_names),
                    'pass': not set(dedicated_names) & (set(missing) | set(mismatches))}
                report['ports'][game] = {'crc_verified_resources': len(entries), 'source_inventory': inventory,
                    'chamber': chamber_report, 'dedicated_assets': dedicated,
                    'archive_sha256': file_digest(package.port_paths[game]),
                    'sha256': file_digest(package.port_paths[game])}
            if baseline:
                report['baseline'] = {'artifact_sha256': BASE_SHA, 'ports': baseline_ports}
                diff = args.output.with_name(args.output.stem + '-resource-diff.json')
                diff.write_text(json.dumps(details, indent=2, sort_keys=True) + '\n')
            good = all(v['source_inventory']['pass'] and v['chamber']['pass'] and
                       v['dedicated_assets']['pass'] for v in report['ports'].values())
            if baseline:
                good = good and all(v['pass'] for v in baseline_ports.values())
            report['status'] = 'passed' if good else 'failed'
    except Exception as e:
        report['error'] = type(e).__name__ + ': ' + str(e)
    finally:
        for package in reversed(opened):
            package.close()
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(json.dumps({'status': report['status'], 'report': str(args.output), 'error': report.get('error')}))
    return 0 if report['status'] == 'passed' else 1

if __name__ == '__main__':
    raise SystemExit(main())
