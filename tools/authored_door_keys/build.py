#!/usr/bin/env python3
"""Original deterministic key authoring; no donor geometry, textures or game data."""
from __future__ import annotations
import argparse, copy, hashlib, json, math, struct
from pathlib import Path
import xml.etree.ElementTree as ET
import numpy as np

def gaussian_filter(values, sigma, mode='wrap'):
    # Separable periodic Gaussian; NumPy is the only authoring dependency.
    assert mode == 'wrap'
    radius=int(4*sigma+.5)
    weights=np.exp(-np.arange(-radius,radius+1,dtype=float)**2/(2*sigma*sigma))
    weights/=weights.sum()
    result=values.copy()
    for axis in (0,1):
        result=sum(weight*np.roll(result,offset,axis=axis) for offset,weight in zip(range(-radius,radius+1),weights))
    return result

ROOT = Path(__file__).resolve().parents[2]
NS = 'objects/combo_door_keys/'
OWNERS = {'RustyKey':'mm', 'OverworldKey':'soh'}

def unit(v):
    v=np.array(v,dtype=float); return v/max(np.linalg.norm(v),1e-12)

class Mesh:
    def __init__(self): self.faces=[]
    def triangle(self, p, material, normals=None):
        p=np.rint(p).astype(int)
        n=np.cross(p[1]-p[0],p[2]-p[0])
        if np.linalg.norm(n)==0: raise ValueError(('quantized degenerate triangle',p))
        ns=[unit(n)]*3 if normals is None else normals
        self.faces.append((p,np.array(ns),material))
    def quad(self,a,b,c,d,material,normals=None):
        self.triangle([a,b,c],material,None if normals is None else [normals[i] for i in [0,1,2]])
        self.triangle([a,c,d],material,None if normals is None else [normals[i] for i in [0,2,3]])


def tube_ring(mesh, cx,cy,rx,ry,r,depth,material='Body',n=40):
    # Closed forged oval with a deliberately imperfect, but reproducible, outline.
    rings=[]; norms=[]
    for i in range(n):
        t=2*math.pi*i/n; outward=unit([math.cos(t)/rx,math.sin(t)/ry,0])
        center=np.array([cx+rx*math.cos(t),cy+ry*math.sin(t),0])
        row=[]; nr=[]
        for j in range(8):
            a=2*math.pi*j/8; rough=1+.045*math.sin(i*2.3+j*1.7)
            row.append(center+outward*r*math.cos(a)*rough+[0,0,depth*math.sin(a)])
            nr.append(unit(outward*math.cos(a)/r+[0,0,math.sin(a)/depth]))
        rings.append(row); norms.append(nr)
    for i in range(n):
        for j in range(8):
            k=(i+1)%n;l=(j+1)%8
            # Parametric order gives outward winding.
            mesh.quad(rings[i][j],rings[k][j],rings[k][l],rings[i][l],
                      'Edge' if j in (0,3,4,7) else material,
                      [norms[i][j],norms[k][j],norms[k][l],norms[i][l]])


def cylinder(mesh, yvalues, radii, sides=12):
    rings=[];ns=[]
    for y,r in zip(yvalues,radii):
        rings.append([[r*math.cos(2*math.pi*i/sides),y,r*math.sin(2*math.pi*i/sides)] for i in range(sides)])
    for j in range(len(rings)-1):
        slope=(radii[j]-radii[j+1])/(yvalues[j+1]-yvalues[j])
        for i in range(sides):
            k=(i+1)%sides
            normal=[unit([math.cos(2*math.pi*q/sides),slope,math.sin(2*math.pi*q/sides)]) for q in (i,i,k,k)]
            mesh.quad(rings[j][i],rings[j+1][i],rings[j+1][k],rings[j][k],
                      'Edge' if radii[j]!=radii[j+1] else 'Body',normal)
    for end,flip in [(0,False),(-1,True)]:
        for i in range(sides):
            p=[[0,yvalues[end],0],rings[end][i],rings[end][(i+1)%sides]]
            if flip:p=p[::-1]
            mesh.triangle(p,'Edge')


