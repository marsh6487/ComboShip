"""Render the fixture-exported owned FX geometry; not an engine/runtime capture.

Uses the candidate's actual vertices, UVs, colors and native I8 strip with ordinary
alpha blending. Character animation and existing native lightning sprites are omitted.
"""
import argparse
import json
from pathlib import Path
import subprocess

import numpy as np
from PIL import Image, ImageDraw, ImageFont


ROWS = [(72,224),(80,240),(92,255),(80,240),(68,216),(80,240),(92,255),(84,248),
        (72,224),(84,248),(92,255),(80,240),(68,216),(80,240),(92,255),(80,240)]
STRIP = np.array([[0,12,a,b,b,a,12,0] for a,b in ROWS], dtype=np.float32) / 255


def project(points, width, height):
    x,y,z = np.asarray(points).T
    scale = width / 980
    return np.stack([width/2 + (x-z)*.707*scale,
                     height*.56 + (x+z)*.39*scale - y*scale], axis=1)


def font(size):
    return ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', size)


def backdrop(width,height):
    im=Image.new('RGB',(width,height),(20,22,29))
    dr=ImageDraw.Draw(im)
    for n in range(-500,501,100):
        for pts in [[(n,0,-500),(n,0,500)],[(-500,0,n),(500,0,n)]]:
            p=project(pts,width,height)
            dr.line([tuple(x) for x in p],fill=(29,32,41),width=1)
    return np.asarray(im,dtype=np.float32)/255


def triangle(canvas, screen, rgba, uv):
    height,width=canvas.shape[:2]
    lo=np.maximum(np.floor(screen.min(0)).astype(int),[0,0])
    hi=np.minimum(np.ceil(screen.max(0)).astype(int),[width-1,height-1])
    if np.any(hi<lo): return
    a,b,c=screen
    den=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
    if abs(den)<1e-5:return
    yy,xx=np.mgrid[lo[1]:hi[1]+1,lo[0]:hi[0]+1].astype(np.float32)
    xx+=.5;yy+=.5
    w0=((b[1]-c[1])*(xx-c[0])+(c[0]-b[0])*(yy-c[1]))/den
    w1=((c[1]-a[1])*(xx-c[0])+(a[0]-c[0])*(yy-c[1]))/den
    w2=1-w0-w1
    inside=(w0>=0)&(w1>=0)&(w2>=0)
    if not inside.any():return
    weights=np.stack([w0,w1,w2],axis=-1)
    coords=weights@uv
    s=np.clip(coords[...,0]/32,0,7)
    t=(coords[...,1]/32)%16
    s0=np.floor(s).astype(int);s1=np.minimum(s0+1,7)
    t0=np.floor(t).astype(int);t1=(t0+1)%16
    sf=s-s0;tf=t-t0
    value=((STRIP[t0,s0]*(1-sf)+STRIP[t0,s1]*sf)*(1-tf)
           +(STRIP[t1,s0]*(1-sf)+STRIP[t1,s1]*sf)*tf)
    color=(weights@rgba)/255
    color*=value[...,None]
    alpha=color[...,3:4]*inside[...,None]
    region=canvas[lo[1]:hi[1]+1,lo[0]:hi[0]+1]
    region[:]=color[...,:3]*alpha+region*(1-alpha)


def render(frame, width=900, height=700):
    canvas=backdrop(width,height)
    for quad in frame['quads']:
        q=np.asarray(quad,dtype=np.float32)
        screen=project(q[:,:3],width,height)
        for indices in [[0,1,2],[0,2,3]]:
            triangle(canvas,screen[indices],q[indices,3:7],q[indices,7:9])
    im=Image.fromarray(np.uint8(np.clip(canvas*255,0,255)))
    dr=ImageDraw.Draw(im)
    dr.text((28,22),'DEMISE’S DESTRUCTION',font=font(26),fill=(235,227,247))
    phase='CHARGE' if frame['phase']==1 else ('DISCHARGE' if frame['tick']<75 else 'AFTERGLOW')
    dr.text((28,61),phase,font=font(15),fill=(182,126,237))
    dr.text((28,height-47),'Offline preview of production geometry',font=font(15),fill=(160,163,178))
    dr.text((28,height-25),'Character animation and native lightning sprites omitted',font=font(12),fill=(108,113,130))
    return im


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('geometry',type=Path)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    frames=json.loads(args.geometry.read_text())
    args.output.mkdir(parents=True,exist_ok=True)
    movie=args.output/'Demise_Electrical_POC_Animation.mp4'
    encoder=subprocess.Popen(['ffmpeg','-hide_banner','-loglevel','error','-y','-f','rawvideo',
        '-pix_fmt','rgb24','-s','900x700','-r','20','-i','pipe:0','-an','-c:v','libx264',
        '-preset','fast','-crf','18','-pix_fmt','yuv420p','-movflags','+faststart',str(movie)],stdin=subprocess.PIPE)
    try:
        for frame in frames:
            encoder.stdin.write(render(frame).tobytes())
        encoder.stdin.close()
        if encoder.wait()!=0:raise RuntimeError('Video encoding failed')
    except Exception:
        encoder.kill()
        raise
    poster=Image.new('RGB',(1800,470),(20,22,29))
    for i,tick in enumerate([66,74,82]):
        frame=next(x for x in frames if x['tick']==tick)
        poster.paste(render(frame,600,470),(i*600,0))
    poster.save(args.output/'Demise_Electrical_POC_Stages.png')
    print(movie)
    print(args.output/'Demise_Electrical_POC_Stages.png')


if __name__=='__main__':main()
