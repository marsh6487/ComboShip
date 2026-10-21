"""Repair destinations must never alias protected input inodes."""
import contextlib
import io
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest import mock
import zipfile

from native_assets import o2r
import recolor_saria_iris as iris
import refine_saria_ocarina as prop
import repair_npc_face_torso as face
import repair_twins_shoulders as twins


class InputPreservationTests(unittest.TestCase):
    def test_refinement_refuses_hardlinked_parent_before_writing(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); parent = root/'parent.o2r'; output = root/'output.o2r'
            ns = 'objects/object_sa/NativeNpcV1'
            dl = o2r.DLBuilder(); dl.settimg_hash(0, 2, 32, 'objects/object_sa/gSariaFairyOcarinaTex'); dl.end()
            with zipfile.ZipFile(parent, 'w') as archive:
                archive.writestr('saria_NativeNpcModelInfo.json', json.dumps(dict(
                    variant='saria-anime', namespace=ns, geometry_repairs={})))
                for suffix in ('', 'Grip'):
                    archive.writestr('alt/'+ns+'/Props/Ocarina'+suffix, dl.to_resource())
            before = parent.read_bytes(); os.link(parent, output)
            with self.assertRaisesRegex(ValueError, 'separate'):
                prop.refine(parent, output)
            self.assertEqual(parent.read_bytes(), before)

    def test_each_archive_repair_refuses_hardlinked_inputs_before_parsing(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            inputs = {name: root/name for name in ('native', 'parent', 'source', 'art')}
            for path in inputs.values(): path.write_bytes(b'protected input')
            calls = [
                ('face-native', 'native', lambda out: face.build(inputs['native'], inputs['parent'], 'malon', out)),
                ('face-parent', 'parent', lambda out: face.build(inputs['native'], inputs['parent'], 'malon', out)),
                ('face-source', 'source', lambda out: face.build(inputs['native'], inputs['parent'], 'saria-anime', out,
                                                                inputs['source'], inputs['art'])),
                ('face-art', 'art', lambda out: face.build(inputs['native'], inputs['parent'], 'saria-anime', out,
                                                          inputs['source'], inputs['art'])),
                ('twins-native', 'native', lambda out: twins.repair(inputs['native'], inputs['parent'], out)),
                ('twins-parent', 'parent', lambda out: twins.repair(inputs['native'], inputs['parent'], out)),
                ('iris-native', 'native', lambda out: iris.recolor(inputs['native'], inputs['parent'], out, inputs['source'])),
                ('iris-parent', 'parent', lambda out: iris.recolor(inputs['native'], inputs['parent'], out, inputs['source'])),
                ('iris-source', 'source', lambda out: iris.recolor(inputs['native'], inputs['parent'], out, inputs['source'])),
            ]
            for label, name, invoke in calls:
                with self.subTest(repair=label):
                    output = root/(label+'.o2r'); os.link(inputs[name], output)
                    with self.assertRaisesRegex(ValueError, 'separate'):
                        invoke(output)
                    self.assertEqual(inputs[name].read_bytes(), b'protected input')

    def test_face_report_refuses_input_and_output_aliases_before_build(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            paths = {name: root/name for name in ('native', 'parent', 'output', 'source', 'art')}
            for path in paths.values(): path.write_bytes(b'protected bytes')
            for name, path in paths.items():
                for hardlink in (False, True):
                    with self.subTest(target=name, hardlink=hardlink):
                        report = root/('report-'+name) if hardlink else path
                        if hardlink: os.link(path, report)
                        argv = ['face', str(paths['native']), str(paths['parent']), 'saria-anime',
                                str(paths['output']), '--eye-source', str(paths['source']),
                                '--eye-art', str(paths['art']), '--report', str(report)]
                        with mock.patch('sys.argv', argv), mock.patch.object(face, 'build', return_value={}) as build:
                            with contextlib.redirect_stdout(io.StringIO()):
                                with self.assertRaisesRegex(ValueError, 'separate'):
                                    face.main()
                            build.assert_not_called()
                        self.assertEqual(path.read_bytes(), b'protected bytes')


if __name__ == '__main__':
    unittest.main()
