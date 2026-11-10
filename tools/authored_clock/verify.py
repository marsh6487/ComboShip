#!/usr/bin/env python3
"""Validate the serialized Clock mesh, native texture/cache constraints and closure."""
from pathlib import Path
import sys, struct, json, hashlib
import xml.etree.ElementTree as ET
import numpy as np
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT))
from tools.authored_clock.build import NS,build
from tools.authored_clock.preview import model
from tools.authored_clock.clock_render import render,sample_native

def clip_triangle(subject,clip):
    """Projected polygon intersection; extrema then prove planar clearance."""
    polygon=[p.copy() for p in subject]
    for a,b in zip(clip,np.roll(clip,-1,axis=0)):
        if not polygon:break
        output=[];edge=b-a
        def distance(p):return edge[0]*(p[1]-a[1])-edge[1]*(p[0]-a[0])
        previous=polygon[-1];dp=distance(previous)
        for current in polygon:
            dc=distance(current)
            if (dp>=-1e-8)!=(dc>=-1e-8):
                output.append(previous+(current-previous)*(dp/(dp-dc)))
            if dc>=-1e-8:output.append(current)
            previous,dp=current,dc
        polygon=output
    return np.array(polygon)

def minimum_lens_clearance(body,glass):
    """Compare actual serialized planes, including triangle-edge crossings."""
    minimum=float('inf');closest=None
    lenses=[]
    for part in glass:
        points=part.positions.astype(float)
        plane=np.linalg.solve(np.c_[points[:,:2],np.ones(3)],points[:,2])
        lenses.append((points[:,:2],plane,points[:,:2].min(0),points[:,:2].max(0)))
    for part in body:
        points=part.positions.astype(float)
        # Anything below the lens's lowest edge is automatically clear. The
        # upward triangles are the upper envelope of the closed raised hands.
        if points[:,2].max()<106 or np.cross(points[1]-points[0],points[2]-points[0])[2]<=0:continue
        low,high=points[:,:2].min(0),points[:,:2].max(0)
        plane=np.linalg.solve(np.c_[points[:,:2],np.ones(3)],points[:,2])
        for xy,lens_plane,lens_low,lens_high in lenses:
            if np.any(low>lens_high) or np.any(high<lens_low):continue
            overlap=clip_triangle(points[:,:2],xy)
            if not len(overlap):continue
            delta=np.c_[overlap,np.ones(len(overlap))]@(lens_plane-plane)
            index=int(np.argmin(delta))
            if delta[index]<minimum:minimum=float(delta[index]);closest=overlap[index].tolist()
    assert minimum>4,('Glass intersects or crowds the raised hands',minimum,closest)
    return minimum,closest

