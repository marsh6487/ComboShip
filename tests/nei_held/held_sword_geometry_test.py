"""Run serialized held sword matrices against the exact reviewed GI vertices."""
from pathlib import Path
from functools import lru_cache
import hashlib
import json
import struct
import unittest
import xml.etree.ElementTree as ET

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "soh/assets/custom"
# Independent landmarks from the quantized approved grip components. Visual
# lengths use the existing native melee reach, except Four Sword's legacy tip.
LANDMARKS = {
    "kokiri_sword": (-14.5, 61., 3000.),
    "mm_kokiri_sword": (-13.5, 63., 3000.),
    "razor_sword": (-33.5, 56.5, 4000.),
    "gilded_sword": (-18.75, 93.625, 4000.),
    "master_sword": (-18.75, 106.375, 4000.),
    "true_master_sword": (-18.75, 106.375, 4000.),
    "biggoron_sword": (-25.75, 166., 5500.),
    "great_fairy_sword": (-21.90625, 154.5, 5500.),
    "four_sword": (-15.40625, 78., 3389.),
}
SOCKETS = {"oot_adult": (0., 328., -77.),
           "oot_child": (0., 216.22, 4.5),
           "mm_human": (0., 216.22, 4.5)}


def matrix(path):
    data = path.read_bytes()
    assert len(data) == 128 and struct.unpack_from("<I", data, 4)[0] == 0x4F4D5458
    words = struct.unpack_from("<16I", data, 64)
    values = []
    for integer, fraction in zip(words[:8], words[8:]):
        values += [(integer & 0xffff0000) | (fraction >> 16),
                   ((integer << 16) & 0xffff0000) | (fraction & 0xffff)]
    return np.array([v if v < 2**31 else v - 2**32 for v in values]).reshape(4, 4) / 65536


@lru_cache(maxsize=None)
def vertices(path):
    return np.array([[int(v.get(axis)) for axis in ("X", "Y", "Z")] + [1]
                     for v in ET.parse(ASSETS / path).getroot()], float)


def execute(path, wrist):
    """Execute the serialized modelview stack and every real vertex load."""
    current, stack, loaded, source, counts = wrist.copy(), [], [], [], [0, 0]

    def draw(resource, ancestors):
        nonlocal current
        assert resource not in ancestors, "Recursive display-list graph"
        ancestors = ancestors | {resource}
        for command in ET.parse(ASSETS / resource).getroot():
            if command.tag == "Matrix":
                flags = command.get("Param", "")
                if "G_MTX_PUSH" in flags:
                    stack.append(current.copy())
                    counts[0] += 1
                transform = matrix(ASSETS / command.get("Path"))
                current = transform if "G_MTX_LOAD" in flags else transform @ current
            elif command.tag == "DisplayList":
                draw(command.get("Path"), ancestors)
            elif command.tag == "PopMatrix":
                assert stack and command.get("Param") == "G_MTX_MODELVIEW"
                current = stack.pop()
                counts[1] += 1
            elif command.tag == "LoadVertices":
                offset, count = int(command.get("VertexOffset")), int(command.get("Count"))
                mesh = vertices(command.get("Path"))[offset:offset + count]
                assert len(mesh) == count and count <= 32
                source.append(mesh)
                loaded.append(mesh @ current)
            elif command.tag == "EndDisplayList":
                break

    draw(path, set())
    assert not stack and counts == [2, 2], "Held/GI modelview stack leaked into the later hand"
    return np.concatenate(source), np.concatenate(loaded), current


class HeldSwordGeometry(unittest.TestCase):
    def test_exact_mesh_has_native_grip_axis_tip_and_balanced_matrix(self):
        for slug, (grip, tip, reach) in LANDMARKS.items():
            source = "objects/nei_gi_redesign/" + slug + "/gi_dl"
            native = matrix(ASSETS / "objects/nei_gi_redesign" / slug / "scale_mtx")
            for frame, socket in SOCKETS.items():
                with self.subTest(sword=slug, frame=frame):
                    prefix = "objects/nei_held_swords/" + slug + "/" + frame + "/"
                    wrapper_path = ASSETS / prefix / "held_dl"
                    self.assertTrue(wrapper_path.exists(), "Exact GI sword has no native held attachment")
                    wrapper = ET.parse(wrapper_path).getroot()
                    self.assertEqual([n.tag for n in wrapper], ["Matrix", "DisplayList", "PopMatrix", "EndDisplayList"])
                    self.assertEqual(wrapper[1].get("Path"), source)
                    self.assertEqual(wrapper[0].get("Param"), "G_MTX_PUSH")
                    self.assertEqual(wrapper[2].get("Param"), "G_MTX_MODELVIEW")
                    held = matrix(ASSETS / wrapper[0].get("Path"))
                    conversion = native @ held
                    palm = np.array([0., grip * 16, 0., 1.]) @ conversion
                    np.testing.assert_allclose(palm[:3], socket, atol=.005)
                    axis = np.array([0., 1., 0., 0.]) @ conversion
                    self.assertGreater(axis[0], 0)
                    np.testing.assert_allclose(axis[1:3], 0, atol=1e-8)
                    linear = conversion[:3, :3]
                    np.testing.assert_allclose(linear @ linear.T, np.eye(3) * axis[0]**2, atol=1e-6)
                    self.assertGreater(np.linalg.det(linear), 0)
                    endpoint = np.array([0., tip * 16, 0., 1.]) @ conversion
                    target = 3000. if slug == "razor_sword" and frame == "mm_human" else reach
                    self.assertAlmostEqual(endpoint[0], target, delta=.005)
                    # Modelview pushes in both wrapper and GI body are popped;
                    # a later hand must still receive this original wrist frame.
                    wrist = np.array([[0, 1, 0, 0], [-1, 0, 0, 0], [0, 0, 1, 0], [10, 20, 30, 1]], float)
                    raw, rendered, restored = execute(prefix + "held_dl", wrist)
                    np.testing.assert_array_equal(restored, wrist)
                    # Real serialized vertices receive one common rigid scale
                    # and rotation. Both native resource pushes restore wrist.
                    np.testing.assert_allclose(rendered, raw @ conversion @ wrist, atol=1e-6)
                    wrist_local = rendered @ np.linalg.inv(wrist)
                    self.assertAlmostEqual(wrist_local[:, 0].max(), target, delta=.005)

    def test_reviewed_source_materials_vertices_and_lists_are_immutable(self):
        # Approved manifests predate this implementation. The two centered
        # originals have no manifest hashes, so freeze their baseline blobs.
        originals = json.loads((ROOT / "tests/nei_held/held_sword_source_hashes.json").read_text())
        for slug in LANDMARKS:
            record = json.loads((ROOT / "tools/nei_gi/CHECKPOINTS" / slug / "checkpoint.json").read_text())
            provenance = record.get("source_provenance", {})
            hashes = provenance.get("resource_sha256", originals["resource_sha256"].get(slug))
            self.assertTrue(hashes, (slug, "No immutable source evidence"))
            for name, expected in hashes.items():
                actual = hashlib.sha256((ASSETS / "objects/nei_gi_redesign" / slug / name).read_bytes()).hexdigest()
                self.assertEqual(actual, expected, (slug, name))


if __name__ == "__main__":
    unittest.main()
