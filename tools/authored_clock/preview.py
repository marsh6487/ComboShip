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
from tools.authored_clock.clock_render import render
from tools.authored_clock.build import NS
OWNERS={"gNeiClock":"soh"}


def read_texture(path):
    raw=path.read_bytes();typ,w,h,size=struct.unpack_from('<4I',raw,64)
    assert typ in (1,2) and size==w*h*(4 if typ==1 else 2) and len(raw)==80+size
    if typ==1:return np.frombuffer(raw,dtype=np.uint8,offset=80).reshape(h,w,4).astype(float)/255
    v=np.frombuffer(raw, dtype='>u2',offset=80).reshape(h,w).astype(np.uint32)
    return np.stack([(((v>>b)&31)*255//31)/255 for b in (11,6,1)]+[(v&1).astype(float)],axis=-1)


def model(name,include_glass=True,phase=0):
    """Evaluate serialized body; TexDL -> caller tile offset -> GlassDL.

    phase is the native tile-0 X origin in quarter-texel units. The runtime
    supplies uint32(gameplayFrames)&127, interpolating towards phase+1.
    """
    base=ROOT/OWNERS[name]/'assets/custom';m=Model();cache={};state=Material();tile=(32,32);state.decal=False;state.clamp=False;env=[1,1,1,1]
    state.uv_offset=(0,0);state.depth_write=True;state.unlit=False
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
                f=base/a['Path'];m.sources.add(f);state.texture=read_texture(f);tile=(int(a['Width']),int(a['Height']));state.clamp='CMS_TXClamp' in a;state.uv_offset=(0,0)
            elif c.tag in ('SetGeometryMode','ClearGeometryMode'):
                if 'G_CULL_BACK' in a:state.cull=c.tag=='SetGeometryMode'
                if 'G_TEXTURE_GEN' in a:state.texgen=c.tag=='SetGeometryMode'
                if 'G_TEXTURE_GEN_LINEAR' in a:state.texgen_linear=c.tag=='SetGeometryMode'
            elif c.tag=='Texture':
                state.tex_scale=(int(a['S']),int(a['T']))
                if a['On']=='0':state.texture=None
            elif c.tag=='SetCombineLERP':
                state.environment=env if a['A0']=='G_CCMUX_PRIMITIVE' else None
                state.unlit=a['A0']=='G_CCMUX_TEXEL0' and a['C0']=='G_CCMUX_PRIMITIVE'
            elif c.tag=='SetPrimColor':state.color=[int(a[k])/255 for k in 'RGBA']
            elif c.tag=='SetEnvColor':
                env=[int(a[k])/255 for k in 'RGBA']
                if state.environment is not None:state.environment=env
            elif c.tag=='SetRenderMode':
                state.decal='DECAL' in a['Mode2'];state.depth_write='OPA_SURF' in a['Mode2']
    read(NS+name+'DL')
    if include_glass:
        read(NS+name+'GlassTexDL')
        # This is the only external draw input. Actual native UVs and loaded
        # texels remain unmodified; the texture origin moves exactly as in RDP.
        state.uv_offset=(-float(phase)/(4*tile[0]),0)
        read(NS+name+'GlassDL')
    return m


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True,help='Static preview destination outside the source checkout')
    args=parser.parse_args();args.output=args.output.resolve()
    if args.output.is_relative_to(ROOT):parser.error('Preview output must stay outside the source checkout')
    args.output.mkdir(parents=True,exist_ok=True)
    fontpath='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
    title=ImageFont.truetype(fontpath,27);font=ImageFont.truetype(fontpath,22);small=ImageFont.truetype(fontpath,16)
    sheet=Image.new('RGB',(1800,1170),(24,27,32));d=ImageDraw.Draw(sheet)
    d.text((28,20),'HYLIAN TIMEPIECE  /  Native scrolling glass candidate',font=title,fill=(237,223,191))
    d.text((28,60),'Accepted body unchanged · actual serialized lens, UVs and alpha ramp · three static native tile-scroll phases',font=small,fill=(174,184,192))
    hashes={};evidence=[]
    views=[('Accepted Clock','No glass · approved body','clock-baseline-front.png',False,0,0,0,0,100),
           ('Sweep · left','Tile X = 96 quarter texels','clock-glass-phase-096.png',True,96,0,0,1,100),
           ('Sweep · center','Tile X = 0 quarter texels','clock-glass-phase-000.png',True,0,0,0,2,100),
           ('Sweep · right','Tile X = 32 quarter texels','clock-glass-phase-032.png',True,32,0,0,3,100),
           ('Accepted · depth','Azimuth 48° / elevation 12°','clock-baseline-depth.png',False,0,48,12,0,650),
           ('Glass · depth','Same angle · tile X = 0','clock-glass-depth.png',True,0,48,12,1,650),
           ('Finished reverse','Back culling + body depth','clock-glass-reverse.png',True,0,180,-5,2,650)]
    models={}
    def get_model(glass,phase):
        key=(glass,phase)
        if key not in models:models[key]=model('gNeiClock',include_glass=glass,phase=phase)
        return models[key]
    for label,note,name,glass,phase,az,el,col,y in views:
        m=get_model(glass,phase)
        frame=render(m,size=420,azimuth=az,elevation=el,roll=0,supersample=2,fill=.9)
        p=args.output/name;frame.save(p);hashes[p.name]=hashlib.sha256(p.read_bytes()).hexdigest()
        sheet.paste(frame,(col*450+15,y),frame)
        d.text((col*450+28,y+426),label,font=font,fill=(224,218,201))
        d.text((col*450+28,y+458),note,font=small,fill=(166,174,180))
        evidence.append({'file':name,'include_glass':glass,'tile_x_quarter_texels':phase,'azimuth':az,'elevation':el})
    d.line((28,600,1772,600),fill=(64,69,74),width=1)
    d.text((1378,663),'At item size',font=font,fill=(224,218,201))
    d.text((1378,699),'96 px',font=small,fill=(166,174,180));d.text((1551,699),'64 px',font=small,fill=(166,174,180))
    for row,(label,phase) in enumerate([('Left',96),('Center',0),('Right',32)]):
        y=730+row*115
        for x,size in [(1378,96),(1551,64)]:
            icon=render(get_model(True,phase),size=size,azimuth=0,elevation=0,roll=0,supersample=2,fill=.9)
            sheet.paste(icon,(x,y),icon)
            p=args.output/f'clock-glass-phase-{phase:03d}-{size}px.png';icon.save(p);hashes[p.name]=hashlib.sha256(p.read_bytes()).hexdigest()
        d.text((1645,y+32),label,font=small,fill=(166,174,180))
    d.text((28,1139),'Offline lights/look-at, coverage and filtering are approximations. Intrinsic glass shown; optional ambient NEI shimmer omitted. No animation output.',font=small,fill=(148,158,166))
    p=args.output/'clock-contact-sheet.png';sheet.save(p);hashes[p.name]=hashlib.sha256(p.read_bytes()).hexdigest()
    (args.output/'preview-hashes.json').write_text(json.dumps(hashes,indent=2)+'\n')
    (args.output/'preview-evidence.json').write_text(json.dumps({'native_sequence':['gNeiClockDL','gNeiClockGlassTexDL','caller SetTileSizeLerp input','gNeiClockGlassDL'],
        'scroll_input':'uint32(gameplayFrames) & 127; next phase + 1; tile 0; 32x32 wrap',
        'views':evidence,'sources':{str(f.relative_to(ROOT)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(get_model(True,0).sources)},
        'limitations':'Static CPU evaluation of actual native resources; fixed lights/look-at, rasterization, coverage and filtering are approximations; no host GPU or gameplay proof.'},indent=2)+'\n')
    print(p)
if __name__=='__main__':main()