def verify_glass_contract(assets):
    """Catch lost body art, opaque glass, wrong layer state and broken paired setup."""
    baseline=json.loads((Path(__file__).parent/'body-baseline.json').read_text())
    for name,digest in baseline['sha256'].items():
        assert hashlib.sha256(assets[name]).hexdigest()==digest,('Accepted body resource changed',name)
    required={'gNeiClockGlassTexDL','gNeiClockGlassDL','gNeiClockGlassGeoDL','gNeiClockGlassVtx','gNeiClockGlassTex'}
    assert required<=assets.keys(),('Missing native glass resources',sorted(required-assets.keys()))
    setup=ET.fromstring(assets['gNeiClockGlassTexDL']);draw=ET.fromstring(assets['gNeiClockGlassDL'])
    loads=[c for c in setup if c.tag=='LoadTextureBlock']
    assert len(loads)==1 and loads[0].attrib['Path']==NS+'gNeiClockGlassTex'
    assert all(loads[0].attrib[k]==v for k,v in {'Width':'32','Height':'32','Size':'3','Format':'0','MaskS':'5','MaskT':'5','CMS_TXWrap':'1','CMT_TXWrap':'1'}.items())
    assert not any(c.tag in ('LoadTextureBlock','SetTextureImage','SetTile','SetTileSize') for c in draw),'Glass draw would erase runtime scroll'
    assert not any(c.attrib.get('Path','').startswith('>') for root in (setup,draw) for c in root),'Glass must not acquire a segment dependency'
    modes=[c.attrib['Mode2'] for c in draw if c.tag=='SetRenderMode']
    assert modes[0]=='G_RM_AA_ZB_XLU_SURF2','Glass must depth-test without writing depth'
    assert any(c.tag=='SetGeometryMode' and c.attrib.get('G_CULL_BACK')=='1' for c in draw)
    assert any(c.tag=='ClearGeometryMode' and c.attrib.get('G_CULL_FRONT')=='1' for c in draw)
    combine=next(c for c in draw if c.tag=='SetCombineLERP').attrib
    assert (combine['A0'],combine['C0'],combine['Aa0'],combine['Ac0'])==('G_CCMUX_TEXEL0','G_CCMUX_PRIMITIVE','G_ACMUX_TEXEL0','G_ACMUX_PRIMITIVE')
    texture=np.frombuffer(assets['gNeiClockGlassTex'],dtype=np.uint8,offset=80).reshape(32,32,4)
    alpha=texture[:,:,3]
    assert 0<alpha.max()<=70,'Reflection would obscure the dial'
    assert np.mean(alpha==0)>.5,'Clear glass must remain exactly transparent outside the reflection'
    assert not np.any(alpha[:,[0,-1]]),'Reflection must cross the wrap boundary without a seam'
    vertices=ET.fromstring(assets['gNeiClockGlassVtx'])
    positions=np.array([[int(v.attrib[k]) for k in ('X','Y','Z')] for v in vertices])
    assert np.linalg.norm(positions[:,:2],axis=1).max()<336,'Lens escapes the inner brass bezel'
    assert positions[:,2].min()==106 and positions[:,2].max()==170
    return baseline

