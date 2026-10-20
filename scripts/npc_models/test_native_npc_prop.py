"""Ocarina HD texture binding preserves every non-texture display-list word."""
import struct
import unittest
from native_assets import o2r as asset
import native_npc_pose_fit as fit
from verify_native_npc_models import Archive

class PropTextureTests(unittest.TestCase):
    def test_rebinding_changes_only_matching_texture_reference_hash(self):
        self.assertTrue(callable(getattr(fit,'bind_ocarina_texture',None)),'Private prop texture binding is missing')
        d=asset.DLBuilder();d.marker('prop');d.settimg_hash(0,2,1,'native/ocarina');d.end();raw=d.to_resource()
        changed=fit.bind_ocarina_texture(raw,'native/ocarina','private/hd')
        h=asset.crc64('private/hd');expected=bytearray(raw);struct.pack_into('<II',expected,96,h>>32,h&0xffffffff)
        self.assertEqual(changed,bytes(expected))
        with self.assertRaisesRegex(ValueError,'one texture'):
            fit.bind_ocarina_texture(raw,'wrong/reference','private/hd')

    def test_texture_scale_uses_resource_metadata_instead_of_filename(self):
        self.assertTrue(callable(getattr(Archive,'texture_scale',None)),'Texture scale metadata reader is missing')
        a=Archive.__new__(Archive)
        raw=asset._header(asset.T_OTEX,1)+struct.pack('<IIIIffI',2,256,128,1,8,8,65536)+bytes(65536)
        a.read=lambda path:raw
        self.assertEqual(a.texture_scale('any/path'),(8,8))
        raw=asset.build_texture_rgba16(bytes(1024),32,16)
        self.assertEqual(a.texture_scale('any/path'),(1,1))
