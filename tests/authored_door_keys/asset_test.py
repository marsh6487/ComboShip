#!/usr/bin/env python3
"""Focused checks on serialized native resources, topology, state and reproducibility."""
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import struct
import sys
import unittest
import zipfile
import xml.etree.ElementTree as ET
import numpy as np
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT))
from tools.authored_door_keys.build import build_assets, NS, OWNERS
from tools.authored_door_keys.preview import model, read_texture

class AuthoredDoorKeyAssets(unittest.TestCase):
    def test_complete_owner_local_dependencies_and_batches(self):
        all_names=set()
        for name,owner in OWNERS.items():
            custom=ROOT/owner/'assets/custom';directory=custom/NS
            files={p.name for p in directory.iterdir() if p.is_file()}
            self.assertFalse(all_names & files, 'Owners must not shadow each other')
            all_names |= files
            visited=set();triangles=0
            def walk(path,stack=()):
                nonlocal triangles
                self.assertTrue(path.startswith(NS+name));self.assertNotIn(path,stack)
                target=custom/path;self.assertTrue(target.is_file());visited.add(target.name)
                if target.name.endswith('Tex'):
                    raw=target.read_bytes()
                    self.assertEqual(struct.unpack_from('<II',raw,4),(0x4f544558,0))
                    tex=read_texture(target)
                    if target.name.endswith('ReflectionTex'):
                        self.assertEqual(tex.shape,(16,16,4));self.assertTrue(np.all(tex[:,:,3]==1))
                    else:self.assertEqual(tex.shape,(32,32,4))
                    self.assertLessEqual(len(raw)-80,4096);return
                root=ET.parse(target).getroot()
                if root.tag=='Vertex':
                    for v in root:
                        self.assertTrue(all(-32768<=int(v.attrib[k])<=32767 for k in ('X','Y','Z','S','T')))
                        self.assertTrue(all(0<=int(v.attrib[k])<=255 for k in ('R','G','B','A')))
                    return
                slots=set()
                for c in root:
                    a=c.attrib
                    if 'Path' in a:walk(a['Path'],stack+(path,))
                    if c.tag=='LoadVertices':
                        start,count,slot=[int(a[k]) for k in ('VertexOffset','Count','VertexBufferIndex')]
                        self.assertGreater(count,0);self.assertLessEqual(count,30);self.assertGreaterEqual(slot,0);self.assertLessEqual(slot+count,30)
                        self.assertLessEqual(start+count,len(ET.parse(custom/a['Path']).getroot()))
                        slots=set(range(slot,slot+count))
                    elif c.tag=='Triangle1':
                        self.assertTrue(set(int(a['V0'+str(i)]) for i in range(3))<=slots);triangles+=1
                self.assertEqual(root[-1].tag,'EndDisplayList')
            walk(NS+name+'DL')
            self.assertEqual(visited,files,'Every authored resource must be reachable')
            self.assertGreater(triangles,500);self.assertLess(triangles,2000)

    def test_quantized_closed_outward_geometry_and_scale(self):
        for name in OWNERS:
            m=model(name);m.parts=[p for p in m.parts if not p.material.decal];edges=defaultdict(list);directed=Counter();points=[]
            for i,part in enumerate(m.parts):
                p=part.positions;points.extend(p)
                n=np.cross(p[1]-p[0],p[2]-p[0]);self.assertGreater(np.linalg.norm(n),0)
                self.assertTrue(np.all(part.normals @ (n/np.linalg.norm(n))>.7),'Signed native normals must face outside')
                for a,b in zip(p,np.roll(p,-1,axis=0)):
                    a,b=tuple(a),tuple(b);key=tuple(sorted((a,b)));edges[key].append(i);directed[key]+=1 if a<b else -1
            self.assertTrue(all(len(v)==2 for v in edges.values()),'Closed components require exactly two faces per edge')
            self.assertTrue(all(v==0 for v in directed.values()),'Shared edge winding must oppose')
            adjacency=defaultdict(set)
            for a,b in edges.values():adjacency[a].add(b);adjacency[b].add(a)
            remaining=set(range(len(m.parts)));components=0
            while remaining:
                stack=[remaining.pop()];component=[]
                while stack:
                    current=stack.pop();component.append(current)
                    for other in adjacency[current]&remaining:remaining.remove(other);stack.append(other)
                volume=sum(np.dot(m.parts[i].positions[0],np.cross(m.parts[i].positions[1],m.parts[i].positions[2]))/6 for i in component)
                self.assertGreater(volume,0,'Every closed component must have outward winding');components+=1
            self.assertGreaterEqual(components,3)
            points=np.array(points)*.25;low,high=points.min(0),points.max(0)
            self.assertGreaterEqual(high[1]-low[1],65);self.assertLessEqual(high[1]-low[1],75)
            self.assertLess(np.linalg.norm((high+low)/2),1);self.assertGreater(high[2]-low[2],4)

    def test_material_setup_and_teardown(self):
        for name,owner in OWNERS.items():
            root=ET.parse(ROOT/owner/'assets/custom'/NS/(name+'DL')).getroot()
            combine=root.find('SetCombineLERP').attrib
            self.assertEqual(combine['A0'],'G_CCMUX_TEXEL0');self.assertEqual(combine['C0'],'G_CCMUX_SHADE')
            self.assertNotIn('G_CCMUX_ENVIRONMENT',combine.values(),'CSMC env tint must not recolor authored materials')
            self.assertEqual(root.find('SetGeometryMode').attrib['G_LIGHTING'],'1')
            self.assertEqual(root.find('ClearGeometryMode').attrib['G_FOG'],'1')
            textures=root.findall('Texture');self.assertEqual(textures[0].attrib['On'],'1');self.assertEqual(textures[-1].attrib['On'],'0')
            self.assertEqual(root.findall('SetEnvColor')[-1].attrib,dict(R='255',G='255',B='255',A='255'))
            self.assertEqual(root[-2].tag,'SetRenderMode');self.assertEqual(root[-2].attrib['Mode2'],'G_RM_AA_ZB_OPA_SURF2');self.assertEqual(root[-1].tag,'EndDisplayList')
            models=model(name);base=[p for p in models.parts if not p.material.decal];coat=[p for p in models.parts if p.material.decal]
            self.assertEqual(len(base),len(coat))
            for metal,weather in zip(base,coat):
                self.assertTrue(np.array_equal(metal.positions,weather.positions));self.assertTrue(np.array_equal(metal.normals,weather.normals))
                self.assertTrue(metal.material.texgen and metal.material.texgen_linear)
                self.assertEqual(metal.material.tex_scale,(960,960));self.assertIsNotNone(metal.material.environment)
                self.assertFalse(weather.material.texgen);self.assertEqual(weather.material.tex_scale,(65535,65535));self.assertIsNone(weather.material.environment)
            ref=ET.parse(ROOT/owner/'assets/custom'/NS/(name+'ReflectionDL')).getroot()
            self.assertEqual(ref.find('SetCombineLERP').attrib['Ad0'],'G_ACMUX_1')
            self.assertEqual(root.findall('SetCombineLERP')[-1].attrib['Ad0'],'G_ACMUX_TEXEL0')
            self.assertEqual(root.findall('SetRenderMode')[0].attrib['Mode2'],'G_RM_AA_ZB_XLU_DECAL2')

    def test_approved_geometry_and_weathering_preserved(self):
        baseline=ROOT/'tools/authored_door_keys/baseline'
        manifest=json.loads((baseline/'manifest.json').read_text())
        with zipfile.ZipFile(baseline/'approved-native-resources.zip') as archive:
            for path,digest in manifest['sha256'].items():
                raw=archive.read(path);self.assertEqual(hashlib.sha256(raw).hexdigest(),digest)
                if path.endswith('Vtx'):
                    self.assertEqual((ROOT/path).read_bytes(),raw,'Approved position, UV and normal bytes must remain unchanged')
                if not path.endswith('Tex'):continue
                value=np.frombuffer(raw,dtype='>u2',offset=80).reshape(32,32).astype(np.uint32)
                old_rgb=np.stack([((value>>b)&31)*255//31 for b in (11,6,1)],axis=-1)
                current=np.rint(read_texture(ROOT/path)*255).astype(int)
                self.assertTrue(np.array_equal(current[:,:,:3],old_rgb),'Native-decoded weathering RGB must remain exactly unchanged')
                r,g,b=[old_rgb[:,:,i].astype(float) for i in range(3)]
                matte=((r>g*1.25+5)|(r<38)) if 'RustyKey' in path else ((g>=r*.88)|(r<70))
                if 'PatinaTex' in path:matte[:]=True
                self.assertTrue(np.all(current[:,:,3][matte]==255),'Oxide, patina and pits fully occlude reflection')
                if 'PatinaTex' not in path:
                    self.assertTrue(np.any(current[:,:,3]<255),'Exposed metal must reveal the native reflection')
                    self.assertGreaterEqual(current[:,:,3].min(),140,'Keep the sheen restrained under the static coat')

    def test_deterministic_regeneration_and_manifest(self):
        assets,stats=build_assets();manifest=json.loads((ROOT/'tools/authored_door_keys/manifest.json').read_text())
        self.assertEqual(stats,manifest['models'])
        self.assertEqual({str(p):hashlib.sha256(data).hexdigest() for p,data in assets.items()},manifest['sha256'])
        for path,data in assets.items():self.assertEqual((ROOT/path).read_bytes(),data,str(path))

if __name__=='__main__':unittest.main(verbosity=2)
