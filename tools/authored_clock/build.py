#!/usr/bin/env python3
"""Deterministic original Hylian pocket timepiece; native XML + original textures."""
from __future__ import annotations
import argparse, hashlib, json, math, struct, sys, copy
from pathlib import Path
import xml.etree.ElementTree as ET
import numpy as np
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT))
from tools.authored_door_keys.build import Mesh, unit, plate, cmd, xml_bytes, rgba32_resource, reflection_texture, combine, triangulate, cross2
NS='objects/object_nei_clock/'
BODY_BASELINE='5cd05ecf71a05e80030a199a58974a1184641ed3'

def make_glass_mesh():
    """One outward-facing lens surface, nested inside the accepted brass lip."""
    mesh=Mesh();rings=[];normals=[];radius=335
    for r in (120,240,335):
        z=170-64*(r/radius)**2
        ring=[];ns=[]
        for i in range(24):
            angle=2*math.pi*i/24;x=r*math.cos(angle);y=r*math.sin(angle)
            ring.append([x,y,z]);ns.append(unit([128*x/radius**2,128*y/radius**2,1]))
        rings.append(ring);normals.append(ns)
    for i in range(24):
        j=(i+1)%24
        mesh.triangle([[0,0,170],rings[0][i],rings[0][j]],'Glass',[[0,0,1],normals[0][i],normals[0][j]])
        for r in (0,1):
            mesh.quad(rings[r][i],rings[r+1][i],rings[r+1][j],rings[r][j],'Glass',
                      [normals[r][i],normals[r+1][i],normals[r+1][j],normals[r][j]])
    return mesh

def glass_texture():
    """A scalar raised-cosine reflection ramp; the rest is truly transparent."""
    _,x=np.mgrid[:32,:32]
    q=np.clip(abs(x-16)/4.75,0,1)
    alpha=np.rint(70*np.cos(q*math.pi/2)**2).astype(np.uint8)
    rgb=np.broadcast_to(np.array([223,242,255],dtype=np.uint8),(32,32,3))
    return rgba32_resource(np.dstack([rgb,alpha]))

