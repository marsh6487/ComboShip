"""Visual-only Woods of Mystery POC. Never writes to either input archive."""
from pathlib import Path
import argparse, collections, copy, hashlib, json, math, struct, xml.etree.ElementTree as ET, zipfile
import numpy as np
from PIL import Image
from scene_assets import Resources, canonical, commands, crc64, render
from extract_tunnel import PREFIX as DONOR, recover, touching_component

ROOT='custom/cor_woods_mystery/poc1/'
SCENE='scenes/nonmq/Z2_26SARUNOMORI/'
WALL=SCENE+'Z2_26SARUNOMORITex_00A1C0'
LINING=SCENE+'Z2_26SARUNOMORITex_00B9C0'
FLOOR='misc/scene_texture_06/scene_texture_06_Tex_004460'
FLOOR_BASE='misc/scene_texture_06/scene_texture_06_Tex_002460'
FOREST_EDGE={SCENE+'Z2_26SARUNOMORITex_00A9C0',
             SCENE+'Z2_26SARUNOMORI_room_02Tex_003A60'}


def read_mesh_header(b):
    """Locate and parse the native type-2 mesh command, without decoding actor commands."""
    for start in range(68,len(b)-7):
        if b[start:start+6]!=b'\x0a\0\0\0\0\x02': continue
        count=b[start+6]
        if not 1<=count<40: continue
        p=start+7;entries=[]
        try:
            for _ in range(count):
                assert b[p]==2;p+=1
                center=struct.unpack_from('<hhhh',b,p);p+=8;paths=[]
                for j in range(2):
                    size=struct.unpack_from('<I',b,p)[0];p+=4;assert size<400
                    paths.append(b[p:p+size].decode());p+=size
                entries.append((center,*paths))
            return start,p,entries
        except (AssertionError,ValueError,struct.error,IndexError): pass
    raise ValueError('Native cullable room mesh command not found')


def append_mesh(b,path,center):
    start,end,entries=read_mesh_header(b)
    def string(s):
        # Prelude and Ship both use length-prefixed strings, not C strings.
        data=s.encode()
        return struct.pack('<I',len(data))+data
    extra=bytes([2])+struct.pack('<hhhh',int(center[0]),300,int(center[2]),1600)+string(path)+string('')
    result=bytearray(b[:end]+extra+b[end:]);result[start+6]+=1
    return bytes(result),dict(mesh_offset=start,mesh_end=end,appended_entry_bytes=len(extra))


def texture_resource(image,tile,header):
    im=image.convert('RGBA');b=bytearray(header[:64]);struct.pack_into('<I',b,8,1)
    # HByteScale scales native RGBA16 row bytes to raw RGBA32 row bytes.
    return bytes(b)+struct.pack('<IIIIffI',2,im.width,im.height,1,
                               2*im.width/tile[0],im.height/tile[1],im.width*im.height*4)+im.tobytes()


