"""Static before/after from actual production VTX, triangles and OTEX bytes.

Software rasterization is a resource/UV/material preview, not gameplay proof.
No concept mesh or independent texture representation is substituted.
"""
import argparse
import json
import re
import struct
import subprocess
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]
BASELINE = 'e22bcb222dcd06d18567cbbb47087d3d5979be96'


def geometry(text):
    vertices = {}
    for name, body in re.findall(r'static Vtx (\w+)\[\] = \{(.*?)\n\};', text, re.S):
        vertices[name] = [list(map(int, row.split(','))) for row in re.findall(r'VTX\(([^)]*)\)', body)]
    shapes = {}
    for kind, name in [('blossom', 'sSpringBlossomGeometry'), ('petal', 'sSpringPetalGeometry')]:
        body = re.search(r'static Gfx ' + name + r'\[\] = \{(.*?)\n\};', text, re.S).group(1)
        triangles, current, textured = [], None, 'NeiSpringBlossoms_SetMaterial' in text
        for command, args in re.findall(r'(gs\w+)\(([^)]*)\)', body):
            if command == 'gsSPVertex':
                current = args.split(',')[0].strip()
            elif command == 'gsSPTexture' and 'G_OFF' in args:
                textured = False
            elif command == 'gsSP2Triangles':
                ids = list(map(int, args.split(',')))
                for face in (ids[:3], ids[4:7]):
                    triangles.append(dict(vertices=[vertices[current][i] for i in face], textured=textured))
        shapes[kind] = triangles
    return shapes


def texture(path):
    raw = path.read_bytes()
    endian, tag, version = struct.unpack_from('<III', raw)
    fmt, w, h, flags, sx, sy, count = struct.unpack_from('<IIIIffI', raw, 64)
    assert (endian, tag, version, fmt, flags) == (0, 0x4F544558, 1, 1, 1)
    assert count == w * h * 4 and sx == w / 32 and sy == h / 32
    return np.frombuffer(raw[92:], np.uint8).reshape(h, w, 4) / 255


def raster(triangles, tex, size, angle=0, fade=220 / 255, flutter=0.65, variant=0):
    samples = 4
    canvas = np.zeros((size * samples, size * samples, 4))
    colors = np.array([[255,229,239],[255,214,230],[255,238,245],[255,204,225]]) / 255
    prim = np.r_[colors[variant], fade]
    angle = np.deg2rad(angle)
    # Same local silhouette bounds and width flutter as native hosts. Angle
    # is an inspection camera only; game particles remain billboarded.
    scale = size * samples / 360
    for tri in triangles:
        verts = np.array(tri['vertices'], dtype=float)
        xyz = verts[:, :3].copy()
        xyz[:, 0] *= flutter
        projected = np.column_stack((xyz[:,0] * np.cos(angle) + xyz[:,2] * np.sin(angle), xyz[:,1]))
        p = np.column_stack((projected[:,0] * scale + size*samples/2,
                             size*samples/2 - projected[:,1] * scale))
        lo = np.maximum(np.floor(p.min(axis=0)).astype(int), 0)
        hi = np.minimum(np.ceil(p.max(axis=0)).astype(int)+1, size*samples)
        if np.any(hi <= lo):
            continue
        x,y=np.meshgrid(np.arange(lo[0],hi[0])+.5,np.arange(lo[1],hi[1])+.5)
        den=(p[1,1]-p[2,1])*(p[0,0]-p[2,0])+(p[2,0]-p[1,0])*(p[0,1]-p[2,1])
        if abs(den)<1e-8: continue
        a=((p[1,1]-p[2,1])*(x-p[2,0])+(p[2,0]-p[1,0])*(y-p[2,1]))/den
        b=((p[2,1]-p[0,1])*(x-p[2,0])+(p[0,0]-p[2,0])*(y-p[2,1]))/den
        c=1-a-b
        inside=(a>=0)&(b>=0)&(c>=0)
        weights=np.stack((a,b,c),axis=-1)
        shade=weights@verts[:,5:9]/255
        rgba=shade*prim
        if tri['textured']:
            # Native half-texel coordinate convention with 0xFFFF texture scale.
            uv=((verts[:,3:5].astype(int)*65535>>16)/32+.5)/32
            coords=weights@uv
            h,w,_=tex.shape
            tx=np.clip(coords[...,0]*w-.5,0,w-1);ty=np.clip(coords[...,1]*h-.5,0,h-1)
            ix=tx.astype(int);iy=ty.astype(int);fx=(tx-ix)[...,None];fy=(ty-iy)[...,None]
            ix2=np.minimum(ix+1,w-1);iy2=np.minimum(iy+1,h-1)
            sampled=(tex[iy,ix]*(1-fx)+tex[iy,ix2]*fx)*(1-fy)+(tex[iy2,ix]*(1-fx)+tex[iy2,ix2]*fx)*fy
            rgba*=sampled
        rgba=np.clip(rgba,0,1); alpha=rgba[...,3:4]*inside[...,None]
        dst=canvas[lo[1]:hi[1],lo[0]:hi[0]]
        dst[:,:,:3]=rgba[:,:,:3]*alpha+dst[:,:,:3]*(1-alpha)
        dst[:,:,3:4]=alpha+dst[:,:,3:4]*(1-alpha)
    # Downsample premultiplied RGB/alpha to retain soft edges.
    rgba=canvas.reshape(size,samples,size,samples,4).mean(axis=(1,3))
    return rgba