def glass_assets():
    """Paired native entrypoints: load texture, caller scrolls tile 0, draw lens.

    The caller inserts gDPSetTileSizeLerp between TexDL and GlassDL. No segment
    is borrowed, and GlassDL never reloads the texture or resets the tile size.
    """
    mesh=make_glass_mesh();vtx=ET.Element('Vertex',Version='0');geo=ET.Element('DisplayList',Version='0')
    for points,normals,_ in mesh.faces:
        for p,n in zip(points,normals):
            # Diagonal direction is authored in the native UVs, not the preview.
            uv=np.rint([512+1024*(.88*p[0]+.46*p[1])/1000,512+1024*p[1]/670]).astype(int)
            nn=np.rint(unit(n)*127).astype(int)%256
            cmd(vtx,'Vtx',X=p[0],Y=p[1],Z=p[2],S=uv[0],T=uv[1],R=nn[0],G=nn[1],B=nn[2],A=255)
    count=3*len(mesh.faces)
    for start in range(0,count,30):
        batch=min(30,count-start);cmd(geo,'LoadVertices',Path=NS+'gNeiClockGlassVtx',VertexBufferIndex=0,VertexOffset=start,Count=batch)
        for i in range(0,batch,3):cmd(geo,'Triangle1',V00=i,V01=i+1,V02=i+2)
    cmd(geo,'EndDisplayList')
    setup=ET.Element('DisplayList',Version='0');cmd(setup,'PipeSync')
    cmd(setup,'LoadTextureBlock',Path=NS+'gNeiClockGlassTex',Format=0,Size=3,Width=32,Height=32,MaskS=5,MaskT=5,ShiftS=0,ShiftT=0,CMS_TXNoMirror=1,CMS_TXWrap=1,CMT_TXNoMirror=1,CMT_TXWrap=1)
    cmd(setup,'EndDisplayList')
    top=ET.Element('DisplayList',Version='0');cmd(top,'PipeSync')
    cmd(top,'ClearGeometryMode',G_TEXTURE_GEN=1,G_TEXTURE_GEN_LINEAR=1,G_CULL_FRONT=1,G_FOG=1,G_LIGHTING=1)
    cmd(top,'SetGeometryMode',G_ZBUFFER=1,G_SHADE=1,G_SHADING_SMOOTH=1,G_CULL_BACK=1)
    cmd(top,'SetOtherMode',Cmd='G_SETOTHERMODE_H',Sft=4,Length=20,G_AD_NOISE=1,G_CD_MAGICSQ=1,G_CK_NONE=1,G_CYC_2CYCLE=1,G_PM_NPRIMITIVE=1,G_TC_FILT=1,G_TD_CLAMP=1,G_TF_BILERP=1,G_TL_TILE=1,G_TP_PERSP=1,G_TT_NONE=1)
    cmd(top,'SetOtherMode',Cmd='G_SETOTHERMODE_L',Sft=0,Length=32,G_AC_NONE=1,G_RM_AA_ZB_XLU_SURF2=1,G_RM_PASS=1,G_ZS_PIXEL=1)
    cmd(top,'SetRenderMode',Mode1='G_RM_PASS',Mode2='G_RM_AA_ZB_XLU_SURF2')
    cmd(top,'Texture',S=65535,T=65535,Level=0,Tile=0,On=1)
    cmd(top,'SetPrimColor',M=0,L=0,R=255,G=255,B=255,A=255);cmd(top,'SetEnvColor',R=255,G=255,B=255,A=255)
    cmd(top,'SetCombineLERP',A0='G_CCMUX_TEXEL0',B0='G_CCMUX_0',C0='G_CCMUX_PRIMITIVE',D0='G_CCMUX_0',
        Aa0='G_ACMUX_TEXEL0',Ab0='G_ACMUX_0',Ac0='G_ACMUX_PRIMITIVE',Ad0='G_ACMUX_0',
        A1='G_CCMUX_0',B1='G_CCMUX_0',C1='G_CCMUX_0',D1='G_CCMUX_COMBINED',
        Aa1='G_ACMUX_0',Ab1='G_ACMUX_0',Ac1='G_ACMUX_0',Ad1='G_ACMUX_COMBINED')
    cmd(top,'CallDisplayList',Path=NS+'gNeiClockGlassGeoDL');cmd(top,'PipeSync')
    cmd(top,'Texture',S=65535,T=65535,Level=0,Tile=0,On=0)
    cmd(top,'ClearGeometryMode',G_TEXTURE_GEN=1,G_TEXTURE_GEN_LINEAR=1)
    cmd(top,'SetGeometryMode',G_LIGHTING=1)
    cmd(top,'SetRenderMode',Mode1='G_RM_PASS',Mode2='G_RM_AA_ZB_OPA_SURF2')
    cmd(top,'SetPrimColor',M=0,L=0,R=255,G=255,B=255,A=255);cmd(top,'SetEnvColor',R=255,G=255,B=255,A=255)
    cmd(top,'EndDisplayList')
    return {'gNeiClockGlassTexDL':xml_bytes(setup),'gNeiClockGlassDL':xml_bytes(top),
            'gNeiClockGlassGeoDL':xml_bytes(geo),'gNeiClockGlassVtx':xml_bytes(vtx),'gNeiClockGlassTex':glass_texture()},mesh

def lathe(mesh, profile, material, n=24, center=(0,0,0)):
    """Closed axial profile revolved about Z, clockwise in (radius,Z)."""
    for j,(r,z) in enumerate(profile):
        rr,zz=profile[(j+1)%len(profile)]
        if r==rr==0 or (r==rr and z==zz):continue
        for i in range(n):
            a,b=2*math.pi*i/n,2*math.pi*(i+1)/n
            p=np.array([[r*math.cos(a),r*math.sin(a),z],[rr*math.cos(a),rr*math.sin(a),zz],[rr*math.cos(b),rr*math.sin(b),zz],[r*math.cos(b),r*math.sin(b),z]])+center
            ns=[unit([(zz-z)*math.cos(t),(zz-z)*math.sin(t),r-rr]) for t in (a,a,b,b)]
            if r==0:mesh.triangle(p[[0,2,1]],material,[ns[k] for k in [0,2,1]])
            elif rr==0:mesh.triangle(p[[0,3,1]],material,[ns[k] for k in [0,3,1]])
            else:mesh.quad(*p[[0,3,2,1]],material,[ns[k] for k in [0,3,2,1]])

