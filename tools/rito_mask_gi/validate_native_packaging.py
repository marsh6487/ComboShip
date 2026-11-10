#!/usr/bin/env python3
"""Stage only Rito assets, invoke the native OoT packer, validate both host archives.
MM results establish Torch packaging/copy-through and texture conversion, not a
full MM ZAPD build. Runtime drawing, shimmer and gameplay are separate evidence.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import xml.etree.ElementTree as ET
import zipfile
from PIL import Image
import numpy as np
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT))
from tools.rito_mask_gi.build import NS,ICON_FILES


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def source_equivalence(library_source,packer_source,library,packer,probe):
    groups={}
    groups['native_library']=['libultraship/src/fast/resource/factory/'+n+'Factory.cpp' for n in ('DisplayList','Vertex','Texture','Matrix')]
    groups['native_library'] += ['libultraship/include/fast/resource/factory/'+n+'Factory.h' for n in ('DisplayList','Vertex','Texture','Matrix')]
    groups['native_library'] += ['libultraship/src/ship/resource/ResourceLoader.cpp','libultraship/include/ship/resource/ResourceLoader.h']
    groups['native_library'] += ['libultraship/'+prefix+'/fast/resource/type/'+n+suffix for prefix,suffix in [('src','.cpp'),('include','.h')] for n in ('DisplayList','Vertex','Texture','Matrix')]
    groups['native_packer']=['soh/assets/tools/soh-o2r-packer/'+n for n in ('main.cpp','PngTexture.cpp','PngTexture.h')]+['torch/src/utils/StringHelper.cpp']
    report={}
    for group,paths in groups.items():
        prior=library_source if group=='native_library' else packer_source
        report[group]=[]
        for rel in paths:
            current=ROOT/rel;retained=prior/rel
            assert current.is_file() and retained.is_file(),rel
            a,b=sha(current),sha(retained);assert a==b,('Source differs',rel,a,b)
            report[group].append({'path':rel,'candidate_sha256':a,'retained_build_source_sha256':b,'byte_equal':True})
    report['binaries']={str(p):{'bytes':p.stat().st_size,'sha256':sha(p)} for p in (library,packer,probe)}
    return report


def validate_texture(data,png,format):
    assert struct.unpack_from('<II',data,4)==(0x4f544558,0)
    kind,w,h,size=struct.unpack_from('<4I',data,64)
    image=Image.open(png).convert('RGBA');assert image.size==(w,h)
    pixels=np.asarray(image,dtype=np.uint8)
    if format=='rgba32':
        assert (kind,size)==(1,w*h*4)
        assert data[80:]==pixels.tobytes(),'RGBA32 conversion must preserve every original pixel'
    else:
        assert format=='ia4' and (kind,size)==(7,w*h//2)
        value=((pixels[:,:,0]>>5)<<1)|(pixels[:,:,3]!=0)
        encoded=((value[:,::2]<<4)|value[:,1::2]).astype(np.uint8).tobytes()
        assert data[80:]==encoded,'IA4 quantization must match native packer algorithm'
    assert len(data)==80+size
    return {'kind':kind,'width':w,'height':h,'texel_bytes':size,'native_v0':True,'pixel_encoding_matches_source':True,'sha256':hashlib.sha256(data).hexdigest()}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for flag in ('output','packer','runtime-libs','library','library-source','packer-source','probe'):
        p.add_argument('--'+flag,type=Path,required=True)
    args=p.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    equivalence=source_equivalence(args.library_source,args.packer_source,args.library,args.packer,args.probe)
    (out/'source-equivalence.json').write_text(json.dumps(equivalence,indent=2)+'\n')
    env=dict(os.environ,LD_LIBRARY_PATH=str(args.runtime_libs.resolve())+':'+str(args.library.resolve().parent))
    results={};probe_dirs=[]
    for host in ('soh','mm'):
        src=ROOT/host/'assets/custom';stage=out/(host+'-rito-only-stage')
        if stage.exists():shutil.rmtree(stage)
        shutil.copytree(src/NS,stage/NS)
        for rel in ICON_FILES:
            dest=stage/rel;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src/rel,dest)
        archive=out/(host+'-rito-only.o2r');command=[str(args.packer.resolve()),str(stage),str(archive),'1.0.0']
        (out/(host+'-pack.command.json')).write_text(json.dumps(command,indent=2)+'\n')
        with (out/(host+'-pack.log')).open('w') as log:
            subprocess.run(command,cwd=out,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
        gi={NS+f.name:f.read_bytes() for f in (src/NS).iterdir() if f.is_file()}
        converted={rel.rsplit('.',2)[0]:rel for rel in ICON_FILES}
        dependencies=triangles=vertices=0;texture_results={}
        extracted=out/(host+'-packed-extracted')
        if extracted.exists():shutil.rmtree(extracted)
        with zipfile.ZipFile(archive) as z:
            assert z.testzip() is None
            assert set(z.namelist())==set(gi)|set(converted)|{'portVersion'},z.namelist()
            for name,data in gi.items():
                assert z.read(name)==data,('GI copy changed bytes',name)
                if not data.startswith(b'<'):continue
                xml=ET.fromstring(data)
                if xml.tag=='Vertex':vertices+=len(xml)
                for c in xml:
                    if c.tag=='Triangle1':triangles+=1
                    if 'Path' in c.attrib:
                        assert c.attrib['Path'] in gi,(name,c.attrib['Path']);dependencies+=1
            for name,rel in converted.items():
                texture_results[name]=validate_texture(z.read(name),src/rel,rel.rsplit('.',2)[1])
            z.extractall(extracted)
        assert triangles==768 and vertices==2304
        probe_dirs += [str(extracted/NS),str(extracted/'textures/icon_item_custom'),str(extracted/'textures/item_name_custom')]
        results[host]={'resources':len(gi)+len(converted),'gi_resources':len(gi),'triangles':triangles,'vertices':vertices,'resolved_gi_path_references':dependencies,
            'gi_bytes_unchanged':True,'textures':texture_results,'archive_bytes':archive.stat().st_size,'archive_sha256':sha(archive),
            'pipeline':'Actual native soh-o2r-packer' if host=='soh' else 'Actual native soh-o2r-packer on MM-owned input: Torch copy-through/archive proof; full MM ZAPD not executed'}
    command=[str(args.probe.resolve()),*probe_dirs]
    (out/'packed-resource-probe.command.json').write_text(json.dumps(command,indent=2)+'\n')
    with (out/'packed-resource-probe.log').open('w') as log:
        subprocess.run(command,cwd=out,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
    text=(out/'packed-resource-probe.log').read_text();assert 'PASS resources=20 actual native libultraship factories' in text,text
    results['native_factory_probe']={'resources':20,'passed':True,'includes_converted_icon_and_name':True}
    (out/'packaging_results.json').write_text(json.dumps(results,indent=2)+'\n')
    print(json.dumps(results,indent=2))

if __name__=='__main__':main()
