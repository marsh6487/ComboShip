#!/usr/bin/env python3
"""Offline lighting approximation; reads only serialized production XML and texels."""
from __future__ import annotations
import argparse, copy, hashlib, json, struct, sys
from pathlib import Path
import xml.etree.ElementTree as ET
import numpy as np
from PIL import Image, ImageDraw, ImageFont
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT))
from tools.nei_icons.render import Model,Part,Material,unit
from tools.authored_door_keys.offline_render import render
from tools.authored_door_keys.build import NS,OWNERS


def read_texture(path):
    raw=path.read_bytes();typ,w,h,size=struct.unpack_from('<4I',raw,64)
    assert typ in (1,2) and size==w*h*(4 if typ==1 else 2) and len(raw)==80+size
    if typ==1:return np.frombuffer(raw,dtype=np.uint8,offset=80).reshape(h,w,4).astype(float)/255
    v=np.frombuffer(raw, dtype='>u2',offset=80).reshape(h,w).astype(np.uint32)
    return np.stack([(((v>>b)&31)*255//31)/255 for b in (11,6,1)]+[(v&1).astype(float)],axis=-1)


def model(name):
    base=ROOT/OWNERS[name]/'assets/custom';m=Model();cache={};state=Material();tile=(32,32);state.decal=False;state.clamp=False;env=[1,1,1,1]
    def read(path,stack=()):
        nonlocal tile, env
        assert path.startswith(NS) and path not in stack,path
        f=base/path;m.sources.add(f)
        for c in ET.parse(f).getroot():
            a=c.attrib
            if c.tag=='CallDisplayList':read(a['Path'],stack+(path,))
            elif c.tag=='LoadVertices':
                source=base/a['Path'];m.sources.add(source);vs=list(ET.parse(source).getroot())
                offset,count,slot=[int(a[x]) for x in ('VertexOffset','Count','VertexBufferIndex')]
                assert 0<count<=30 and slot+count<=30 and offset+count<=len(vs)
                for i,v in enumerate(vs[offset:offset+count]):
                    b=v.attrib;cache[slot+i]=([int(b[k]) for k in ('X','Y','Z')],unit([((int(b[k])+128)%256-128) for k in ('R','G','B')]),[int(b[k])/(32*tile[j]) for j,k in enumerate(('S','T'))])
            elif c.tag=='Triangle1':
                v=[cache[int(a['V0'+str(i)])] for i in range(3)]
                m.parts.append(Part(np.array([x[0] for x in v]),np.array([x[1] for x in v]),np.array([x[2] for x in v]),np.array([[0,1,2]]),copy.copy(state)))
            elif c.tag=='LoadTextureBlock':
                f=base/a['Path'];m.sources.add(f);state.texture=read_texture(f);tile=(int(a['Width']),int(a['Height']));state.clamp='CMS_TXClamp' in a
            elif c.tag in ('SetGeometryMode','ClearGeometryMode'):
                if 'G_CULL_BACK' in a:state.cull=c.tag=='SetGeometryMode'
                if 'G_TEXTURE_GEN' in a:state.texgen=c.tag=='SetGeometryMode'
                if 'G_TEXTURE_GEN_LINEAR' in a:state.texgen_linear=c.tag=='SetGeometryMode'
            elif c.tag=='Texture':
                state.tex_scale=(int(a['S']),int(a['T']))
                if a['On']=='0':state.texture=None
            elif c.tag=='SetCombineLERP':
                state.environment=env if a['A0']=='G_CCMUX_PRIMITIVE' else None
            elif c.tag=='SetPrimColor':state.color=[int(a[k])/255 for k in 'RGBA']
            elif c.tag=='SetEnvColor':
                env=[int(a[k])/255 for k in 'RGBA']
                if state.environment is not None:state.environment=env
            elif c.tag=='SetRenderMode':state.decal='DECAL' in a['Mode2']
    read(NS+name+'DL');return m


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=Path(__file__).parent/'previews/sheen');parser.add_argument('--animation',action='store_true');args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    fontpath='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
    font=ImageFont.truetype(fontpath,20);small=ImageFont.truetype(fontpath,15)
    sheet=Image.new('RGB',(1500,1140),(24,27,32));d=ImageDraw.Draw(sheet)
    d.text((30,15),'AUTHORED DOOR KEYS  /  Native metallic sheen beneath static weathering',font=font,fill=(230,226,214))
    d.text((30,45),'OFFLINE LIGHTING / LOOK-AT APPROXIMATION — serialized native two-pass materials; local key roll −45°.',font=small,fill=(166,174,180))
    hashes={}
    for row,name in enumerate(OWNERS):
        m=model(name)
        for col,(label,angle,elevation) in enumerate([('Front',0,0),('Three-quarter',38,10),('Back',180,0)]):
            frame=render(m,size=490,azimuth=angle,elevation=elevation,supersample=2,fill=.88)
            p=args.output/(name+'-'+label.lower()+'.png');frame.save(p);hashes[p.name]=hashlib.sha256(p.read_bytes()).hexdigest()
            x=col*500;y=75+row*530;sheet.paste(frame,(x,y),frame)
            d.text((x+28,y+485),('Rusty iron' if row==0 else 'Aged brass')+' / '+label,font=font,fill=(224,218,201))
            icon=render(m,size=96,azimuth=angle,elevation=elevation,supersample=2,fill=.88);sheet.paste(icon,(x+(12 if label=='Back' else 386),y+370),icon)
        for texture in sorted(p for p in m.sources if p.name.endswith('Tex')):
            Image.fromarray(np.uint8(read_texture(texture)*255)).resize((256,256),Image.Resampling.NEAREST).save(args.output/(texture.name+'.png'))
    sheet.save(args.output/'door-keys-contact-sheet.png')
    (args.output/'preview-hashes.json').write_text(json.dumps(hashes,indent=2)+'\n')
    print(args.output/'door-keys-contact-sheet.png')
    if args.animation:
        frames=[];models=[model(n) for n in OWNERS]
        for angle in range(0,360,10):
            canvas=Image.new('RGB',(900,530),(24,27,32));draw=ImageDraw.Draw(canvas)
            draw.text((20,15),'NATIVE TEXGEN SHEEN  /  static rust & patina coat',font=font,fill=(230,226,214))
            draw.text((20,45),'Offline lighting/look-at approximation · real serialized materials · local −45° roll',font=small,fill=(166,174,180))
            for i,m in enumerate(models):
                frame=render(m,size=440,azimuth=angle,elevation=8,supersample=1,fill=.86)
                canvas.paste(frame,(i*450,70),frame)
                draw.text((25+i*450,500),'Rusty iron' if i==0 else 'Aged brass',font=font,fill=(224,218,201))
            frames.append(canvas)
        frames[0].save(args.output/'door-keys-sheen-rotation.gif',save_all=True,append_images=frames[1:],duration=90,loop=0,optimize=False)
        print(args.output/'door-keys-sheen-rotation.gif')
    for name in ('door-keys-contact-sheet.png','door-keys-sheen-rotation.gif'):
        path=args.output/name
        if path.is_file():hashes[name]=hashlib.sha256(path.read_bytes()).hexdigest()
    (args.output/'preview-hashes.json').write_text(json.dumps(hashes,indent=2)+'\n')
if __name__=='__main__':main()