def shape(mesh,xy,z,depth,mat,bevel=2):
    part=Mesh()
    if bevel<1:
        points=np.rint(xy).astype(int)
        points=np.array([v for i,v in enumerate(points) if not np.array_equal(v,points[i-1])])
        if sum(cross2(points[i],points[(i+1)%len(points)]) for i in range(len(points)))<0:points=points[::-1]
        lo=np.c_[points,np.full(len(points),-depth/2)];hi=np.c_[points,np.full(len(points),depth/2)]
        for i in range(len(points)):
            j=(i+1)%len(points);part.quad(lo[i],lo[j],hi[j],hi[i],mat)
        for tri in triangulate(points):
            part.triangle(hi[tri],mat);part.triangle(lo[tri[::-1]],mat)
    else:plate(part,xy,depth/2,bevel,mat)
    # plate uses Edge for bevels; retain intended enamel/ivory or metal bevel.
    for p,n,m in part.faces:mesh.faces.append((p+[0,0,z],n,('Brass' if mat=='Brass' else mat) if m=='Edge' else m))

def disk(mesh,r,z,depth,mat,center=(0,0,0),n=24):
    lathe(mesh,[(0,z-depth/2),(r,z-depth/2),(r,z+depth/2),(0,z+depth/2)],mat,n,center)

def annulus(mesh,ri,ro,z,mat,n=24):
    # Rear decorative inlay: only its visible face is needed, flush with the back.
    for i in range(n):
        a,b=2*math.pi*i/n,2*math.pi*(i+1)/n
        mesh.quad([ri*math.cos(a),ri*math.sin(a),z],[ro*math.cos(a),ro*math.sin(a),z],[ro*math.cos(b),ro*math.sin(b),z],[ri*math.cos(b),ri*math.sin(b),z],mat)

def rotated(points,angle,center=(0,0)):
    a=math.radians(angle);rot=np.array([[math.cos(a),-math.sin(a)],[math.sin(a),math.cos(a)]])
    return np.array(points)@rot.T+center