def emit_mesh(files,path,triangles,lighting=False,cutout=False):
    root=ET.Element('DisplayList',Version='0');vtx=ET.Element('Vertex',Version='0')
    def el(tag,**attr):ET.SubElement(root,tag,{k:str(v) for k,v in attr.items()})
    el('PipeSync');el('ClearGeometryMode',G_CULL_BOTH=1,G_LIGHTING=1,G_TEXTURE_GEN=1,G_TEXTURE_GEN_LINEAR=1)
    geometry=dict(G_ZBUFFER=1,G_SHADE=1,G_SHADING_SMOOTH=1,G_FOG=1)
    if lighting:geometry['G_LIGHTING']=1
    el('SetGeometryMode',**geometry)
    el('SetOtherMode',Cmd='G_SETOTHERMODE_H',Sft=4,Length=20,G_AD_NOISE=1,G_CD_MAGICSQ=1,
       G_CK_NONE=1,G_TC_FILT=1,G_TF_BILERP=1,G_TL_TILE=1,G_TD_CLAMP=1,G_TP_PERSP=1,G_CYC_2CYCLE=1,G_PM_NPRIMITIVE=1)
    mode=dict(G_AC_NONE=1,G_ZS_PIXEL=1,G_RM_FOG_SHADE_A=1)
    mode['G_RM_AA_ZB_TEX_EDGE2' if cutout else 'G_RM_AA_ZB_OPA_SURF2']=1
    el('SetOtherMode',Cmd='G_SETOTHERMODE_L',Sft=0,Length=32,**mode)
    el('SetPrimColor',M=0,L=0,R=255,G=255,B=255,A=255)
    el('SetCombineLERP',A0='G_CCMUX_TEXEL0',B0='G_CCMUX_0',C0='G_CCMUX_SHADE',D0='G_CCMUX_0',
       Aa0='G_ACMUX_TEXEL0',Ab0='G_ACMUX_0',Ac0='G_ACMUX_SHADE',Ad0='G_ACMUX_0',
       A1='G_CCMUX_COMBINED',B1='G_CCMUX_0',C1='G_CCMUX_PRIMITIVE',D1='G_CCMUX_0',
       Aa1='G_ACMUX_0',Ab1='G_ACMUX_0',Ac1='G_ACMUX_0',Ad1='G_ACMUX_COMBINED')
    el('Texture',S=65535,T=65535,Level=0,Tile=0,On=1);el('SetTextureLUT',Mode='G_TT_NONE')
    groups=collections.defaultdict(list)
    for tri in triangles:groups[(tri['texture'],tri['tile'])].append(tri)
    vertex_count=0
    for (texture,tile),tris in groups.items():
        w,h=map(int,tile);line=max(1,w//4)
        el('TileSync');el('SetTextureImage',Path=texture,Format='G_IM_FMT_RGBA',Size='G_IM_SIZ_16b_LOAD_BLOCK',Width=1)
        common=dict(Format='G_IM_FMT_RGBA',TMem=0,Palette=0,Cms0='G_TX_WRAP',Cms1='G_TX_NOMIRROR',
                    Cmt0='G_TX_WRAP',Cmt1='G_TX_NOMIRROR',MaskS=int(math.log2(w)),MaskT=int(math.log2(h)),ShiftS=0,ShiftT=0)
        el('SetTile',Size='G_IM_SIZ_16b_LOAD_BLOCK',Line=0,Tile=7,**common);el('LoadSync')
        el('LoadBlock',Tile=7,Uls=0,Ult=0,Lrs=w*h-1,Dxt=(2048+line-1)//line);el('PipeSync')
        el('SetTile',Size='G_IM_SIZ_16b',Line=line,Tile=0,**common)
        el('SetTileSize',T=0,Uls=0,Ult=0,Lrs=(w-1)*4,Lrt=(h-1)*4)
        for offset in range(0,len(tris),8):
            batch=tris[offset:offset+8]
            el('LoadVertices',Path=path+'_vtx',Count=len(batch)*3,VertexBufferIndex=0,VertexOffset=vertex_count)
            for i,tri in enumerate(batch):
                for row in tri['v']:
                    values=[int(round(x)) for x in row]
                    assert all(-32768<=x<=32767 for x in values[:5])
                    ET.SubElement(vtx,'Vtx',dict(zip(['X','Y','Z','S','T','R','G','B','A'],map(str,values))))
                el('Triangle1',V00=i*3,V01=i*3+1,V02=i*3+2,Flag0=0);vertex_count+=3
    el('EndDisplayList')
    files['alt/'+path]=ET.tostring(root,encoding='utf-8')
    files['alt/'+path+'_vtx']=ET.tostring(vtx,encoding='utf-8')


def transformed(tris,fn):
    output=[]
    for tri in tris:
        t={**tri,'v':tri['v'].copy()};t['v'][:,:3]=fn(t['v'][:,:3]);output.append(t)
    return output


def correct_interior(tunnel):
    """Weld the inward wood's UV/color seams without changing its geometry or rim."""
    rows=collections.defaultdict(list)
    for tri in tunnel:
        if tri['path'].endswith('/room_0/Geometry5'):
            for v in tri['v']:rows[tuple(v[:3])].append(v)
    attributes={}
    for xyz,vertices in rows.items():
        # Retain the original circumferential wrap, including the full-width
        # floor span. Only reconcile small phase offsets at shared positions.
        s=collections.Counter(round(v[3])%1024 for v in vertices).most_common(1)[0][0]
        colors=np.asarray(sorted({tuple(v[5:8]) for v in vertices}))
        attributes[xyz]=(s,np.rint(colors.mean(0)))
    output=[]
    for tri in tunnel:
        if not tri['path'].endswith('/room_0/Geometry5'):
            output.append(tri);continue
        t={**tri,'v':tri['v'].copy()}
        for v in t['v']:
            s,color=attributes[tuple(v[:3])]
            v[3]=s+round((v[3]-s)/1024)*1024
            # The last ~6 texels of this atlas are bark. Use the continuous
            # plank band for the entire inward sleeve, with bilinear margin.
            v[4]=16+(763-v[0])*784/317
            v[5:8]=color
        output.append(t)
    return output


def solid_lining(tri,center,floor_uv):
    """Replace the native black alpha fade with opaque, world-tiled surfaces."""
    t={**tri,'v':tri['v'].copy()};v=t['v'];p=v[:,:3]-center
    direction=np.zeros(2);direction[int(abs(p[:,[0,2]].mean(0)).argmax())]=1
    along=p[:,[0,2]]@direction;across=p[:,[0,2]]@np.array([-direction[1],direction[0]])
    if v[:,1].max()<=10:
        material='grass';v[:,3:5]=np.column_stack([v[:,0],v[:,2],np.ones(3)])@floor_uv
    else:
        material='vines' if direction[0] else 'rock'
        normal=np.cross(p[1]-p[0],p[2]-p[0])
        # This is opaque backing, not the visible wood sleeve. The original
        # 70/80-unit sides and 112-unit roof cut through the larger donor tube.
        across=np.where(abs(across)>1,np.sign(across)*116,across)
        near=abs(along)==240
        along=np.where(near,np.sign(along)*210,along)
        along=np.where(abs(along)==440,np.sign(along)*450,along)
        side=np.array([-direction[1],direction[0]])
        v[:,[0,2]]=center[[0,2]]+along[:,None]*direction+across[:,None]*side
        v[:,1]=np.where(v[:,1]-center[1]>=90,np.maximum(v[:,1],center[1]+145),v[:,1])
        # Keep the side feet below every grass surface, including the widened
        # floor wings and the far seam. These vertices are visual backing only.
        v[:,1]=np.where(v[:,1]-center[1]<=10,center[1]-4,v[:,1])
        p=v[:,:3]-center
        v[:,3:5]=np.column_stack([along,across if abs(normal[1])>max(abs(normal[0]),abs(normal[2])) else -p[:,1]])*6.4
    v[:,8]=255
    t.update(texture=ROOT+'textures/'+material,tile=(32,32),origin=(0,0))
    return t


def lining_floor_wings(triangles,center,floor_uv):
    """Close the widened hidden backing while retaining the original floor faces."""
    floors=[t for t in triangles if t['v'][:,1].max()<=10];output=[]
    for x,z in [(1,0),(-1,0),(0,1),(0,-1)]:
        direction=np.array([x,z]);side=np.array([-z,x])
        template=next(t for t in floors if np.min((t['v'][:,[0,2]]-center[[0,2]])@direction)>=239)
        for sign in [-1,1]:
            points=[(240,0,sign*80),(440,10,sign*70),(440,10,sign*116),(240,0,sign*116)]
            rows=[]
            for depth,height,across in points:
                xz=center[[0,2]]+depth*direction+across*side
                rows.append(np.r_[xz[0],height+center[1],xz[1],0,0,template['v'][0,5:9]])
            for tri in polygon_triangles(rows,template):output.append(solid_lining(tri,center,floor_uv))
        rows=[]
        for depth,across in [(440,-116),(450,-116),(450,116),(440,116)]:
            xz=center[[0,2]]+depth*direction+across*side
            rows.append(np.r_[xz[0],10+center[1],xz[1],0,0,template['v'][0,5:9]])
        for tri in polygon_triangles(rows,template):output.append(solid_lining(tri,center,floor_uv))
    return output


def lining_exit_surround(fitted,center,direction,template):
    """Seal the backing's far annulus, welded to the still-open wood end loop."""
    side=np.array([-direction[1],direction[0]]);edges={}
    for tri in fitted:
        for i,a in enumerate(tri['v']):
            b=tri['v'][(i+1)%3]
            if max(abs((p[[0,2]]-center[[0,2]])@direction-450) for p in [a,b])>1e-5:continue
            key=tuple(sorted(tuple(np.rint(p[:3]).astype(int)) for p in [a,b]))
            if key[0]!=key[1]:edges[key]=key
    assert len(edges)>=12
    midpoint=np.array([0,65.]);corners=[np.array(p,float) for p in [(-116,-4),(116,-4),(116,145),(-116,145)]]
    def cross(a,b):return a[0]*b[1]-a[1]*b[0]
    def point(p):return np.array([(np.asarray(p)[[0,2]]-center[[0,2]])@side,p[1]-center[1]])
    projections={}
    def outer(p):
        key=tuple(p)
        if key in projections:return projections[key]
        d=p-midpoint;limits=[]
        if abs(d[0])>1e-8:limits.append(((116 if d[0]>0 else -116)-midpoint[0])/d[0])
        if abs(d[1])>1e-8:limits.append(((145 if d[1]>0 else -4)-midpoint[1])/d[1])
        projections[key]=midpoint+min(limits)*d
        return projections[key]
    normal=np.array([-direction[0]*127,0,-direction[1]*127])%256
    def vertex(p):
        xz=center[[0,2]]+450*direction+p[0]*side
        return np.r_[xz[0],center[1]+p[1],xz[1],p[0]*6.4,-p[1]*6.4,normal,255]
    result=[]
    for a,b in edges.values():
        a=point(a);b=point(b)
        if cross(a-midpoint,b-midpoint)<0:a,b=b,a
        da=a-midpoint;db=b-midpoint
        between=[p for p in corners if cross(da,p-midpoint)>1e-7 and cross(p-midpoint,db)>1e-7]
        between.sort(key=lambda p:math.atan2(cross(da,p-midpoint),da@(p-midpoint)),reverse=True)
        polygon=[a,b,outer(b),*between,outer(a)]
        material='vines' if direction[0] else 'rock'
        face={**template,'texture':ROOT+'textures/'+material,'tile':(32,32),'origin':(0,0),'lit':True}
        result.extend(polygon_triangles([vertex(p) for p in polygon],face))
    return result


def clip_polygon(polygon,distance):
    """Clip full vertex rows against a linear plane, interpolating UVs/normals."""
    output=[]
    for i,current in enumerate(polygon):
        previous=polygon[i-1];a=distance(previous);b=distance(current)
        if (a>=0)!=(b>=0):output.append(previous+(current-previous)*a/(a-b))
        if b>=0:output.append(current)
    return output


def polygon_triangles(polygon,template):
    output=[]
    for i in range(1,len(polygon)-1):
        v=np.stack([polygon[0],polygon[i],polygon[i+1]])
        if np.linalg.norm(np.cross(v[1,:3]-v[0,:3],v[2,:3]-v[0,:3]))>1e-5:
            output.append({**template,'v':v})
    return output


def convex_profile(points):
    points=sorted({tuple(round(float(x),6) for x in p) for p in points})
    def cross(o,a,b):return (a[0]-o[0])*(b[1]-o[1])-(a[1]-o[1])*(b[0]-o[0])
    def chain(sequence):
        result=[]
        for p in sequence:
            while len(result)>1 and cross(result[-2],result[-1],p)<=1e-3:result.pop()
            result.append(p)
        return result
    hull=chain(points)[:-1]+chain(points[::-1])[:-1]
    assert len(hull)>=6, 'Wood sleeve cross-section is not closed'
    return np.asarray(hull)


def section_profile(triangles,origin,direction,depth):
    """Convex outline of an actual wood-sleeve cross-section (across, height)."""
    axis=np.asarray(direction);side=np.array([-axis[1],axis[0]]);points=[]
    for tri in triangles:
        v=tri['v'];along=(v[:,[0,2]]-origin)@axis-depth
        for i,a in enumerate(v):
            b=v[(i+1)%3];da=along[i];db=along[(i+1)%3]
            if abs(da)<1e-6:points.append(((a[[0,2]]-origin)@side,a[1]))
            if da*db<0:
                p=a+(b-a)*da/(da-db);points.append(((p[[0,2]]-origin)@side,p[1]))
    return convex_profile(points)


def bridge_surround(tri,profile,origin,direction):
    """Keep only the native corner wall outside the fitted wood aperture."""
    side=np.array([-direction[1],direction[0]]);v=tri['v'].copy()
    # Lighting normals are signed bytes, unlike unlit donor vertex colors.
    v[:,5:8]=np.where(v[:,5:8]>=128,v[:,5:8]-256,v[:,5:8])
    remaining=list(v);outside=[]
    for i,a in enumerate(profile):
        edge=profile[(i+1)%len(profile)]-a
        def distance(v):
            across=(v[[0,2]]-origin)@side
            return edge[0]*(v[1]-a[1])-edge[1]*(across-a[0])
        outside.extend(polygon_triangles(clip_polygon(remaining,lambda v:-distance(v)),tri))
        remaining=clip_polygon(remaining,distance)
        if not remaining:break
    for piece in outside:piece['v'][:,5:8]=np.rint(piece['v'][:,5:8])%256
    return outside


def conform_wall_edges(triangles):
    """Share every clipped boundary subdivision before integer serialization."""
    if not triangles:return []
    points=np.asarray(sorted({tuple(np.round(v[:3],8)) for tri in triangles for v in tri['v']}))
    result=[]
    for tri in triangles:
        v=tri['v'].copy();v[:,5:8]=np.where(v[:,5:8]>=128,v[:,5:8]-256,v[:,5:8])
        boundary=[]
        for i,a in enumerate(v):
            b=v[(i+1)%3];delta=b[:3]-a[:3];length=delta@delta
            boundary.append(a.copy())
            if length<1e-10:continue
            t=(points-a[:3])@delta/length
            error=np.linalg.norm(points-a[:3]-t[:,None]*delta,axis=1)
            indices=np.where((t>1e-7)&(t<1-1e-7)&(error<1e-6))[0]
            for j in sorted(indices,key=lambda j:t[j]):
                row=a+(b-a)*t[j];row[:3]=points[j];boundary.append(row)
        if len(boundary)==3:
            result.append(tri);continue
        center=v.mean(0)
        for i,a in enumerate(boundary):
            rows=np.stack([center,a,boundary[(i+1)%len(boundary)]])
            rows[:,5:8]=np.rint(rows[:,5:8])%256
            result.extend(polygon_triangles(list(rows),tri))
    return result


def recessed_bridge_end(triangles,origin,direction):
    """Continue every approved end edge, with one shared quantized far endpoint."""
    edges={}
    for tri in triangles:
        v=tri['v']
        for i,a in enumerate(v):
            b=v[(i+1)%3]
            if max(abs((p[[0,2]]-origin)@direction-120) for p in [a,b])>1e-5:continue
            key=tuple(sorted(tuple(np.rint(p[:3]).astype(int)) for p in [a,b]))
            if key[0]!=key[1]:edges[key]=(a,b,tri)
    assert len(edges)>=8, 'Recessed sleeve has no closed terminal loop'
    endpoints={tuple(np.rint(v[:3]).astype(int)) for a,b,_ in edges.values() for v in [a,b]}
    far={p:np.rint(np.asarray(p)+[direction[0]*30,0,direction[1]*30]) for p in endpoints}
    center=np.rint(np.mean(list(far.values()),axis=0))
    def vertex(p):return np.r_[p[:3],0,0,255,255,255,255]
    extension=[];cap=[]
    for a,b,tri in edges.values():
        a=a.copy();b=b.copy();a[:3]=np.rint(a[:3]);b[:3]=np.rint(b[:3])
        end_a=a.copy();end_b=b.copy()
        end_a[:3]=far[tuple(a[:3])];end_b[:3]=far[tuple(b[:3])]
        end_a[4]-=30/.72*784/317;end_b[4]-=30/.72*784/317
        extension.extend(polygon_triangles([a,b,end_b,end_a],tri))
        # Keep all collinear cut vertices. A convex hull would remove them and
        # open tiny cracks after independent integer vertex quantization.
        cap.append(dict(v=np.stack([vertex(center),vertex(end_a),vertex(end_b)]),
                        texture=ROOT+'textures/black',tile=(4,1),origin=(0,0),lit=False))
    return extension+cap


def tunnel_at(tunnel,origin,direction,length=225,width=1.20,height=1.0):
    axis=np.asarray(direction,float);side=np.array([-axis[1],axis[0]])
    # Use the external mouth and only the required part of its interior. The
    # second room's distant exit rim and the long intervening body stay in the
    # reusable donor GLB; they are not squeezed into these short passageways.
    limit=446+length/width
    cropped=[]
    for tri in tunnel:
        if '/room_0/' not in tri['path']:continue
        polygon=[];source=tri['v']
        for i,current in enumerate(source):
            previous=source[i-1];inside=current[0]<=limit;was_inside=previous[0]<=limit
            if inside!=was_inside:
                amount=(limit-previous[0])/(current[0]-previous[0])
                polygon.append(previous+(current-previous)*amount)
            if inside:polygon.append(current)
        for i in range(1,len(polygon)-1):
            cropped.append({**tri,'v':np.stack([polygon[0],polygon[i],polygon[i+1]])})
    def transform(p):
        along=(p[:,0]-446)*width
        across=p[:,2]*width
        return np.column_stack([origin[0]+axis[0]*along+side[0]*across,
                                p[:,1]*height+1,
                                origin[1]+axis[1]*along+side[1]*across])
    return transformed(cropped,transform)


def components(tris):
    remaining=list(tris);groups=[]
    while remaining:
        seed=remaining.pop();part=touching_component(remaining,{tuple(v[:3]) for v in seed['v']})+[seed]
        ids={id(t) for t in part};remaining=[t for t in remaining if id(t) not in ids];groups.append(part)
    return groups


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--native',type=Path,required=True)
    parser.add_argument('--poc',type=Path,required=True);parser.add_argument('--textures',type=Path,required=True)
    parser.add_argument('--foliage',type=Path,required=True,help='Selected original Henriko PNGs and provenance manifest')
    parser.add_argument('--donor-archive-sha256',help='Original POC identity when restoring from its exact resource subset')
    parser.add_argument('--preview',action='store_true',help='Optional software cutaways; not Prelude verification')
    parser.add_argument('--output',type=Path,required=True);a=parser.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    native=Resources(a.native);donor=Resources(a.poc);tunnel=correct_interior(recover(donor));files={};header=donor.get(tunnel[0]['texture'])
    bridge_origin=np.array([110,-110]);bridge_axis=np.array([1,-1])/math.sqrt(2)
    approved_bridge=tunnel_at(tunnel,bridge_origin,bridge_axis,length=120,width=.72,height=.95)
    deeper_bridge=tunnel_at(tunnel,bridge_origin,bridge_axis,length=151,width=.72,height=.95)
    aperture=section_profile(deeper_bridge,bridge_origin,bridge_axis,100)
    # Give the inward crown clearance. The closed wood sleeve and cap retain
    # coverage; the forest wall must not project a bright lip inside that roof.
    aperture[:,0]*=.99;bottom=aperture[:,1].min()
    aperture[:,1]=60+(aperture[:,1]-60)*.99
    aperture[:,1]+=3*np.clip((aperture[:,1]-60)/15,0,1)
    aperture[aperture[:,1]<bottom+1,1]=-2
    rim=np.concatenate([t['v'] for t in tunnel if t['path'].endswith('/room_0/Geometry6')])
    maze_aperture=convex_profile(np.column_stack([rim[:,2]*1.2,rim[:,1]+1]))
    maze_aperture[:,0]*=1.02;maze_aperture[:,1]=67+(maze_aperture[:,1]-67)*1.02
    maze_aperture[maze_aperture[:,1]<2,1]=-2
    texture_files={'grass':('tex1_256x256_ADA05E94E0A29C59_12_mip0(1).png',(32,32)),
                   'rock':('tex1_256x256_C43F3D1429C99348_12_mip0.png',(32,32)),
                   'vines':('tex1_256x256_C12BE192D6CD5D49_12_mip0.png',(32,32)),
                   'ruins':('tex1_64x128_251B211589768393_12_mip0.png',(32,64))}
    for name,(filename,tile) in texture_files.items():
        files['alt/'+ROOT+'textures/'+name]=texture_resource(Image.open(a.textures/filename),tile,header)
    foliage=json.loads((a.foliage/'manifest.json').read_text())
    foliage_tiles={'canopy':(32,32),'forest_edge':(32,16)}
    for role,item in foliage['materials'].items():
        image=a.foliage/item['file']
        assert hashlib.sha256(image.read_bytes()).hexdigest()==item['sha256']
        files['alt/'+ROOT+'textures/henriko_'+role]=texture_resource(Image.open(image),foliage_tiles[role],header)
    trees=sorted(components(donor.mesh(DONOR+'room_0/Geometry6')),key=lambda ts:-len(ts))[:2]
    needed={t['texture'] for t in tunnel+trees[0]+trees[1]}
    for n in needed:files['alt/'+n]=donor.get(n)
    donor_names=needed|{t['path'] for t in tunnel+trees[0]+trees[1]}|{r[0] for t in tunnel+trees[0]+trees[1] for r in t['refs']}
    donor_identity=a.donor_archive_sha256 or hashlib.sha256(a.poc.read_bytes()).hexdigest()
    assert len(donor_identity)==64 and all(c in '0123456789abcdef' for c in donor_identity)
    # A render tile row is at least eight native bytes (four RGBA16 pixels).
    black=Image.new('RGBA',(4,1),(0,0,0,255))
    files['alt/'+ROOT+'textures/black']=texture_resource(black,(4,1),header)
    report=dict(candidate='Woods of Mystery OoT Makeover POC4 - Wall Fit',donor_faces=246,donor_vertices=321,
      donor_sha256=donor_identity,collision='Vanilla; not overridden',
      donor_resources={n:hashlib.sha256(donor.get(n)).hexdigest() for n in sorted(donor_names)},
      native_sha256=hashlib.sha256(a.native.read_bytes()).hexdigest(),rooms=[],
      supplied_textures={k:dict(file=f,tile=list(tile),sha256=hashlib.sha256((a.textures/f).read_bytes()).hexdigest()) for k,(f,tile) in texture_files.items()},
      tunnel_fit='External entrance rim and cropped interior only; no axial compression of the full donor',
      maze_mouth_offset=175,maze_sleeve_length=275,maze_far_join=450,bridge_sleeve_length=150,
      maze_surround='Wall aperture follows the complete projected bark rim, including its lower sides',
      lining_backing=dict(near_start=210,far_end=450,side_offset=116,roof_minimum=145,
                         floor_positions='Original floor unchanged; 16 grass wings and 8 tail triangles close backing feet',
                         exit_surround='Annulus welded to every actual wood terminal edge; passage remains open'),
      approved_bridge_faces=110,approved_bridge_sleeve_length=120,
      approved_bridge_preservation='All 110 triangle positions; all 24 exterior rim triangles and attributes',
      interior_wood='Continuous axial plank-band UVs; shared interior S phase and baked RGB welded; geometry unchanged',
      bridge_continuation='30-unit continuation stitched to every quantized approved end-loop vertex',
      bridge_surround='Native corner wall clipped to wood aperture; original stone pillars removed',
      dark_cap='Every terminal wood edge welded at depth 150, including collinear cuts; no rectangular backdrop',
      wall_material='Native lighting and normal bytes retained; donor bark keeps its unlit vertex colors',
      foliage=foliage,foliage_display_lists=[],solid_lining_display_lists=[],
      status='POC3 sky coverage accepted by cor; POC4 wall fit is a separate candidate awaiting play-renderer inspection.')
    for room in range(9):
        key=SCENE+f'Z2_26SARUNOMORI_room_{room:02d}';b=native.get(key)
        start,end,entries=read_mesh_header(b)
        paths=list(dict.fromkeys(p for _,opa,xlu in entries for p in (opa,xlu) if p))
        original=[]
        for path in paths:original.extend(native.mesh(path))
        floor_positions=np.concatenate([t['v'][:,:3] for t in original if t['texture']==FLOOR])
        floor_paths={t['path'] for t in original if t['texture']==FLOOR}
        room_origin=(floor_positions.min(0)+floor_positions.max(0))/2
        floor_vertices=np.concatenate([t['v'] for t in original if t['texture']==FLOOR])
        floor_uv=np.linalg.lstsq(np.column_stack([floor_vertices[:,0],floor_vertices[:,2],np.ones(len(floor_vertices))]),
                                floor_vertices[:,3:5],rcond=None)[0]
        walls_by_path=collections.defaultdict(list)
        for tri in original:
            if tri['texture']==WALL:walls_by_path[tri['path']].append(tri)
        assert len(walls_by_path)==1,(room,walls_by_path.keys())
        removed=0;corner_faces=0
        for path,walls in walls_by_path.items():
            kept=[]
            for tri in walls:
                p=tri['v'][:,:3]-room_origin
                hallway=any(((p[:,axis]*sign>=239)&(abs(p[:,other])<=81)).all()
                            for axis,other in [(0,2),(2,0)] for sign in [-1,1])
                # The top wall above the native opening is still the surround,
                # not an interior corridor face to remove for the wood sleeve.
                fascia=any((p[:,axis]==sign*240).all() for axis in [0,2] for sign in [-1,1])
                bridge=room==1 and ((p[:,0]>=80)&(p[:,2]<=-80)).all()
                if hallway and not fascia:removed+=1;continue
                if bridge:
                    corner_faces+=1
                    pieces=bridge_surround(tri,aperture,bridge_origin,bridge_axis)
                else:pieces=[tri]
                for axis,other in [(0,2),(2,0)]:
                    for sign in [-1,1]:
                        direction=np.zeros(2);direction[axis//2]=sign
                        # The wall shoulders slope into the clearing: testing
                        # only the flat 240-unit opening misses those panels.
                        if np.min(p[:,[0,2]]@direction)<150:continue
                        origin=room_origin[[0,2]]+direction*175
                        pieces=[cut for piece in pieces for cut in bridge_surround(piece,maze_aperture,origin,direction)]
                kept.extend(pieces)
            kept=conform_wall_edges(kept)
            tiled=[]
            for piece in kept:
                face_center=(piece['v'][:,:3]-room_origin).mean(0)
                material='vines' if abs(face_center[0])>abs(face_center[2]) else 'rock'
                tiled.append({**piece,'v':piece['v'].copy(),'texture':ROOT+'textures/'+material,
                              'tile':texture_files[material][1]})
            emit_mesh(files,path,tiled,lighting=True)
        for path in sorted({t['path'] for t in original if t['texture']==LINING}):
            source=[t for t in original if t['path']==path]
            triangles=[solid_lining(t,room_origin,floor_uv) for t in source]
            triangles.extend(lining_floor_wings(source,room_origin,floor_uv))
            for d in [(1,0),(-1,0),(0,1),(0,-1)]:
                direction=np.asarray(d)
                fitted=tunnel_at(tunnel,room_origin[[0,2]]+175*direction,direction,length=275)
                triangles.extend(lining_exit_surround(fitted,room_origin,direction,source[0]))
            assert len(triangles)>64 and all(t['lit'] for t in triangles)
            backing=ROOT+f'room_{room}/backing'
            emit_mesh(files,backing,triangles,lighting=True)
            # The expanded backing exceeds the native lining's 450-unit cull
            # sphere. Draw it through the appended 1600-unit visual entry.
            files['alt/'+path]=b'<DisplayList Version="0"><EndDisplayList /></DisplayList>'
            report['solid_lining_display_lists'].append(dict(room=room,path=backing,native_path=path,
                triangles=len(triangles),culling='Shares appended visual entry radius 1600'))
        # Replace the old two-texture leaf tints with original Henriko RGBA.
        # Keep the native foliage mesh and normals; scale UVs with logical tiles
        # so the single RGBA16 stage remains within the 4 KiB texture memory.
        foliage_paths={t['path'] for t in original if t['texture'] in FOREST_EDGE|{
           FLOOR_BASE,
           'misc/scene_texture_06/scene_texture_06_Tex_000C20',
           'misc/scene_texture_06/scene_texture_06_Tex_001440'}
          and t['v'][:,1].max()>200}
        for path in sorted(foliage_paths):
            triangles=[]
            for tri in [t for t in original if t['path']==path]:
                role='forest_edge' if tri['texture'] in FOREST_EDGE else 'canopy'
                tile=foliage_tiles[role];t={**tri,'v':tri['v'].copy()}
                t['v'][:,3:5]=(t['v'][:,3:5]-np.asarray(tri['origin'])*32)*np.asarray(tile)/np.asarray(tri['tile'])
                t.update(texture=ROOT+'textures/henriko_'+role,tile=tile,origin=(0,0));triangles.append(t)
            assert triangles and all(t['lit'] for t in triangles)
            emit_mesh(files,path,triangles,lighting=True,cutout=True)
            report['foliage_display_lists'].append(dict(room=room,path=path,triangles=len(triangles)))
        # Scope the grass replacement to this room's display lists, never the shared scene texture.
        for name,data in native.files.items():
            # The same base texture also colors the canopy: patch only the floor list.
            if name not in floor_paths:continue
            if data.startswith(b'<'):continue
            patched=bytearray(data);changed=False
            for off,op,w0,w1,h,size in commands(data):
                if op==32 and h in [crc64(FLOOR_BASE),crc64(FLOOR)]:
                    h=crc64(ROOT+'textures/grass');struct.pack_into('<II',patched,off+8,h>>32,h&0xffffffff);changed=True
            if changed:files['alt/'+name]=bytes(patched)
        added=[]
        for direction in [(1,0),(-1,0),(0,1),(0,-1)]:
            added.extend(tunnel_at(tunnel,np.asarray(direction)*175+room_origin[[0,2]],direction,length=275))
        for i,(x,z) in enumerate([(-235,-235),(235,-235),(235,235),(-235,235)]):
            source=trees[i%2];p=np.concatenate([t['v'][:,:3] for t in source]);donor_center=(p.min(0)+p.max(0))/2
            angle=i*math.pi/2;co,si=math.cos(angle),math.sin(angle)
            def tree_transform(p,center=donor_center,x=x,z=z,co=co,si=si,origin=room_origin):
                q=p-center;xx=q[:,0]*.37;zz=q[:,2]*.37
                return np.column_stack([origin[0]+x+xx*co-zz*si,175+(p[:,1])*0.42,origin[2]+z+xx*si+zz*co])
            added.extend(transformed(source,tree_transform))
        if room==1:
            added.extend(approved_bridge)
            added.extend(recessed_bridge_end(approved_bridge,bridge_origin,bridge_axis))
        path=ROOT+f'room_{room}/geometry';emit_mesh(files,path,added)
        draw=ET.fromstring(files['alt/'+path])
        draw.insert(len(draw)-1,ET.Element('CallDisplayList',Path=backing))
        draw.insert(len(draw)-1,ET.Element('ClearGeometryMode',G_LIGHTING='1'))
        files['alt/'+path]=ET.tostring(draw,encoding='utf-8')
        files['alt/'+key],info=append_mesh(b,path,room_origin)
        info.update(room=room,center=room_origin.tolist(),tunnel_count=4,bridge_tunnel=room==1,added_faces=len(added),
                    replaced_hallway_faces=removed,reworked_bridge_corner_faces=corner_faces)
        report['rooms'].append(info)
    files['woods-mystery-poc4.json']=(json.dumps(report,indent=2)+'\n').encode()
    manifest=dict(prelude=1,game='mm',romVersion='NTSC 1.0',romVersionHash='0x5354631C',
      exportedAt='2026-10-19T18:00:00.000Z',scenes=['Z2_26SARUNOMORI'],imported=[],altAssets=True,
      project=dict(version=1,previewedAgainst=[],layers=[dict(kind='scene-replace',targetScene='Z2_26SARUNOMORI',
        sceneName='Woods of Mystery OoT Makeover POC4',entries=sorted(canonical(n) for n in files if n.startswith('alt/')))]))
    files['prelude/manifest.json']=(json.dumps(manifest,indent=2)+'\n').encode()
    files['prelude/project/edits.json']=b'{"codec":2,"blobs":[],"edits":{}}\n'
    files['prelude/project/blobs.bin']=b''
    # A complete additive pack: no other forest/shop scenes or collision resources are present.
    candidate=a.output/'zzzzz_Woods_of_Mystery_OoT_Makeover_POC4.prelude.o2r'
    with zipfile.ZipFile(candidate,'w',zipfile.ZIP_DEFLATED) as z:
        for n,b in sorted(files.items()):
            info=zipfile.ZipInfo(n,(2026,10,19,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;z.writestr(info,b)
    (a.output/'woods-mystery-poc4.json').write_bytes(files['woods-mystery-poc4.json'])
    if not a.preview:
        print(json.dumps(dict(candidate=str(candidate),bytes=candidate.stat().st_size,resources=len(files),rooms=report['rooms']),indent=2))
        return
    res=Resources(a.native,candidate);key=SCENE+'Z2_26SARUNOMORI_room_01';_,_,entries=read_mesh_header(res.get(key));tris=[]
    for path in dict.fromkeys(e[1] for e in entries if e[1]):tris.extend(res.mesh(path))
    # This inspection renderer has one texture stage. Hide original native canopy
    # billboards that require two-stage alpha, rather than render opaque rectangles.
    inspection=[t for t in tris if t['path'].startswith(ROOT) or t['texture'] in
                [ROOT+'textures/'+k for k in texture_files] or
                t['texture']==SCENE+'Z2_26SARUNOMORITex_00BAC0']
    render(res,inspection,a.output/'Woods-of-Mystery-POC1-Clearing.png',eye=[-450,650,450],target=[20,100,-20],
           width=1100,height=760,ortho_width=900,label='POC1 asset cutaway | Native canopy hidden for inspection | Gameplay untested')
    render(res,inspection,a.output/'Woods-of-Mystery-POC1-Bridge.png',eye=[-170,105,170],target=[190,75,-190],
           width=1000,height=650,ortho_width=420,label='POC1 bridge tunnel | Native canopy hidden for inspection | Matching hex source required')
    print(json.dumps(dict(candidate=str(candidate),bytes=candidate.stat().st_size,resources=len(files),rooms=report['rooms']),indent=2))


if __name__=='__main__':main()