def main():
    assets,expected=build();baseline=verify_glass_contract(assets)
    manifest=json.loads((Path(__file__).parent/'manifest.json').read_text());assert manifest==expected
    for name,data in assets.items():
        for host in ('soh','mm'):assert (ROOT/host/'assets/custom'/NS/name).read_bytes()==data,(host,name)
        assert hashlib.sha256(data).hexdigest()==manifest['sha256'][name]
        if name.endswith('Tex'):
            magic=struct.unpack_from('<I',data,4)[0];typ,w,h,size=struct.unpack_from('<4I',data,64)
            assert magic==0x4f544558 and typ in (1,2) and size==w*h*(4 if typ==1 else 2) and len(data)==80+size and size<=4096
        else:
            root=ET.fromstring(data)
            if root.tag=='DisplayList':
                assert list(root)[-1].tag=='EndDisplayList';cache=set()
                for command in root:
                    a=command.attrib
                    if 'Path' in a:assert a['Path'].startswith(NS) and a['Path'][len(NS):] in assets
                    if command.tag=='LoadVertices':
                        offset,count,slot=[int(a[k]) for k in ('VertexOffset','Count','VertexBufferIndex')]
                        vertices=ET.fromstring(assets[a['Path'][len(NS):]])
                        assert 0<count<=30 and slot+count<=30 and offset+count<=len(vertices)
                        cache=set(range(slot,slot+count))
                    elif command.tag=='Triangle1':assert all(int(a[k]) in cache for k in ('V00','V01','V02'))
            elif root.tag=='Vertex':
                for v in root:
                    assert all(-32768<=int(v.attrib[k])<=32767 for k in ('X','Y','Z','S','T'))
                    assert all(0<=int(v.attrib[k])<=255 for k in 'RGBA')
    m=model('gNeiClock');assert len(m.parts)==manifest['submitted_triangles']
    body=model('gNeiClock',include_glass=False)
    assert len(body.parts)==baseline['submitted_triangles']==manifest['body']['submitted_triangles']
    assert len(body.sources)==14
    body_points=np.concatenate([p.positions for p in body.parts])
    assert [body_points.min(0).tolist(),body_points.max(0).tolist()]==baseline['bounds_native']==manifest['body']['bounds_native']
    glass=m.parts[len(body.parts):]
    assert len(glass)==120 and all(p.material.cull and p.material.unlit and not p.material.depth_write for p in glass)
    clearance,closest=minimum_lens_clearance(body.parts,glass)
    points=[]
    for p in m.parts:
        points.append(p.positions);cross=np.cross(p.positions[1]-p.positions[0],p.positions[2]-p.positions[0]);assert np.linalg.norm(cross)>0
        assert np.dot(cross,p.normals.mean(0))>0,'Winding disagrees with native normals'
        assert p.material.texture is not None
    points=np.concatenate(points);assert [points.min(0).tolist(),points.max(0).tolist()]==manifest['bounds_native']
    assert len(m.sources)==len(assets),'Unreachable or missing dependencies'
    for entrypoint in ('gNeiClockDL','gNeiClockGlassDL'):
        tail=list(ET.fromstring(assets[entrypoint]))[-9:]
        assert any(c.tag=='Texture' and c.attrib['On']=='0' for c in tail)
        assert any(c.tag=='ClearGeometryMode' and 'G_TEXTURE_GEN' in c.attrib and 'G_TEXTURE_GEN_LINEAR' in c.attrib for c in tail)
        assert any(c.tag=='SetRenderMode' and c.attrib['Mode2']=='G_RM_AA_ZB_OPA_SURF2' for c in tail)
        assert any(c.tag=='SetPrimColor' and all(c.attrib[k]=='255' for k in 'RGBA') for c in tail)
        assert any(c.tag=='SetEnvColor' and all(c.attrib[k]=='255' for k in 'RGBA') for c in tail)
        if entrypoint.endswith('GlassDL'):assert any(c.tag=='SetGeometryMode' and c.attrib.get('G_LIGHTING')=='1' for c in tail)
    phase32=model('gNeiClock',phase=32);phase128=model('gNeiClock',phase=128)
    alpha=[]
    for candidate in (m,phase32,phase128):
        values=[]
        for part in candidate.parts[len(body.parts):]:
            uv=part.uv.mean(0,keepdims=True)*np.array(part.material.tex_scale)/65536
            uv+=np.array(part.material.uv_offset)+.5/np.array(part.material.texture.shape[1::-1])
            values.append(sample_native(part.material.texture,uv,part.material.clamp)[0,3])
        alpha.append(np.array(values))
    assert np.max(abs(alpha[0]-alpha[1]))>.1,'Native tile offset does not move the reflection'
    assert np.allclose(alpha[0],alpha[2],atol=1e-12),'Native reflection does not wrap seamlessly'
    for candidate in (phase32,phase128):
        for original,shifted in zip(m.parts,candidate.parts):
            assert np.array_equal(original.positions,shifted.positions) and np.array_equal(original.uv,shifted.uv),'Scroll must change tile origin, never geometry or authored UVs'
    rear=lambda x:np.asarray(render(x,size=96,azimuth=180,elevation=-5,roll=0,supersample=1,fill=.9))
    assert np.array_equal(rear(body),rear(m)),'Front reflection leaks through the reverse'
    front=lambda x:np.asarray(render(x,size=96,azimuth=0,elevation=0,roll=0,supersample=1,fill=.9))
    assert np.any(front(body)!=front(m)),'Serialized glass has no visible reflection'
    print(f'PASS: {len(assets)} declared paired-entrypoint dependencies; two byte-identical hosts; {len(m.parts)} serialized triangles')
    print(f'PASS: all 14 accepted body hashes unchanged; body {len(body.parts)} submissions + lens {len(glass)}; exact minimum hand clearance {clearance:.6f} at XY {closest}')
    print('PASS: cache, signed UV, TMEM, outward normals, XLU/no-depth-write, low alpha, zero-alpha clear area, scroll movement/128-unit wrap, rear occlusion and exit state')
if __name__=='__main__':main()
