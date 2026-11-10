#!/usr/bin/env python3
"""Static GI proof from serialized production resources; approximate native lighting."""
from __future__ import annotations
import argparse
import copy
import hashlib
import json
from pathlib import Path
import struct
import sys
import xml.etree.ElementTree as ET
import numpy as np
from PIL import Image, ImageDraw, ImageFont
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT))
from tools.nei_icons.render import Model, Part, Material, unit
from tools.authored_door_keys.offline_render import render
from tools.authored_door_keys.preview import read_texture
from tools.rito_mask_gi.build import NS


def model(owner='soh'):
    base=ROOT/owner/'assets/custom';m=Model();cache={};state=Material();tile=(32,32);state.decal=False;state.clamp=False;env=[1,1,1,1];scales=[1.]
    def read(path,stack=()):
        nonlocal tile,env
        assert path.startswith(NS) and path not in stack,path
        f=base/path;m.sources.add(f)
        for c in ET.parse(f).getroot():
            a=c.attrib
            if c.tag=='CallDisplayList':read(a['Path'],stack+(path,))
            elif c.tag=='Matrix':
                source=base/a['Path'];m.sources.add(source);raw=source.read_bytes()
                assert len(raw)==128 and struct.unpack_from('<I',raw,4)[0]==0x4f4d5458
                words=struct.unpack_from('<16I',raw,64);values=[]
                for i in range(16):
                    shift=16 if i%2==0 else 0
                    integer=(words[i//2]>>shift)&65535;fraction=(words[8+i//2]>>shift)&65535
                    values.append((integer if integer<32768 else integer-65536)+fraction/65536)
                assert values[0]==values[5]==values[10]>0 and values[15]==1
                assert all(values[i]==0 for i in (1,2,3,4,6,7,8,9,11,12,13,14))
                assert a['Param']=='G_MTX_PUSH';scales.append(scales[-1]*values[0])
            elif c.tag=='PopMatrix':assert len(scales)>1;scales.pop()
            elif c.tag=='LoadVertices':
                source=base/a['Path'];m.sources.add(source);vs=list(ET.parse(source).getroot())
                offset,count,slot=[int(a[x]) for x in ('VertexOffset','Count','VertexBufferIndex')]
                assert 0<count<=30 and slot+count<=30 and offset+count<=len(vs)
                cache.clear()
                for i,v in enumerate(vs[offset:offset+count]):
                    b=v.attrib;cache[slot+i]=([int(b[k])*scales[-1] for k in ('X','Y','Z')],unit([((int(b[k])+128)%256-128) for k in ('R','G','B')]),[int(b[k])/(32*tile[j]) for j,k in enumerate(('S','T'))])
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
            elif c.tag=='SetCombineLERP':state.environment=env if a['A0']=='G_CCMUX_PRIMITIVE' else None
            elif c.tag=='SetPrimColor':state.color=[int(a[k])/255 for k in 'RGBA']
            elif c.tag=='SetEnvColor':
                env=[int(a[k])/255 for k in 'RGBA']
                if state.environment is not None:state.environment=env
            elif c.tag=='SetRenderMode':state.decal='DECAL' in a['Mode2']
    read(NS+'gi_dl');assert len(scales)==1
    return m


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=Path(__file__).parent/'previews');args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',21)
    small=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',16)
    sheet=Image.new('RGB',(1500,620),(27,31,38));draw=ImageDraw.Draw(sheet)
    draw.text((22,17),'RITO MASK / Authored native get-item sculpture',font=font,fill=(245,236,206))
    draw.text((22,49),'Serialized production mesh + native materials. Offline light / look-at approximation. Runtime acceptance pending.',font=small,fill=(170,181,193))
    m=model();hashes={}
    for i,(label,angle,elevation) in enumerate([('Front',0,0),('Three-quarter',38,8),('Back',180,0)]):
        frame=render(m,size=480,azimuth=angle,elevation=elevation,roll=0,supersample=2,fill=.88)
        path=args.output/('rito-mask-'+label.lower()+'.png');frame.save(path)
        hashes[path.name]=hashlib.sha256(path.read_bytes()).hexdigest()
        sheet.paste(frame,(i*500+10,85),frame)
        draw.text((i*500+26,577),label,font=font,fill=(242,232,201))
    path=args.output/'rito-mask-contact-sheet.png';sheet.save(path);hashes[path.name]=hashlib.sha256(path.read_bytes()).hexdigest()
    (args.output/'preview-hashes.json').write_text(json.dumps(hashes,indent=2)+'\n')
    print(path)

if __name__=='__main__':main()
