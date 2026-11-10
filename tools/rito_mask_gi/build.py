#!/usr/bin/env python3
"""Deterministic original Rito GI sculpture. No donor mesh or game data required."""
from __future__ import annotations
import argparse
import hashlib
import json
import math
from pathlib import Path
import shutil
import struct
import xml.etree.ElementTree as ET
import numpy as np

ROOT = Path(__file__).resolve().parents[2]
NS = 'objects/nei_gi_redesign/rito_mask/'
DRAW_SCALE = 1.0
MATRIX_SCALE = 13107 / 65536  # native 16.16 nearest representable 0.2
ICON_FILES = ('textures/icon_item_custom/gItemIconRitoMaskTex.rgba32.png',
              'textures/item_name_custom/gRitoMaskNameTex.ia4.png')


def unit(v):
    v = np.asarray(v, dtype=float)
    return v / max(np.linalg.norm(v), 1e-12)


class Mesh:
    def __init__(self):
        self.faces = []
        self.components = []

    def triangle(self, points, material):
        p = np.rint(points).astype(int)
        cross = np.cross(p[1] - p[0], p[2] - p[0])
        if np.linalg.norm(cross) == 0:
            raise ValueError(('quantized degenerate triangle', p))
        self.faces.append((p, np.tile(unit(cross), (3, 1)), material))

    def quad(self, a, b, c, d, material):
        self.triangle([a, b, c], material)
        self.triangle([a, c, d], material)

    def component(self, name, start):
        self.components.append({'name': name, 'first_triangle': start,
                                'triangles': len(self.faces) - start})


def surface_z(x, y):
    return 32 - .0016 * x*x - .0007 * (y + 5)**2


def eye_ring(mesh, side):
    """Closed bowed shell with a genuine almond opening, never a black decal."""
    start = len(mesh.faces)
    # Loop direction is CCW viewed from the mask's +Z front.
    # Authored angular feather lobes; each half terminates at the nasal bridge.
    outer = np.array([(114,-8),(106,26),(91,55),(66,73),(38,72),(15,56),
                      (4,30),(2,2),(9,-29),(22,-65),(41,-104),(67,-89),
                      (88,-68),(104,-37)], dtype=float)
    # Inner loop follows the same radial directions as the outer silhouette.
    center = np.array([48., 12.])
    inner = []
    for p in outer:
        d = p - center
        t = math.atan2(d[1]/18, d[0]/27)
        # Almond height pinches toward the two sharp corners.
        inner.append(center + [27*math.cos(t), 18*math.sin(t)*(.72+.28*abs(math.sin(t)))])
    inner = np.array(inner)
    if side < 0:
        outer[:,0] *= -1; inner[:,0] *= -1
        outer = outer[::-1]; inner = inner[::-1]
    # Outer bevel, front field, eye bevel, aperture wall, rear lining, outer wall.
    middle = inner + .26*(outer-inner)
    loops = []
    for xy, dz in ((outer,-3),(outer,2),(middle,4),(inner,0),(inner,-12),(outer,-12)):
        loops.append(np.array([[x,y,surface_z(x,y)+dz] for x,y in xy]))
    for j in range(len(loops)):
        k = (j+1) % len(loops)
        # Front ring outward->inward, return along the concave back.
        material = ('IvoryEdge' if j in (0,2,3,5) else 'Ivory' if j==1 else 'Lining')
        for i in range(len(outer)):
            n=(i+1)%len(outer)
            mesh.quad(loops[j][i],loops[j][n],loops[k][n],loops[k][i],material)
    mesh.component(('left' if side < 0 else 'right') + '_eye_shell', start)
    # Raised narrow gold eye surround, independently closed and beveled.
    start = len(mesh.faces)
    # Work directly in the correctly mirrored inner coordinates.
    c = np.array([48*side,12.]); outside=c+(inner-c)*1.13
    loops=[]
    for xy,dz in ((outside,2),(outside,5),(inner,5),(inner,2)):
        loops.append(np.array([[x,y,surface_z(x,y)+dz] for x,y in xy]))
    for j in range(4):
        for i in range(len(inner)):
            n=(i+1)%len(inner);k=(j+1)%4
            mesh.quad(loops[j][i],loops[j][n],loops[k][n],loops[k][i],'Gold')
    mesh.component(('left' if side < 0 else 'right') + '_eye_rim',start)