def cross2(a,b): return a[0]*b[1]-a[1]*b[0]
def inset(poly,d):
    out=[]
    for i,p in enumerate(poly):
        prev=unit(p-poly[i-1]);nxt=unit(poly[(i+1)%len(poly)]-p)
        a=np.array([-prev[1],prev[0]]);b=np.array([-nxt[1],nxt[0]])
        out.append(p+(a+b)*d/(1+np.dot(a,b)))
    return np.array(out)

def triangulate(poly):
    ids=list(range(len(poly)));result=[]
    def cross(a,b,c):return cross2(b-a,c-a)
    while len(ids)>3:
        for j,b in enumerate(ids):
            a=ids[j-1];c=ids[(j+1)%len(ids)]
            if cross(poly[a],poly[b],poly[c])<=1e-7:continue
            if any(min(cross(poly[a],poly[b],poly[q]),cross(poly[b],poly[c],poly[q]),cross(poly[c],poly[a],poly[q]))>=-1e-7 for q in ids if q not in (a,b,c)):continue
            result.append([a,b,c]);ids.pop(j);break
        else:raise ValueError('Cannot ear-clip polygon')
    result.append(ids);return result

def plate(mesh,points,halfdepth=7,bevel=2,material='Body'):
    p=np.array(points,dtype=float)
    if sum(cross2(p[i],p[(i+1)%len(p)]) for i in range(len(p)))<0:p=p[::-1]
    inner=inset(p,bevel)
    layers=[np.c_[inner,np.full(len(p),-halfdepth)],np.c_[p,np.full(len(p),-halfdepth+bevel)],np.c_[p,np.full(len(p),halfdepth-bevel)],np.c_[inner,np.full(len(p),halfdepth)]]
    for j in range(3):
        for i in range(len(p)):
            k=(i+1)%len(p)
            mesh.quad(layers[j][i],layers[j][k],layers[j+1][k],layers[j+1][i],'Edge' if j!=1 else material)
    for tri in triangulate(inner):
        mesh.triangle(layers[-1][tri],material)
        mesh.triangle(layers[0][tri[::-1]],material)


def brass_bow(mesh):
    outer=np.array([[-22,140],[22,140],[48,116],[48,83],[23,58],[-23,58],[-48,83],[-48,116]],float)[::-1]
    # Matching loops retain a true open hole, rather than a painted dark circle.
    outer=np.array([a+(b-a)*j/4 for a,b in zip(outer,np.roll(outer,-1,axis=0)) for j in range(4)])
    center=np.array([0,109]);delta=outer-center
    hole=np.array([center+unit(v)*np.array([15,13]) for v in delta])
    oi=center+(outer-center)*.94;hi=center+(hole-center)*1.17
    loops=[np.c_[outer,np.full(32,-5)],np.c_[oi,np.full(32,-8)],np.c_[hi,np.full(32,-8)],np.c_[hole,np.full(32,-5)],np.c_[hole,np.full(32,5)],np.c_[hi,np.full(32,8)],np.c_[oi,np.full(32,8)],np.c_[outer,np.full(32,5)]]
    for j in range(8):
        for i in range(32):
            k=(i+1)%32;l=(j+1)%8
            mesh.quad(loops[j][i],loops[l][i],loops[l][k],loops[j][k], 'Body' if j in (1,5) else 'Edge')
    # Raised maker's escutcheon and two engraved-looking inset bands are geometry.
    plate(mesh,[(-10,68),(10,68),(13,75),(10,82),(-10,82),(-13,75)],9,2,'Patina')