def make_mesh():
    m=Mesh()
    # Substantial rounded housing, fine lip, inset dial and narrow enamel waist.
    lathe(m,[(0,-72),(344,-88),(378,-77),(399,-48),(400,37),(388,74),(363,99),(342,99),(335,79),(335,67),(0,67)],'Brass',24)
    lathe(m,[(399,-28),(403,-24),(403,12),(399,17)],'Blue',24)
    lathe(m,[(347,94),(361,106),(353,114),(341,102)],'Brass',24)
    disk(m,332,78,12,'Ivory',n=24)
    annulus(m,310,314,89,'Brass',24)
    # Twelve raised hour lozenges and small minute divisions.
    for i in range(12):
        a=-i*30
        shape(m,rotated([(-7,267),(7,267),(10,290),(0,301),(-10,290)],a),96,10,'Blue',.5)
    for i in range(60):
        if i%5==0:continue
        a=math.radians(i*6);c=np.array([math.sin(a),math.cos(a)])*305
        # Flat but physically raised 4-vertex markers; depth comes from the dial.
        pts=rotated([(-2,-5),(2,-5),(2,5),(-2,5)],-i*6,c)
        m.quad(*np.c_[pts,np.full(4,90)],'Dark')
    # Two independent raised sword-shaped hands, showing 10:10.
    shape(m,rotated([(-8,-42),(8,-42),(13,125),(24,138),(0,194),(-24,138),(-13,125)],50),110,12,'Blue',2)
    shape(m,rotated([(-6,-48),(6,-48),(9,178),(17,188),(0,244),(-17,188),(-9,178)],-60),126,10,'Blue',2)
    disk(m,21,136,12,'Brass',n=16)
    disk(m,9,144,6,'Blue',n=12)
    # Sun/moon inset: physical navy oval, gold sun and ivory crescent.
    disk(m,57,91,10,'Blue',center=(0,-164,0),n=24)
    disk(m,17,99,6,'Brass',center=(-23,-164,0),n=12)
    for i in range(8):
        shape(m,rotated([(-2,23),(2,23),(0,31)],i*45,(-23,-164)),100,4,'Brass',.5)
    crescent=[]
    for a in np.linspace(60,300,13):crescent.append((24+24*math.cos(math.radians(a)),-164+24*math.sin(math.radians(a))))
    for a in np.linspace(270,90,9):crescent.append((36+21*math.cos(math.radians(a)),-164+21*math.sin(math.radians(a))))
    shape(m,crescent,101,5,'Ivory',.5)
    # Tiny triangular maker mark near twelve, below the raised index.
    for cx,cy in [(0,226),(-10,209),(10,209)]:shape(m,[(cx,cy+9),(cx-9,cy-7),(cx+9,cy-7)],94,4,'Brass',.6)
    # Crown with ribbed grip; rotate local axial metal into the vertical Y axis.
    c=Mesh();lathe(c,[(0,375),(32,375),(32,406),(48,414),(48,457),(32,470),(0,470)],'Brass',10)
    for p,n,mat in c.faces:m.faces.append((p[:,[0,2,1]]*np.array([1,1,-1]),n[:,[0,2,1]]*np.array([1,1,-1]),mat))
    for y in [422,433,444]:
        c=Mesh();lathe(c,[(46,y-2),(51,y),(46,y+5)],'Brass',10)
        for p,n,mat in c.faces:m.faces.append((p[:,[0,2,1]]*np.array([1,1,-1]),n[:,[0,2,1]]*np.array([1,1,-1]),mat))
    # Open suspension ring, oval and square-section bevelled brass.
    ring=[];N=16
    for i in range(N):
        t=2*math.pi*i/N;row=[]
        for radius,z in [(1,-11),(1.08,0),(1,11),(.76,11),(.70,0),(.76,-11)]:row.append([75*radius*math.cos(t),520+75*radius*math.sin(t),z])
        ring.append(row)
    for i in range(N):
        for j in range(6):m.quad(ring[i][j],ring[(i+1)%N][j],ring[(i+1)%N][(j+1)%6],ring[i][(j+1)%6],'Brass')
    # Finished inset rear, double border and raised crest, all faces outward -Z.
    back=Mesh();disk(back,303,91,10,'Blue',n=24)
    annulus(back,286,291,101,'Brass',24)
    annulus(back,252,255,100,'Brass',24)
    for x,y in [(0,66),(-70,-55),(70,-55)]:shape(back,[(x,y+71),(x-64,y-40),(x+64,y-40)],106,12,'Brass',.5)
    # Paired stylized feather fans surround the crest without copying donor art.
    for side in (-1,1):
        for j in range(3):
            shape(back,[(side*(113+j*18),35-j*29),(side*(216+j*7),89-j*31),(side*(190+j*9),27-j*30),(side*(111+j*15),-5-j*28)],104,8,'Brass',.5)
    for p,n,mat in back.faces:m.faces.append((p*np.array([-1,1,-1]),n*np.array([-1,1,-1]),mat))
    return m

def texture(mat):
    y,x=np.mgrid[:32,:32];rng=np.random.default_rng(20261109+sum(map(ord,mat)))
    noise=rng.random((32,32));wave=(np.sin(x*.37+np.cos(y*.3))+np.cos(y*.31+x*.14))/4+.5
    base={'Brass':[191,149,71],'Blue':[22,58,83],'Ivory':[238,223,182],'Dark':[74,66,46]}[mat]
    rgb=np.array(base)*( .88+.11*wave[:,:,None]+.06*noise[:,:,None])
    if mat=='Brass':
        patina=np.clip((wave-.64)*1.8,0,.36)[:,:,None];rgb=rgb*(1-patina)+np.array([48,78,61])*patina
        rgb[noise<.025]*=.58
        alpha=np.full((32,32),142,dtype=np.uint8);alpha[patina[:,:,0]>.15]=225;alpha[noise<.025]=255
    else:alpha=np.full((32,32),255,dtype=np.uint8)
    return rgba32_resource(np.dstack([np.clip(rgb,0,255).astype(np.uint8),alpha]))