def convex_piece(mesh, name, outline, front, back, material):
    """Closed convex relief with bevelled perimeter and a raised central ridge."""
    start=len(mesh.faces)
    xy=np.array(outline,float)
    area=sum(xy[i,0]*xy[(i+1)%len(xy),1]-xy[i,1]*xy[(i+1)%len(xy),0] for i in range(len(xy)))
    if area<0:xy=xy[::-1]
    center=xy.mean(0)
    outer=np.c_[xy,np.full(len(xy),back+2)]
    bevel=np.c_[center+(xy-center)*.88,np.full(len(xy),front-2)]
    peak=np.r_[center,front]
    for i in range(len(xy)):
        n=(i+1)%len(xy)
        mesh.quad(outer[i],outer[n],bevel[n],bevel[i],material)
        mesh.triangle([bevel[i],bevel[n],peak],material)
        mesh.triangle([np.r_[center,back],outer[n],outer[i]],'Lining' if material=='Ivory' else 'GoldEdge')
    mesh.component(name,start)


def beak(mesh):
    start=len(mesh.faces)
    # Hooked six-sided cross sections: frontal keel extends far beyond the shell.
    rings=[]
    for y,w,rear,ridge in [(32,10,22,47),(5,22,21,93),(-42,17,5,88),(-88,2,-3,32)]:
        rings.append(np.array([[-w,y,rear],[-w,y,(rear+ridge)*.50],[0,y,ridge],
                               [w,y,(rear+ridge)*.50],[w,y,rear],[0,y,rear-3]],float))
    # Ring orientation is clockwise around +Y; longitudinal order goes down -Y.
    for j in range(len(rings)-1):
        for i in range(6):
            n=(i+1)%6
            mesh.quad(rings[j][i],rings[j+1][i],rings[j+1][n],rings[j][n],
                      'Gold' if i in (0,1,2,3) else 'GoldEdge')
    for end,reverse in [(0,False),(-1,True)]:
        c=rings[end].mean(0)
        for i in range(6):
            p=[c,rings[end][i],rings[end][(i+1)%6]]
            mesh.triangle(p[::-1] if reverse else p,'GoldEdge')
    mesh.component('hooked_gold_beak',start)


def model():
    mesh=Mesh()
    for side in (-1,1):eye_ring(mesh,side)
    # Five separately sculpted crest vanes with ivory bases and gold lance tips.
    for index,(x,tipx,tipy,width) in enumerate([(-49,-77,112,12),(-27,-45,133,14),(0,0,145,15),(27,45,133,14),(49,77,112,12)]):
        midx=x+(tipx-x)*.52;midy=53+(tipy-53)*.52
        convex_piece(mesh,f'crest_{index}',[(x-4,53),(x+4,53),(midx+width,midy),(tipx,tipy),(midx-width,midy)],
                     14,-4,'Gold')
    convex_piece(mesh,'ivory_forehead',[(-36,58),(0,81),(36,58),(21,33),(-21,33)],40,19,'Ivory')
    beak(mesh)
    # Raised temple rivets and rear mounting pads make the wearable back legible.
    for side in (-1,1):
        x=91*side
        convex_piece(mesh,f'temple_mount_{side}',[(x-6,-17),(x+6,-17),(x+7,-6),(x,0),(x-7,-6)],
                     20,-10,'GoldEdge')
    # Center the total silhouette around the source origin for NEI rotation/fit.
    points=np.concatenate([p for p,_,_ in mesh.faces]);offset=np.rint((points.min(0)+points.max(0))/2).astype(int)
    mesh.faces=[(p-offset,n,m) for p,n,m in mesh.faces]
    return mesh,offset


def resource_header(kind):
    return struct.pack('<B3xIIQ',0,kind,0,0xDEADBEEFDEADBEEF).ljust(64,b'\0')


def matrix():
    values=[int(MATRIX_SCALE*65536) if i in (0,5,10) else (65536 if i==15 else 0) for i in range(16)]
    half=[((v>>16)&65535) for v in values];fraction=[v&65535 for v in values]
    words=[(a<<16)|b for data in (half,fraction) for a,b in zip(data[::2],data[1::2])]
    return resource_header(0x4f4d5458)+struct.pack('<16I',*words)