def models():
    rusty=Mesh()
    tube_ring(rusty,0,94,42,39,10,9)
    cylinder(rusty,[-140,-136,-124,36,40,44,49,54,59,63],[6,9,9,9,13,14,14,11,10,7])
    plate(rusty,[(6,-126),(47,-126),(47,-111),(34,-111),(34,-99),(48,-99),(48,-78),(29,-78),(29,-88),(19,-88),(19,-74),(6,-74)],8,2)
    brass=Mesh();brass_bow(brass)
    plate(brass,[(-13,64),(-13,-133),(-6,-140),(9,-140),(19,-130),(19,-116),(9,-106),(19,-96),(19,-85),(7,-75),(18,-64),(18,-51),(10,-44),(20,-33),(20,-19),(13,-12),(13,64)],6,2)
    # Long milled blade channel: a narrow, dark raised inset at both faces.
    for z in (-1,1):
        groove=Mesh();plate(groove,[(-8,-122),(-4,-125),(-4,49),(-8,49)],2,1,'Patina')
        for p,n,m in groove.faces:brass.faces.append((p+np.array([0,0,z*6]),n,m))
    return {'RustyKey':rusty,'OverworldKey':brass}


def texture(name,kind):
    rng=np.random.default_rng(70431+(0 if name=='RustyKey' else 100)+{'Body':0,'Edge':1,'Patina':2}[kind])
    h=w=32;grain=rng.random((h,w));noise=gaussian_filter(rng.random((h,w)),2,mode='wrap');noise=(noise-noise.min())/(noise.max()-noise.min())
    if name=='RustyKey':
        iron=np.array([66,64,57]) if kind=='Body' else np.array([103,101,87])
        rust=np.array([139,68,30]);amount=np.clip((noise-.29)*2.2,0,1)
        if kind=='Edge':amount*=.58
        rgb=iron[None,None,:]*(1-amount[:,:,None])+rust*amount[:,:,None]
        rgb*= (.68+.52*grain)[:,:,None]
        pits=grain<.075;rgb[pits]*=.37
    else:
        base=np.array({'Body':[156,118,49],'Edge':[202,166,87],'Patina':[58,70,47]}[kind])
        rgb=base[None,None,:]*(.72+.37*noise+.14*grain)[:,:,None]
        tarnish=np.clip((noise-.61)*3,0,.6)[:,:,None]
        rgb=rgb*(1-tarnish)+np.array([53,68,45])*tarnish
        rgb[grain<.045]*=.5
        # Fine interrupted linear scuffing, in the actual texture.
        rgb[::8]*=1.12
    rgb=np.uint16(np.clip(np.rint(rgb),0,255))
    rgba16=((rgb[:,:,0]>>3)<<11)|((rgb[:,:,1]>>3)<<6)|((rgb[:,:,2]>>3)<<1)|1
    header=struct.pack('<B3xIIQ',0,0x4f544558,0,0xDEADBEEFDEADBEEF).ljust(64,b'\0')
    return header+struct.pack('<4I',2,w,h,w*h*2)+rgba16.astype('>u2').tobytes()


def xml_bytes(root):
    ET.indent(root,space='  ');return (ET.tostring(root,encoding='unicode')+'\n').encode()
