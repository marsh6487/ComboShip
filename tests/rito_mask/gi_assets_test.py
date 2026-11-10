#!/usr/bin/env python3
"""Resource-level proof for authored Rito GI; deliberately not runtime acceptance."""
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import struct
import sys
import unittest
import xml.etree.ElementTree as ET
import numpy as np
from PIL import Image
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT))
from tools.rito_mask_gi.build import NS, ICON_FILES, MATRIX_SCALE, build_assets
from tools.rito_mask_gi.preview import model, read_texture


class RitoGiAssets(unittest.TestCase):
    def test_host_parity_reproducibility_and_preserved_icons(self):
        resources,meta=build_assets()
        manifest=json.loads((ROOT/'tools/rito_mask_gi/manifest.json').read_text())
        for owner in ('soh','mm'):
            base=ROOT/owner/'assets/custom'/NS
            self.assertEqual({p.name for p in base.iterdir()},set(resources))
            for name,data in resources.items():
                self.assertEqual((base/name).read_bytes(),data)
                self.assertEqual(hashlib.sha256(data).hexdigest(),manifest['sha256'][name])
        for path in ICON_FILES:
            raw=(ROOT/'soh/assets/custom'/path).read_bytes()
            self.assertEqual(raw,(ROOT/'mm/assets/custom'/path).read_bytes())
            self.assertEqual(hashlib.sha256(raw).hexdigest(),manifest['preserved_icon_sha256'][path])
            with Image.open(ROOT/'mm/assets/custom'/path) as image:
                image.verify()
        self.assertFalse(list((ROOT/'tools/rito_mask_gi').rglob('*.gif')))
        self.assertEqual(meta['bounds_native'],manifest['bounds_native'])
        self.assertEqual(meta['triangles'],manifest['triangles'])

    def test_complete_native_resources_and_vertex_cache(self):
        for owner in ('soh','mm'):
            base=ROOT/owner/'assets/custom';directory=base/NS
            dl=ET.parse(directory/'gi_dl').getroot()
            vs=ET.parse(directory/'mesh_opa_vtx').getroot()
            self.assertEqual(dl.tag,'DisplayList');self.assertEqual(dl.attrib['Version'],'0')
            self.assertEqual(vs.tag,'Vertex');self.assertEqual(vs.attrib['Version'],'0')
            self.assertEqual(dl[-1].tag,'EndDisplayList')
            for v in vs:
                self.assertTrue(all(-32768<=int(v.attrib[k])<=32767 for k in ('X','Y','Z','S','T')))
                self.assertTrue(all(0<=int(v.attrib[k])<=255 for k in ('R','G','B','A')))
                self.assertEqual(int(v.attrib['A']),255)
            reached={'gi_dl'};slots=set();triangles=0;balance=0
            for c in dl:
                a=c.attrib
                if 'Path' in a:
                    self.assertTrue(a['Path'].startswith(NS))
                    f=base/a['Path'];self.assertTrue(f.is_file());reached.add(f.name)
                if c.tag=='LoadVertices':
                    offset,count,slot=(int(a[k]) for k in ('VertexOffset','Count','VertexBufferIndex'))
                    self.assertTrue(0<count<=30 and 0<=slot and slot+count<=30)
                    self.assertTrue(0<=offset and offset+count<=len(vs))
                    slots=set(range(slot,slot+count))
                elif c.tag=='Triangle1':
                    self.assertTrue({int(a['V0'+str(i)]) for i in range(3)}<=slots);triangles+=1
                elif c.tag=='Matrix':
                    self.assertEqual(a['Param'],'G_MTX_PUSH');balance+=1
                elif c.tag=='PopMatrix':balance-=1;self.assertGreaterEqual(balance,0)
                elif c.tag=='LoadTextureBlock':
                    raw=(base/a['Path']).read_bytes()
                    self.assertEqual(struct.unpack_from('<II',raw,4),(0x4f544558,0))
                    kind,w,h,size=struct.unpack_from('<4I',raw,64)
                    self.assertEqual((kind,w,h,size),(2,32,32,2048))
                    self.assertEqual(len(raw),80+size);self.assertLessEqual(size,4096)
                    self.assertEqual(read_texture(base/a['Path']).shape,(32,32,4))
            self.assertEqual(balance,0);self.assertEqual(triangles,768)
            self.assertEqual(reached,{p.name for p in directory.iterdir()})
            # Parser resolves the actual N64 fixed-point scale, winding and colors.
            m=model(owner);self.assertEqual(len(m.parts),triangles)
            for part in m.parts:
                p=part.positions;cross=np.cross(p[1]-p[0],p[2]-p[0]);self.assertGreater(np.linalg.norm(cross),0)
                self.assertTrue(np.all(part.normals @ (cross/np.linalg.norm(cross))>.999))

    def test_closed_outward_components_and_true_eye_openings(self):
        manifest=json.loads((ROOT/'tools/rito_mask_gi/manifest.json').read_text())
        # Production XML groups by material, so identify each closed component
        # independently via shared quantized position edges instead of recipe IDs.
        parts=model().parts;edges=defaultdict(list);direction=Counter();adjacency=defaultdict(set)
        for i,part in enumerate(parts):
            p=np.rint(part.positions/MATRIX_SCALE).astype(int)
            for a,b in zip(p,np.roll(p,-1,axis=0)):
                a,b=tuple(a),tuple(b);edge=tuple(sorted((a,b)));edges[edge].append(i);direction[edge]+=1 if a<b else -1
        self.assertTrue(all(len(ids)==2 for ids in edges.values()),'Every component must be closed')
        self.assertTrue(all(count==0 for count in direction.values()),'Shared edges must wind oppositely')
        for a,b in edges.values():adjacency[a].add(b);adjacency[b].add(a)
        remaining=set(range(len(parts)));count=0
        while remaining:
            stack=[remaining.pop()];component=[]
            while stack:
                current=stack.pop();component.append(current)
                for other in adjacency[current]&remaining:remaining.remove(other);stack.append(other)
            volume=sum(np.dot(parts[i].positions[0],np.cross(parts[i].positions[1],parts[i].positions[2]))/6 for i in component)
            self.assertGreater(volume,0,'Culling must face outside every closed component');count+=1
        self.assertEqual(count,len(manifest['components']))
        # Orthographic rays through eye centers must intersect no triangle at any
        # Z, proving holes continue through shell, bevel, trim and rear lining.
        offset=np.array(manifest['source_center_offset'])
        for x in (-48,48):
            eye=(np.array([x,12,0])-offset)[:2]*MATRIX_SCALE
            hits=0
            for part in parts:
                p=part.positions[:,:2]
                area=np.cross(np.r_[p[1]-p[0],0],np.r_[p[2]-p[0],0])[2]
                if abs(area)<1e-10:continue
                cross=lambda a,b:a[0]*b[1]-a[1]*b[0]
                bary=[cross(p[1]-eye,p[2]-eye)/area,cross(p[2]-eye,p[0]-eye)/area,cross(p[0]-eye,p[1]-eye)/area]
                if min(bary)>=0:hits+=1
            self.assertEqual(hits,0,'Eye must be a true open aperture')
        points=np.concatenate([p.positions for p in parts]);low,high=points.min(0),points.max(0)
        self.assertTrue(np.array_equal(np.array(manifest['bounds_native'])*MATRIX_SCALE,[low,high]))
        self.assertLess(np.linalg.norm((low+high)/2),.25)
        self.assertGreater(high[2]-low[2],20,'Sculpture must have substantial physical depth')
        self.assertEqual(manifest['draw_scale'],1.)

    def test_material_setup_sheen_and_exit_state(self):
        root=ET.parse(ROOT/'soh/assets/custom'/NS/'gi_dl').getroot()
        self.assertEqual(root.find('SetGeometryMode').attrib['G_CULL_BACK'],'1')
        self.assertEqual(root.find('SetGeometryMode').attrib['G_LIGHTING'],'1')
        self.assertEqual(root.find('ClearGeometryMode').attrib['G_FOG'],'1')
        self.assertEqual(root.find('SetRenderMode').attrib,dict(Mode1='G_RM_PASS',Mode2='G_RM_AA_ZB_OPA_SURF2'))
        self.assertEqual(root.findall('Texture')[-1].attrib['On'],'0')
        self.assertEqual(root.findall('ClearGeometryMode')[-1].attrib,dict(G_TEXTURE_GEN='1',G_TEXTURE_GEN_LINEAR='1'))
        self.assertEqual(root.findall('SetPrimColor')[-1].attrib,dict(M='0',L='0',R='255',G='255',B='255',A='255'))
        self.assertEqual(root.findall('SetEnvColor')[-1].attrib,dict(R='255',G='255',B='255',A='255'))
        parts=model().parts;metal=[p for p in parts if p.material.texgen]
        self.assertGreater(len(metal),200)
        for part in metal:
            self.assertTrue(part.material.texgen_linear and part.material.clamp)
            self.assertIsNotNone(part.material.environment)
            self.assertEqual(part.material.tex_scale,(1984,1984))
        self.assertTrue(all(p.material.cull for p in parts))
        self.assertEqual(root[-2].tag,'PopMatrix')

if __name__=='__main__':unittest.main()
