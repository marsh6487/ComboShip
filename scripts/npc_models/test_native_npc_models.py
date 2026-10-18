"""Tests for source material selection and native rig compatibility."""
import io
import json
from pathlib import Path
import struct
import tempfile
import unittest

from PIL import Image

try:
    import native_npc_models as compiler
except ModuleNotFoundError:
    compiler = None


def fixture_glb(path):
    """A normal map precedes the diffuse image, as in the supplied Tripo files."""
    binary = bytearray()
    views = []
    for color in [(0, 0, 255), (235, 130, 60)]:
        stream = io.BytesIO()
        Image.new("RGB", (2, 2), color).save(stream, format="PNG")
        views.append({"buffer": 0, "byteOffset": len(binary), "byteLength": len(stream.getvalue())})
        binary.extend(stream.getvalue())
    document = {"asset": {"version": "2.0"}, "scene": 0, "scenes": [{"nodes": []}],
                "nodes": [], "meshes": [], "skins": [], "accessors": [], "bufferViews": views,
                "buffers": [{"byteLength": len(binary)}],
                "images": [{"bufferView": 0, "mimeType": "image/png"},
                           {"bufferView": 1, "mimeType": "image/png"}],
                "textures": [{"source": 0}, {"source": 1}],
                "materials": [{"pbrMetallicRoughness": {"baseColorTexture": {"index": 1}}}]}
    raw = json.dumps(document).encode()
    raw += b" " * (-len(raw) % 4)
    binary.extend(b"\0" * (-len(binary) % 4))
    path.write_bytes(struct.pack("<III", 0x46546C67, 2, 28 + len(raw) + len(binary)) +
                     struct.pack("<I4s", len(raw), b"JSON") + raw +
                     struct.pack("<I4s", len(binary), b"BIN\0") + binary)


class ExportTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(compiler, "The native NPC archive compiler is missing")

    def test_uses_material_diffuse_instead_of_first_image(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "fixture.glb"
            fixture_glb(path)
            source = compiler.SourceModel(path)
            self.assertEqual(source.image.getpixel((0, 0)), (235, 130, 60, 255))

    def test_kokiri_girl_scope_covers_every_native_placement(self):
        self.assertEqual(compiler.PROFILES["twins"]["native_actor_types"], [1, 5, 6, 9, 10])

    def test_saria_variants_share_the_same_actor_contract(self):
        a = compiler.VARIANTS["saria-anime"]
        b = compiler.VARIANTS["saria-render"]
        self.assertEqual(a["profile"], "saria")
        self.assertEqual(a["profile"], b["profile"])
        self.assertNotEqual(a["archive"], b["archive"])

    def test_rotation_handles_opposite_vectors_without_nan(self):
        import numpy as np
        rotation = compiler.align([0, 1, 0], [0, -1, 0])
        np.testing.assert_allclose(rotation @ [0, 1, 0], [0, -1, 0], atol=1e-8)
        np.testing.assert_allclose(rotation.T @ rotation, np.eye(3), atol=1e-8)

    def test_animation_sampler_rejects_out_of_bounds_native_channels(self):
        raw = bytearray(compiler.asset._header(0x4F414E4D))
        raw += struct.pack("<IhIhIHHHH", 0, 1, 1, 0, 1, 0, 20, 0, 1)
        with self.assertRaises(ValueError):
            compiler.animation_frames(bytes(raw))

    def test_preview_rig_reads_private_limb_offsets(self):
        import numpy as np
        from edit_native_npc_pose import encode_animation
        compiler.PROFILES["fixture"] = dict(object="object_fixture", skeleton="Skel", animation="Anim")
        ns = "objects/object_fixture/NativeNpcV1"
        limb_path = ns+"/Rig/Limb00"
        skel = compiler.asset.build_skeleton([limb_path], 1,
                                             compiler.asset.SKEL_FLEX, 1)
        animation = encode_animation(np.zeros((1, 2, 3), dtype=int))
        class Fixture:
            def read(self, path):
                return {ns+"/Skel": skel,
                        limb_path: compiler.asset.build_limb((11, 22, 33), 255, 255, ns+"/Body/Limb00", "", 1),
                        "objects/object_fixture/Anim": animation}[path]
        try:
            rig = compiler.NativeRig(Fixture(), "fixture", private=True)
            self.assertEqual((rig.limbs[0].x, rig.limbs[0].y, rig.limbs[0].z), (11, 22, 33))
        finally:
            del compiler.PROFILES["fixture"]

    def test_pose_editor_round_trip_preserves_root_and_other_joints(self):
        import numpy as np
        import zipfile
        import hashlib
        from edit_native_npc_pose import edit, encode_animation
        frames = np.array([[[10, 20, 30], [100, 200, 300]], [[10, 20, 30], [400, 500, 600]]])
        np.testing.assert_array_equal(compiler.animation_frames(encode_animation(frames)), frames)
        with tempfile.TemporaryDirectory() as temp:
            base, native, output = [Path(temp)/name for name in ["model.o2r", "native.o2r", "fit.o2r"]]
            path = "objects/object_os_anime/testAnim"
            with zipfile.ZipFile(native, "w") as z:
                z.writestr(path, encode_animation(frames))
            with zipfile.ZipFile(base, "w") as z:
                z.writestr("test_NativeNpcModelInfo.json", json.dumps(dict(namespace="objects/test/NativeNpcV1", target_limbs=1, variant="test")))
                z.writestr("alt/objects/test/NativeNpcV1/Body/Limb00", b"untouched")
            recipe = dict(schema_version=1, parent_sha256=hashlib.sha256(base.read_bytes()).hexdigest(),
                          clips=[dict(native_animation=path, rotations=[dict(joint=1, mode="add", binang=[1000, 0, 0])])])
            edit(base, native, recipe, output)
            with zipfile.ZipFile(output) as z:
                actual = compiler.animation_frames(z.read("alt/objects/test/NativeNpcV1/PoseAnimations/"+path))
                np.testing.assert_array_equal(actual[:, 0], frames[:, 0])
                np.testing.assert_array_equal(actual[:, 1, 0], frames[:, 1, 0]+1000)
                self.assertEqual(z.read("alt/objects/test/NativeNpcV1/Body/Limb00"), b"untouched")
            recipe["clips"][0]["rotations"][0]["joint"] = 0
            with self.assertRaisesRegex(ValueError, "root motion"):
                edit(base, native, recipe, output)

    def test_split_surface_join_keeps_one_native_owner(self):
        import numpy as np
        self.assertTrue(hasattr(compiler, "join_split_surfaces"), "Split Tripo surface joins need native seam fitting")
        model = dict(source_positions=np.array([[0., 0., 0.], [.001, 0., 0.]]),
                     positions=np.array([[0., 0., 0.], [100., 0., 0.]]),
                     bones=np.array([0, 1]), weights=np.array([[1., 0.], [0., 1.]]),
                     mesh_ids=np.array([0, 1]), boundary=np.array([True, True]))
        compiler.join_split_surfaces(model, 1000, -1)
        np.testing.assert_allclose(model["positions"][0], model["positions"][1])
        self.assertEqual(model["bones"][0], model["bones"][1])


if __name__ == "__main__":
    unittest.main()