def cmd(root,tag,**a):return ET.SubElement(root,tag,{k:str(v) for k,v in a.items()})
def base_assets():
    assets={};stats={}
    for name,mesh in models().items():
        base=Path(OWNERS[name])/'assets/custom'/NS
        top=ET.Element('DisplayList',Version='0');cmd(top,'PipeSync')
        cmd(top,'ClearGeometryMode',G_TEXTURE_GEN=1,G_TEXTURE_GEN_LINEAR=1,G_CULL_FRONT=1,G_FOG=1)
        cmd(top,'SetGeometryMode',G_ZBUFFER=1,G_SHADE=1,G_SHADING_SMOOTH=1,G_CULL_BACK=1,G_LIGHTING=1)
        cmd(top,'SetOtherMode',Cmd='G_SETOTHERMODE_H',Sft=4,Length=20,G_AD_NOISE=1,G_CD_MAGICSQ=1,G_CK_NONE=1,G_CYC_2CYCLE=1,G_PM_NPRIMITIVE=1,G_TC_FILT=1,G_TD_CLAMP=1,G_TF_BILERP=1,G_TL_TILE=1,G_TP_PERSP=1,G_TT_NONE=1)
        cmd(top,'SetOtherMode',Cmd='G_SETOTHERMODE_L',Sft=0,Length=32,G_AC_NONE=1,G_RM_AA_ZB_OPA_SURF2=1,G_RM_PASS=1,G_ZS_PIXEL=1)
        cmd(top,'SetCombineLERP',A0='G_CCMUX_TEXEL0',B0='G_CCMUX_0',C0='G_CCMUX_SHADE',D0='G_CCMUX_0',Aa0='G_ACMUX_0',Ab0='G_ACMUX_0',Ac0='G_ACMUX_0',Ad0='G_ACMUX_1',A1='G_CCMUX_0',B1='G_CCMUX_0',C1='G_CCMUX_0',D1='G_CCMUX_COMBINED',Aa1='G_ACMUX_0',Ab1='G_ACMUX_0',Ac1='G_ACMUX_0',Ad1='G_ACMUX_COMBINED')
        cmd(top,'SetPrimColor',M=0,L=0,R=255,G=255,B=255,A=255);cmd(top,'SetEnvColor',R=255,G=255,B=255,A=255)
        cmd(top,'Texture',S=65535,T=65535,Level=0,Tile=0,On=1)
        for material in sorted({m for _,_,m in mesh.faces}):
            dl=ET.Element('DisplayList',Version='0');vertices=ET.Element('Vertex',Version='0')
            cmd(top,'CallDisplayList',Path=NS+name+material+'DL')
            cmd(dl,'PipeSync');cmd(dl,'LoadTextureBlock',Path=NS+name+material+'Tex',Format=0,Size=2,Width=32,Height=32,MaskS=5,MaskT=5,ShiftS=0,ShiftT=0,CMS_TXNoMirror=1,CMS_TXWrap=1,CMT_TXNoMirror=1,CMT_TXWrap=1)
            count=0
            for p,n,m in mesh.faces:
                if m!=material:continue
                # Stable object-space planar UV projection per dominant face axis.
                axis=int(np.argmax(np.abs(np.cross(p[1]-p[0],p[2]-p[0]))));uvaxes={0:[2,1],1:[0,2],2:[0,1]}[axis]
                for pos,normal in zip(p,n):
                    uv=np.rint(pos[uvaxes]*1024/84).astype(int);nn=np.rint(unit(normal)*127).astype(int)%256
                    cmd(vertices,'Vtx',X=pos[0],Y=pos[1],Z=pos[2],S=uv[0],T=uv[1],R=nn[0],G=nn[1],B=nn[2],A=255)
                count+=3
            for start in range(0,count,30):
                batch=min(30,count-start);cmd(dl,'LoadVertices',Path=NS+name+material+'Vtx',VertexBufferIndex=0,VertexOffset=start,Count=batch)
                for i in range(0,batch,3):cmd(dl,'Triangle1',V00=i,V01=i+1,V02=i+2)
            cmd(dl,'EndDisplayList')
            assets[base/(name+material+'DL')]=xml_bytes(dl);assets[base/(name+material+'Vtx')]=xml_bytes(vertices);assets[base/(name+material+'Tex')]=texture(name,material)
        cmd(top,'PipeSync');cmd(top,'Texture',S=65535,T=65535,Level=0,Tile=0,On=0)
        cmd(top,'ClearGeometryMode',G_TEXTURE_GEN=1,G_TEXTURE_GEN_LINEAR=1)
        cmd(top,'SetEnvColor',R=255,G=255,B=255,A=255);cmd(top,'EndDisplayList')
        assets[base/(name+'DL')]=xml_bytes(top)
        points=np.concatenate([p for p,_,_ in mesh.faces]);stats[name]={'owner':OWNERS[name],'root':NS+name+'DL','triangles':len(mesh.faces),'vertices':len(points),'bounds_native':[points.min(0).tolist(),points.max(0).tolist()],'bounds_draw_scale_0_25':[(points.min(0)*.25).tolist(),(points.max(0)*.25).tolist()]}
    return assets,stats