def composite(rgba, bg):
    rgb=rgba[:,:,:3]+np.array(bg)[None,None,:]/255*(1-rgba[:,:,3:4])
    return Image.fromarray(np.uint8(np.clip(rgb,0,1)*255), 'RGB')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    old=subprocess.check_output(['git','show',BASELINE+':combo/NeiSpringBlossoms.h'],cwd=ROOT,text=True)
    new=(ROOT/'combo/NeiSpringBlossoms.h').read_text()
    meshes={'before':geometry(old),'after':geometry(new)}
    textures={kind:texture(ROOT/'soh/assets/custom/objects/object_spring_blossoms'/(kind+'_tex')) for kind in ('blossom','petal')}
    # Portable data export contains the same packed integer coordinates/colors.
    (args.output/'spring-resource-meshes.json').write_text(json.dumps(meshes,indent=2)+'\n')
    sheet=Image.new('RGB',(1400,1260),(243,240,234));d=ImageDraw.Draw(sheet)
    d.text((28,18),'SPRING SURFACE ITERATION | actual native meshes and serialized textures',fill=(35,39,43))
    d.text((28,39),'Before: e22bcb22 untextured | After: generated art + UVs + cupping | software preview, runtime untested',fill=(65,68,72))
    for row,kind in enumerate(('blossom','petal')):
        y=80+row*580
        for col,(stage,angle) in enumerate((('before',0),('after',0),('before',58),('after',58))):
            x=26+col*344
            d.text((x,y),f'{kind} {stage} | '+('front' if angle==0 else '58 deg depth inspection'),fill=(35,39,43))
            image=composite(raster(meshes[stage][kind],textures[kind],300,angle,flutter=1), (41,53,60))
            sheet.paste(image,(x,y+22))
        d.text((28,y+334),'Game-scale readability: 16 / 32 / 64 px native bounding box; flutter 0.65, alpha 220/255',fill=(35,39,43))
        for stage_i,stage in enumerate(('before','after')):
            x=28+stage_i*686
            d.text((x,y+358),stage,fill=(35,39,43))
            for bgi,bg in enumerate(((221,227,217),(41,53,60))):
                strip=Image.new('RGB',(620,80),bg);sd=ImageDraw.Draw(strip)
                for size_i,size in enumerate((16,32,64)):
                    image=composite(raster(meshes[stage][kind],textures[kind],size),bg)
                    strip.paste(image,(50+size_i*180,(80-size)//2))
                sheet.paste(strip,(x,y+378+bgi*82))
    sheet.save(args.output/'spring-before-after.png')
    print(args.output/'spring-before-after.png')
    print('Native triangle counts:',{stage:{k:len(v) for k,v in model.items()} for stage,model in meshes.items()})

if __name__=='__main__': main()