def texture(material):
    y,x=np.mgrid[0:32,0:32]
    if material=='Gold':
        band=np.exp(-((x/31-.34+.2*(y/31-.5))/.24)**2-((y/31-.45)/.65)**2)
        gray=np.clip(.10+.85*band,0,1)
        rgb=np.repeat(gray[:,:,None],3,axis=2)*255
    else:
        base={'Ivory':[248,239,203],'IvoryEdge':[212,195,147],
              'Lining':[151,130,88],'GoldEdge':[148,100,29]}[material]
        variation=.97+.025*np.sin(x*.7+y*.4)+.015*np.cos(y*.9-x*.3)
        rgb=np.asarray(base)[None,None,:]*variation[:,:,None]
    rgb=np.uint16(np.clip(np.rint(rgb),0,255))
    rgba=((rgb[:,:,0]>>3)<<11)|((rgb[:,:,1]>>3)<<6)|((rgb[:,:,2]>>3)<<1)|1
    return resource_header(0x4f544558)+struct.pack('<4I',2,32,32,2048)+rgba.astype('>u2').tobytes()


def cmd(root,tag,**attrs):
    return ET.SubElement(root,tag,{k:str(v) for k,v in attrs.items()})


def xml(root):
    ET.indent(root,space='  ')
    return (ET.tostring(root,encoding='unicode')+'\n').encode()


def combine(root,reflection=False):
    cmd(root,'SetCombineLERP',A0='G_CCMUX_PRIMITIVE' if reflection else 'G_CCMUX_TEXEL0',B0='G_CCMUX_ENVIRONMENT' if reflection else 'G_CCMUX_0',C0='G_CCMUX_TEXEL0' if reflection else 'G_CCMUX_SHADE',D0='G_CCMUX_ENVIRONMENT' if reflection else 'G_CCMUX_0',Aa0='G_ACMUX_0',Ab0='G_ACMUX_0',Ac0='G_ACMUX_0',Ad0='G_ACMUX_1',A1='G_CCMUX_COMBINED' if reflection else 'G_CCMUX_0',B1='G_CCMUX_0',C1='G_CCMUX_SHADE' if reflection else 'G_CCMUX_0',D1='G_CCMUX_0' if reflection else 'G_CCMUX_COMBINED',Aa1='G_ACMUX_0',Ab1='G_ACMUX_0',Ac1='G_ACMUX_0',Ad1='G_ACMUX_COMBINED')