def rgba32_resource(pixels):
    # Native v0 RGBA32, exactly one 4 KiB tile, no custom HD/raw flags.
    header=struct.pack('<B3xIIQ',0,0x4f544558,0,0xDEADBEEFDEADBEEF).ljust(64,b'\0')
    h,w= pixels.shape[:2]
    return header+struct.pack('<4I',1,w,h,w*h*4)+pixels.astype(np.uint8).tobytes()

def weathering_coat(raw,name,material):
    values=np.frombuffer(raw,dtype='>u2',offset=80).reshape(32,32).astype(np.uint32)
    # Expand exactly as the native RGBA16 loader does: 5-bit values to 8 bits.
    rgb=np.stack([((values>>b)&31)*255//31 for b in (11,6,1)],axis=-1).astype(np.uint8)
    r,g,b=[rgb[:,:,i].astype(float) for i in range(3)]
    if name=='RustyKey':
        oxide=(r>g*1.25+5)|(r<38)
        opacity=np.full((32,32),185 if material=='Body' else 145,dtype=np.uint8)
        opacity[oxide]=255
    else:
        patina=(g>=r*.88)|(r<70)
        opacity=np.full((32,32),195 if material=='Body' else 155,dtype=np.uint8)
        opacity[patina]=255
        if material=='Patina':opacity[:]=255
    return rgba32_resource(np.dstack((rgb,opacity)))

def reflection_texture():
    # Original broad soft studio reflection, not an emissive or scrolling map.
    y,x=np.mgrid[0:16,0:16]/15
    band=np.exp(-((x-.34+.28*(y-.5))/.19)**2-((y-.46)/.52)**2)
    shoulder=.22*np.exp(-((x-.82)/.25)**2-((y-.2)/.4)**2)
    value=np.uint16(np.rint(np.clip(.055+.87*band+shoulder,0,1)*31))
    rgba=(value<<11)|(value<<6)|(value<<1)|1
    header=struct.pack('<B3xIIQ',0,0x4f544558,0,0xDEADBEEFDEADBEEF).ljust(64,b'\0')
    return header+struct.pack('<4I',2,16,16,512)+rgba.astype('>u2').tobytes()

def combine(parent,reflection=False):
    return cmd(parent,'SetCombineLERP',
        A0='G_CCMUX_PRIMITIVE' if reflection else 'G_CCMUX_TEXEL0',
        B0='G_CCMUX_ENVIRONMENT' if reflection else 'G_CCMUX_0',
        C0='G_CCMUX_TEXEL0' if reflection else 'G_CCMUX_SHADE',
        D0='G_CCMUX_ENVIRONMENT' if reflection else 'G_CCMUX_0',
        Aa0='G_ACMUX_0',Ab0='G_ACMUX_0',Ac0='G_ACMUX_0',
        Ad0='G_ACMUX_1' if reflection else 'G_ACMUX_TEXEL0',
        A1='G_CCMUX_COMBINED' if reflection else 'G_CCMUX_0',
        B1='G_CCMUX_0',C1='G_CCMUX_SHADE' if reflection else 'G_CCMUX_0',
        D1='G_CCMUX_0' if reflection else 'G_CCMUX_COMBINED',
        Aa1='G_ACMUX_0',Ab1='G_ACMUX_0',Ac1='G_ACMUX_0',Ad1='G_ACMUX_COMBINED')

def build_assets():
    assets,stats=base_assets()
    for name,owner in OWNERS.items():
        base=Path(owner)/'assets/custom'/NS
        top=ET.fromstring(assets[base/(name+'DL')]);calls=[c for c in top if c.tag=='CallDisplayList']
        # Draw every opaque component first; then every coplanar static weathering coat.
        reflection=ET.Element('DisplayList',Version='0');cmd(reflection,'PipeSync')
        cmd(reflection,'SetGeometryMode',G_TEXTURE_GEN=1,G_TEXTURE_GEN_LINEAR=1)
        cmd(reflection,'Texture',S=960,T=960,Level=0,Tile=0,On=1)
        combine(reflection,True)
        bright=(188,182,170) if name=='RustyKey' else (238,211,135)
        dark=(41,40,37) if name=='RustyKey' else (92,69,29)
        cmd(reflection,'SetPrimColor',M=0,L=0,**dict(zip('RGBA',(*bright,255))))
        cmd(reflection,'SetEnvColor',**dict(zip('RGBA',(*dark,255))))
        cmd(reflection,'LoadTextureBlock',Path=NS+name+'ReflectionTex',Format=0,Size=2,Width=16,Height=16,MaskS=0,MaskT=0,ShiftS=0,ShiftT=0,CMS_TXNoMirror=1,CMS_TXClamp=1,CMT_TXNoMirror=1,CMT_TXClamp=1)
        for call in calls:
            path=base/Path(call.attrib['Path']).name;dl=ET.fromstring(assets[path]);material=path.name[len(name):-2]
            # Geometry commands remain byte-identical, shared through existing vertex resources.
            for c in dl:
                if c.tag in ('LoadVertices','Triangle1'):reflection.append(copy.deepcopy(c))
            texpath=base/(name+material+'Tex')
            assets[texpath]=weathering_coat(assets[texpath],name,material)
            dl.find('LoadTextureBlock').attrib['Size']='3'
            assets[path]=xml_bytes(dl)
        cmd(reflection,'EndDisplayList')
        assets[base/(name+'ReflectionDL')]=xml_bytes(reflection)
        assets[base/(name+'ReflectionTex')]=reflection_texture()
        insertion=list(top).index(calls[0]);setup=ET.Element('Commands')
        cmd(setup,'CallDisplayList',Path=NS+name+'ReflectionDL');cmd(setup,'PipeSync')
        cmd(setup,'ClearGeometryMode',G_TEXTURE_GEN=1,G_TEXTURE_GEN_LINEAR=1)
        cmd(setup,'SetRenderMode',Mode1='G_RM_PASS',Mode2='G_RM_AA_ZB_XLU_DECAL2')
        cmd(setup,'Texture',S=65535,T=65535,Level=0,Tile=0,On=1)
        cmd(setup,'SetPrimColor',M=0,L=0,R=255,G=255,B=255,A=255)
        cmd(setup,'SetEnvColor',R=255,G=255,B=255,A=255);combine(setup)
        for i,c in enumerate(setup):top.insert(insertion+i,c)
        # Restore opaque render mode after the decal and all generated-UV state.
        top.insert(len(top)-1,ET.Element('SetRenderMode',Mode1='G_RM_PASS',Mode2='G_RM_AA_ZB_OPA_SURF2'))
        assets[base/(name+'DL')]=xml_bytes(top)
        stats[name]['unique_triangles']=stats[name]['triangles']
        stats[name]['submitted_triangles']=stats[name]['triangles']*2
        stats[name]['material_passes']=2
    return assets,stats

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--check',action='store_true');args=p.parse_args()
    assets,stats=build_assets()
    for path,data in assets.items():
        dest=ROOT/path
        if args.check:assert dest.read_bytes()==data, f'Regeneration differs: {path}'
        else:dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data)
    manifest={'recipe':'build.py','original_authorship':True,'draw_scale':.25,'models':stats,'sha256':{str(path):hashlib.sha256(data).hexdigest() for path,data in sorted(assets.items())}}
    target=Path(__file__).parent/'manifest.json';data=json.dumps(manifest,indent=2)+'\n'
    if args.check:assert target.read_text()==data
    else:target.write_text(data)
    print(json.dumps(stats,indent=2));print('Deterministic regeneration verified' if args.check else f'Wrote {len(assets)} native resources')
if __name__=='__main__':main()