def build():
    mesh=make_mesh();assets={};top=ET.Element('DisplayList',Version='0');cmd(top,'PipeSync')
    cmd(top,'ClearGeometryMode',G_TEXTURE_GEN=1,G_TEXTURE_GEN_LINEAR=1,G_CULL_FRONT=1,G_FOG=1)
    cmd(top,'SetGeometryMode',G_ZBUFFER=1,G_SHADE=1,G_SHADING_SMOOTH=1,G_CULL_BACK=1,G_LIGHTING=1)
    cmd(top,'SetOtherMode',Cmd='G_SETOTHERMODE_H',Sft=4,Length=20,G_AD_NOISE=1,G_CD_MAGICSQ=1,G_CK_NONE=1,G_CYC_2CYCLE=1,G_PM_NPRIMITIVE=1,G_TC_FILT=1,G_TD_CLAMP=1,G_TF_BILERP=1,G_TL_TILE=1,G_TP_PERSP=1,G_TT_NONE=1)
    cmd(top,'SetOtherMode',Cmd='G_SETOTHERMODE_L',Sft=0,Length=32,G_AC_NONE=1,G_RM_AA_ZB_OPA_SURF2=1,G_RM_PASS=1,G_ZS_PIXEL=1)
    # Real native metal reflection, followed by weathering on the exact same mesh.
    for mat in ['Brass','Blue','Ivory','Dark']:
        vtx=ET.Element('Vertex',Version='0');geo=ET.Element('DisplayList',Version='0');count=0
        for p,n,material in mesh.faces:
            if material!=mat:continue
            axis=int(np.argmax(abs(np.cross(p[1]-p[0],p[2]-p[0]))));axes={0:[2,1],1:[0,2],2:[0,1]}[axis]
            for pos,normal in zip(p,n):
                uv=np.rint(pos[axes]*1024/170).astype(int);nn=np.rint(unit(normal)*127).astype(int)%256
                cmd(vtx,'Vtx',X=pos[0],Y=pos[1],Z=pos[2],S=uv[0],T=uv[1],R=nn[0],G=nn[1],B=nn[2],A=255)
            count+=3
        for start in range(0,count,30):
            batch=min(30,count-start);cmd(geo,'LoadVertices',Path=NS+'gNeiClock'+mat+'Vtx',VertexBufferIndex=0,VertexOffset=start,Count=batch)
            for i in range(0,batch,3):cmd(geo,'Triangle1',V00=i,V01=i+1,V02=i+2)
        cmd(geo,'EndDisplayList');assets['gNeiClock'+mat+'Vtx']=xml_bytes(vtx);assets['gNeiClock'+mat+'GeoDL']=xml_bytes(geo);assets['gNeiClock'+mat+'Tex']=texture(mat)
    cmd(top,'SetGeometryMode',G_TEXTURE_GEN=1,G_TEXTURE_GEN_LINEAR=1);cmd(top,'Texture',S=960,T=960,Level=0,Tile=0,On=1);combine(top,True)
    cmd(top,'SetPrimColor',M=0,L=0,R=246,G=222,B=154,A=255);cmd(top,'SetEnvColor',R=100,G=70,B=25,A=255)
    cmd(top,'LoadTextureBlock',Path=NS+'gNeiClockReflectionTex',Format=0,Size=2,Width=16,Height=16,MaskS=0,MaskT=0,ShiftS=0,ShiftT=0,CMS_TXNoMirror=1,CMS_TXClamp=1,CMT_TXNoMirror=1,CMT_TXClamp=1)
    cmd(top,'CallDisplayList',Path=NS+'gNeiClockBrassGeoDL');cmd(top,'PipeSync');cmd(top,'ClearGeometryMode',G_TEXTURE_GEN=1,G_TEXTURE_GEN_LINEAR=1)
    cmd(top,'Texture',S=65535,T=65535,Level=0,Tile=0,On=1);cmd(top,'SetPrimColor',M=0,L=0,R=255,G=255,B=255,A=255);cmd(top,'SetEnvColor',R=255,G=255,B=255,A=255);combine(top)
    for mat in ['Blue','Ivory','Dark','Brass']:
        cmd(top,'PipeSync');cmd(top,'SetRenderMode',Mode1='G_RM_PASS',Mode2='G_RM_AA_ZB_XLU_DECAL2' if mat=='Brass' else 'G_RM_AA_ZB_OPA_SURF2')
        cmd(top,'LoadTextureBlock',Path=NS+'gNeiClock'+mat+'Tex',Format=0,Size=3,Width=32,Height=32,MaskS=5,MaskT=5,ShiftS=0,ShiftT=0,CMS_TXNoMirror=1,CMS_TXWrap=1,CMT_TXNoMirror=1,CMT_TXWrap=1)
        cmd(top,'CallDisplayList',Path=NS+'gNeiClock'+mat+'GeoDL')
    cmd(top,'PipeSync');cmd(top,'Texture',S=65535,T=65535,Level=0,Tile=0,On=0);cmd(top,'ClearGeometryMode',G_TEXTURE_GEN=1,G_TEXTURE_GEN_LINEAR=1);cmd(top,'SetRenderMode',Mode1='G_RM_PASS',Mode2='G_RM_AA_ZB_OPA_SURF2');cmd(top,'SetPrimColor',M=0,L=0,R=255,G=255,B=255,A=255);cmd(top,'SetEnvColor',R=255,G=255,B=255,A=255);cmd(top,'EndDisplayList')
    assets['gNeiClockDL']=xml_bytes(top);assets['gNeiClockReflectionTex']=reflection_texture()
    points=np.concatenate([p for p,_,_ in mesh.faces]);counts={mat:sum(m==mat for _,_,m in mesh.faces) for mat in ['Brass','Blue','Ivory','Dark']}
    body={'baseline_commit':BODY_BASELINE,'resource_count':len(assets),'bounds_native':[points.min(0).tolist(),points.max(0).tolist()],
          'unique_triangles':len(mesh.faces),'submitted_triangles':len(mesh.faces)+counts['Brass'],'material_triangles':dict(counts)}
    glass,glass_mesh=glass_assets();assets.update(glass);counts['Glass']=len(glass_mesh.faces)
    points=np.concatenate([points]+[p for p,_,_ in glass_mesh.faces])
    stats={'entrypoint':NS+'gNeiClockDL','glass':True,'glass_texture_setup':NS+'gNeiClockGlassTexDL','glass_entrypoint':NS+'gNeiClockGlassDL',
           'bounds_native':[points.min(0).tolist(),points.max(0).tolist()],'draw_scale':.04,
           'unique_triangles':len(mesh.faces)+len(glass_mesh.faces),'submitted_triangles':body['submitted_triangles']+len(glass_mesh.faces),
           'material_triangles':counts,'body':body,
           'glass_contract':{'native_radius':335,'edge_z':106,'center_z':170,'texture_dimensions':[32,32],'max_texture_alpha':70,
                             'texture_setup_then':'runtime tile 0 SetTileSizeLerp, then glass_entrypoint','scroll_offset_units':'quarter texel',
                             'scroll_phase':'uint32(gameplayFrames) & 127','scroll_next_phase':'phase + 1','period_gameplay_ticks':128,
                             'dynamic_segments':[],'render_mode':'G_RM_AA_ZB_XLU_SURF2','depth_write':False,'back_cull':True},
           'dependencies':sorted(NS+x for x in assets),'sha256':{k:hashlib.sha256(v).hexdigest() for k,v in sorted(assets.items())}}
    return assets,stats

def main():
    p=argparse.ArgumentParser();p.add_argument('--check',action='store_true');args=p.parse_args();assets,stats=build()
    for host in ['soh','mm']:
        base=ROOT/host/'assets/custom'/NS
        for name,data in assets.items():
            path=base/name
            if args.check:assert path.read_bytes()==data,str(path)
            else:path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
    target=Path(__file__).parent/'manifest.json';data=json.dumps(stats,indent=2)+'\n'
    if args.check:assert target.read_text()==data
    else:target.write_text(data)
    print(json.dumps({k:v for k,v in stats.items() if k not in ['dependencies','sha256']},indent=2));print('Deterministic resources match both hosts' if args.check else 'Wrote matching native resources to both hosts')
if __name__=='__main__':main()
