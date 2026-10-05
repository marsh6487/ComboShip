#!/usr/bin/env python3
"""Fit selected sword resource graphs from real supplied O2Rs through production code.

No mod geometry is checked in. --archives reads authorized files only while
building the fixture. Without that flag, a small XML-style resource fixture runs.
"""
import argparse, json, math, os, subprocess, tempfile
from pathlib import Path
from run_time_pedestal_tests import functions
from sword_archive_fixture import Archive, crc64, roots_for
ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--archives',type=Path)
parser.add_argument('--sanitize',action='store_true')
args=parser.parse_args()
owner=functions((ROOT/'combo/menu/ComboItemDrawOOT.h').read_text())
native=functions((ROOT/'soh/soh/Enhancements/randomizer/NeiGiPresentation.cpp').read_text())
production='\n'.join(owner[n] for n in ('CwSimple','CwCustomGi','CwAltSwordGi'))+'\n'+native['Spin']+'\n'+native['NeiGi_DrawSelectedSword']
fixture=(ROOT/'tests/sword_fallback/model_fit_test.cpp').read_text().replace('/* PRODUCTION */',production)

def data_for(archive):
    lists,vertices,expect={},{},[]
    available=set(archive.z.namelist())
    def visit(path):
        if path in lists:return
        lists[path]=[]
        xml=archive.z.read(path).lstrip().startswith(b'<')
        for op,w0,w1,ref in archive.commands(path):
            if op==0x31:
                assert ref;visit(ref)
                if xml: lists[path].append((0x27000000,'(uintptr_t)'+json.dumps('__OTR__'+ref)))
                else: lists[path].extend(((w0,w1),(crc64(ref)>>32,crc64(ref)&0xffffffff)))
            elif op==0x32:
                assert ref
                if ref not in vertices:
                    import struct,xml.etree.ElementTree as ET
                    raw=archive.z.read(ref)
                    if raw.lstrip().startswith(b'<'):
                        vertices[ref]=[tuple(int(v.attrib[k]) for k in ('X','Y','Z')) for v in ET.fromstring(raw)]
                    else:
                        kind,count=struct.unpack_from('<II',raw,64);assert kind==25
                        vertices[ref]=[struct.unpack_from('<hhh',raw,72+i*16) for i in range(count)]
                if xml: lists[path].extend(((0x24000000,'(uintptr_t)'+json.dumps('__OTR__'+ref)),((w0>>12)&255,w1//16)))
                else: lists[path].extend(((w0,w1),(crc64(ref)>>32,crc64(ref)&0xffffffff)))
            else:
                lists[path].append((0xdf000000 if op==0xdf else w0,w1))
                if op in (0x20,0x33,0x35,0x36,0x42):lists[path].append((0,0))
    def bounds(points):
        c,s=math.cos(1.8),math.sin(1.8)
        upright=[(float(x*c-y*s),float(x*s+y*c),float(z)) for x,y,z in points]
        return min(p[1] for p in upright),max(p[1] for p in upright),2*max(math.hypot(p[0],p[2]) for p in upright)
    for root in roots_for(archive):
        _,points,_=archive.mesh(root);visit(root)
        low,high,width=bounds(points);layer_low,layer_high,layer_width=low,high,width
        profile=0
        for index,(name,custom,marker) in enumerate((
          ('adult','gCustomMasterSwordDL','objects/object_link_boy/DinSleekEquipmentPOC1_OOT_Adult/SwordDL'),
          ('child','gCustomKokiriSwordDL','objects/object_link_child/DinSleekEquipmentPOC1_OOT_Child/SwordDL'),
          ('bgs','gCustomLongswordDL','objects/object_link_boy/DinSleekEquipmentPOC1_OOT_Adult/SwordDL'))):
            progressive=root==f'objects/din_fire_sword/progressive/{name}/SwordDL'
            selected=root==f'alt/objects/object_custom_equip/{custom}'
            required=[f'objects/din_fire_sword/poc1/{name}/{part}' for part in ('CoreDL','FlameDL','CoreVertices','FlameVertices')]
            required+=['objects/din_fire_sword/poc1/CoreTex','objects/din_fire_sword/poc1/FlameTex']
            if (progressive or selected and marker in available) and all(path in available for path in required):
                profile=index+1
                for path in required[:2]:
                    _,p,_=archive.mesh(path);visit(path)
                    b=bounds(p);layer_low=min(layer_low,b[0]);layer_high=max(layer_high,b[1]);layer_width=max(layer_width,b[2])
        expect.append((root,low,high,width,layer_low,layer_high,layer_width,profile))
    # Eligibility only queries Din dependencies and these two pack markers.
    # Keep their actual archive membership, without compiling thousands of
    # unrelated player textures into this geometry fixture.
    markers={'objects/object_link_boy/DinSleekEquipmentPOC1_OOT_Adult/SwordDL',
             'objects/object_link_child/DinSleekEquipmentPOC1_OOT_Child/SwordDL'}
    relevant={path for path in available if path.startswith('objects/din_fire_sword/') or path in markers}
    return lists,vertices,expect,relevant

def build(lists,vertices,expect,available):
    src='void InitArchive(){\n'
    for name,coords in vertices.items():
        src+='Vertex('+json.dumps(name)+f',{crc64(name)}ULL,'+'{'+','.join('{'+','.join(map(str,p))+'}' for p in coords)+'});\n'
    for name,commands in lists.items():
        src+='List('+json.dumps(name)+f',{crc64(name)}ULL,'+'{'+','.join('{'+str(w0)+','+str(w1)+'}' for w0,w1 in commands)+'});\n'
    for name,low,high,width,layer_low,layer_high,layer_width,profile in expect:
        src+='expected.push_back({'+json.dumps('__OTR__'+name)+f',{low:.9f}f,{high:.9f}f,{width:.9f}f,{layer_low:.9f}f,{layer_high:.9f}f,{layer_width:.9f}f,{profile}'+'});\n'
    for path in sorted(available):src+='available.insert('+json.dumps(path)+');\n'
    src+='}\n'
    return fixture.replace('/* ARCHIVES */',src)

archives=sorted(args.archives.glob('*.o2r')) if args.archives else [None]
assert archives,'No supplied O2Rs found'
with tempfile.TemporaryDirectory(prefix='sword-mod-gi-') as temporary:
    out=Path(temporary)
    for i,path in enumerate(archives):
        if path:
            data=data_for(Archive(path));print('ARCHIVE '+path.name,flush=True)
        else:
            root='alt/objects/object_custom_equip/gCustomMasterSwordDL';v='selected/vertices'
            points=[(-670,-268,-74),(-670,809,101),(4122,-268,-74),(4122,809,101)]
            c,s=math.cos(1.8),math.sin(1.8);upright=[(x*c-y*s,x*s+y*c,z) for x,y,z in points]
            b=(min(p[1] for p in upright),max(p[1] for p in upright),2*max(math.hypot(p[0],p[2]) for p in upright))
            data=({root:[(0x24000000,'(uintptr_t)'+json.dumps('__OTR__'+v)),(4,0),(0xdf000000,0)]},{v:points},[(root,*b,*b,0)],{root,v})
        source=out/f'fit-{i}.cpp';source.write_text(build(*data));binary=out/f'fit-{i}'
        command=[os.environ.get('CXX','c++'),'-std=c++20','-DF3DEX_GBI_2','-I'+str(ROOT),'-I'+str(ROOT/'libultraship/include'),str(source),'-o',str(binary)]
        if args.sanitize:command+=['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
        subprocess.run(command,check=True)
        subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