def build_assets():
    mesh,offset=model()
    dl=ET.Element('DisplayList',Version='0');vertices=ET.Element('Vertex',Version='0')
    cmd(dl,'PipeSync');cmd(dl,'Matrix',Path=NS+'scale_mtx',Param='G_MTX_PUSH')
    cmd(dl,'SetCycleType',G_CYC_2CYCLE=1);cmd(dl,'SetAlphaCompare',Mode=0);cmd(dl,'SetTextureLUT',Mode='G_TT_NONE')
    cmd(dl,'ClearGeometryMode',G_CULL_FRONT=1,G_TEXTURE_GEN=1,G_TEXTURE_GEN_LINEAR=1,G_FOG=1)
    cmd(dl,'SetGeometryMode',G_ZBUFFER=1,G_SHADE=1,G_SHADING_SMOOTH=1,G_LIGHTING=1,G_CULL_BACK=1)
    cmd(dl,'SetRenderMode',Mode1='G_RM_PASS',Mode2='G_RM_AA_ZB_OPA_SURF2')
    assets={'scale_mtx':matrix()};count=0
    for material in ('Lining','IvoryEdge','Ivory','GoldEdge','Gold'):
        cmd(dl,'PipeSync')
        cmd(dl,'SetGeometryMode' if material=='Gold' else 'ClearGeometryMode',G_TEXTURE_GEN=1,G_TEXTURE_GEN_LINEAR=1)
        cmd(dl,'Texture',S=1984 if material=='Gold' else 65535,T=1984 if material=='Gold' else 65535,Level=0,Tile=0,On=1)
        cmd(dl,'SetPrimColor',M=0,L=0,R=255,G=219 if material=='Gold' else 255,B=119 if material=='Gold' else 255,A=255)
        cmd(dl,'SetEnvColor',R=112 if material=='Gold' else 255,G=68 if material=='Gold' else 255,B=16 if material=='Gold' else 255,A=255)
        combine(dl,material=='Gold')
        tex=material.lower()+'_tex';assets[tex]=texture(material)
        cmd(dl,'LoadTextureBlock',Path=NS+tex,Format=0,Size=2,Width=32,Height=32,MaskS=0 if material=='Gold' else 5,MaskT=0 if material=='Gold' else 5,ShiftS=0,ShiftT=0,CMS_TXNoMirror=1,**({'CMS_TXClamp':1,'CMT_TXClamp':1} if material=='Gold' else {'CMS_TXWrap':1,'CMT_TXWrap':1}),CMT_TXNoMirror=1)
        start=count
        for p,n,m in mesh.faces:
            if m!=material:continue
            for pos,normal in zip(p,n):
                uv=np.rint(pos[:2]*1024/100).astype(int);nn=np.rint(normal*127).astype(int)%256
                cmd(vertices,'Vtx',X=pos[0],Y=pos[1],Z=pos[2],S=uv[0],T=uv[1],R=nn[0],G=nn[1],B=nn[2],A=255)
            count+=3
        for begin in range(start,count,30):
            batch=min(30,count-begin)
            cmd(dl,'LoadVertices',Path=NS+'mesh_opa_vtx',VertexBufferIndex=0,VertexOffset=begin,Count=batch)
            for i in range(0,batch,3):cmd(dl,'Triangle1',V00=i,V01=i+1,V02=i+2)
    cmd(dl,'PipeSync');cmd(dl,'Texture',S=0,T=0,Level=0,Tile=0,On=0)
    cmd(dl,'ClearGeometryMode',G_TEXTURE_GEN=1,G_TEXTURE_GEN_LINEAR=1)
    cmd(dl,'SetPrimColor',M=0,L=0,R=255,G=255,B=255,A=255);cmd(dl,'SetEnvColor',R=255,G=255,B=255,A=255)
    cmd(dl,'PopMatrix',Param='G_MTX_MODELVIEW');cmd(dl,'EndDisplayList')
    assets['gi_dl']=xml(dl);assets['mesh_opa_vtx']=xml(vertices)
    points=np.concatenate([p for p,_,_ in mesh.faces]);scaled=points*MATRIX_SCALE
    metadata={'root':'__OTR__'+NS+'gi_dl','draw_scale':DRAW_SCALE,'matrix_scale':MATRIX_SCALE,
              'orientation':'front +Z; crest +Y; no local roll','triangles':len(mesh.faces),'vertices':count,
              'bounds_native':[points.min(0).tolist(),points.max(0).tolist()],
              'bounds_after_internal_matrix':[scaled.min(0).tolist(),scaled.max(0).tolist()],
              'rotation_width':float(2*np.max(np.linalg.norm(scaled[:,[0,2]],axis=1))),
              'source_center_offset':offset.tolist(),'components':mesh.components,
              'runtime_tested':False}
    return assets,metadata


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--check',action='store_true');args=parser.parse_args()
    assets,metadata=build_assets()
    for owner in ('soh','mm'):
        dest=ROOT/owner/'assets/custom'/NS
        if not args.check:dest.mkdir(parents=True,exist_ok=True)
        for name,data in assets.items():
            if args.check:assert (dest/name).read_bytes()==data,f'Regeneration differs: {dest/name}'
            else:(dest/name).write_bytes(data)
    for path in ICON_FILES:
        source=ROOT/'soh/assets/custom'/path;dest=ROOT/'mm/assets/custom'/path
        if args.check:assert source.read_bytes()==dest.read_bytes(),f'Icon/name mismatch: {path}'
        else:dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,dest)
    metadata['sha256']={name:hashlib.sha256(data).hexdigest() for name,data in sorted(assets.items())}
    metadata['preserved_icon_sha256']={p:hashlib.sha256((ROOT/'soh/assets/custom'/p).read_bytes()).hexdigest() for p in ICON_FILES}
    data=json.dumps(metadata,indent=2)+'\n';manifest=Path(__file__).parent/'manifest.json'
    if args.check:assert manifest.read_text()==data,'Manifest regeneration differs'
    else:manifest.write_text(data)
    print(json.dumps({k:v for k,v in metadata.items() if k not in ('sha256','components','preserved_icon_sha256')},indent=2))
    print('Deterministic resources and host parity verified' if args.check else f'Wrote {len(assets)} native resources to each host')

if __name__=='__main__':main()
