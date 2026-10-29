#!/usr/bin/env python3
"""Candidate-only ZIP metadata normalization preserves every resource payload and flag."""
from pathlib import Path
import hashlib
import importlib.util
import io
import struct
import tempfile
import unittest
import warnings
import zipfile

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('normalize_candidate_o2r', ROOT/'scripts/packaging/normalize_candidate_o2r.py')
normalizer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(normalizer)


class NonSeekable(io.BytesIO):
    def seek(self, *args):
        raise io.UnsupportedOperation('stream')


def fixture(path, reverse=False):
    names = ['zeta', 'alpha', 'café/texture', 'folder/']
    if reverse:
        names.reverse()
    target = NonSeekable()
    with zipfile.ZipFile(target, 'w') as archive:
        archive.comment = b'different build metadata' if reverse else b'first build'
        for name in names:
            info = zipfile.ZipInfo(name, (2025 if reverse else 2026, 7, 12, 6, 4, 2))
            info.compress_type = zipfile.ZIP_STORED if name == 'alpha' else zipfile.ZIP_DEFLATED
            info.create_system = 0 if reverse else 3
            info.external_attr = (0o777 if reverse else 0o600) << 16
            info.comment = b'volatile comment'
            info.extra = struct.pack('<HHBI', 0x5455, 5, 1, 123456) + struct.pack('<HH', 0xCAFE, 3) + b'abc'
            archive.writestr(info, b'' if name.endswith('/') else (name.encode('utf8') + b'\x00\xff') * 50)
    path.write_bytes(target.getvalue())


def snapshot(path):
    with zipfile.ZipFile(path) as archive, path.open('rb') as raw:
        result = {}
        for info in archive.infolist():
            raw.seek(info.header_offset)
            header = normalizer.LOCAL.unpack(raw.read(normalizer.LOCAL.size))
            raw.seek(header[-2] + header[-1], io.SEEK_CUR)
            compressed = raw.read(info.compress_size)
            result[info.filename] = (info.compress_type, info.flag_bits, info.CRC,
                                     archive.read(info), compressed)
        return result


class NormalizationTests(unittest.TestCase):
    def test_metadata_order_flags_and_idempotence(self):
        with tempfile.TemporaryDirectory() as temporary:
            a, b = Path(temporary)/'soh.o2r', Path(temporary)/'2ship.o2r'
            fixture(a)
            fixture(b, True)
            before = snapshot(a)
            self.assertEqual(before, snapshot(b))
            result = normalizer.normalize_candidate_archive(a)
            normalizer.normalize_candidate_archive(b)
            self.assertEqual(before, snapshot(a))
            self.assertEqual(a.read_bytes(), b.read_bytes())
            self.assertEqual(result['payload_manifest_before'], result['payload_manifest_after'])
            self.assertEqual(result['entries'], 4)
            with zipfile.ZipFile(a) as archive:
                self.assertEqual(archive.namelist(), sorted(archive.namelist()))
                self.assertEqual(archive.comment, b'')
                for info in archive.infolist():
                    self.assertEqual(info.date_time, (1980, 1, 1, 0, 0, 0))
                    self.assertEqual(info.create_system, 3)
                    self.assertEqual(info.comment, b'')
                    self.assertEqual(info.extra, struct.pack('<HH', 0xCAFE, 3) + b'abc')
                    self.assertEqual(info.external_attr >> 16, 0o40755 if info.is_dir() else 0o100644)
            first = hashlib.sha256(a.read_bytes()).hexdigest()
            self.assertFalse(normalizer.normalize_candidate_archive(a)['changed'])
            self.assertEqual(hashlib.sha256(a.read_bytes()).hexdigest(), first)

    def test_duplicate_entries_rejected_without_mutation(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)/'candidate.o2r'
            with warnings.catch_warnings():
                warnings.simplefilter('ignore')
                with zipfile.ZipFile(path, 'w') as archive:
                    archive.writestr('same', b'one')
                    archive.writestr('same', b'two')
            before = path.read_bytes()
            with self.assertRaises(ValueError):
                normalizer.normalize_candidate_archive(path)
            self.assertEqual(path.read_bytes(), before)

    def test_corrupt_payload_rejected_without_mutation(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)/'candidate.o2r'
            with zipfile.ZipFile(path, 'w', compression=zipfile.ZIP_STORED) as archive:
                archive.writestr('entry', b'content')
            data = bytearray(path.read_bytes())
            data[data.index(b'content')] ^= 0x55
            path.write_bytes(data)
            before = path.read_bytes()
            with self.assertRaises(zipfile.BadZipFile):
                normalizer.normalize_candidate_archive(path)
            self.assertEqual(path.read_bytes(), before)


if __name__ == '__main__':
    unittest.main()
