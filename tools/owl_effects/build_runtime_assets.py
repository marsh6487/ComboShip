"""Deterministic runtime resources for the accepted translucent owl-flame POC4.

Requires only numpy and the retained native o2r reader/serializer. No base
geometry, closed pose, page artwork, or existing textures are rewritten.
"""
from pathlib import Path
import argparse
from collections import Counter
import hashlib
import json
import struct
import sys
import zipfile

import numpy as np

ROOT = Path(__file__).resolve().parent
HELPERS = ROOT/'porting'
sys.path.insert(0, str(HELPERS))
from o2r import crc64, DLBuilder, _header, T_OTEX, TEX_RGBA32, read_texture, read_vertex_array
from flame_field import flame_texture

BASELINE_SHA256 = 'c8922c8397cdd5de61196cbbc2268ecc75b7dfbee99dedb2a3fcbd727cbc9d04'
PREFIX = 'objects/owl_effects_poc4/'
REMOVE = {
    'objects/owl_reconstruction_poc3/Opened/Vertices_061': 825,
    'objects/owl_reconstruction_poc3/Opened/Vertices_062': 825,
    'objects/owl_reconstruction_poc3/page_glow_poc1/PageSurfaceVertices': 384,
}
WIDE_OPS = (0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x37)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def commands(data):
    assert data[4:8] == b'TLDO'
    offset = 72
    while offset < len(data):
        start = offset
        w0,w1 = struct.unpack_from('<II',data,offset)
        offset += 8
        op = w0 >> 24
        reference = None
        if op in WIDE_OPS:
            hi,lo = struct.unpack_from('<II',data,offset)
            reference = hi << 32 | lo
            offset += 8
        yield op,w0,w1,reference,data[start:offset]
        if op == 0xdf:
            assert offset == len(data)
            return
    raise AssertionError('unterminated display list')


def filter_display_list(data, hashes):
    cache = {}
    kept = bytearray(data[:72])
    removed = Counter()
    total = Counter()
    for op,w0,w1,reference,raw in commands(data):
        omit = False
        if op == 0x32:
            path = hashes[reference]
            count = (w0 >> 12) & 255
            destination = ((w0 >> 1) & 127) - count
            for i in range(count):
                cache[destination+i] = (path,w1//16+i)
            omit = path in REMOVE
        elif op in (0x05,0x06):
            sources = []
            for value in [w0] + ([w1] if op == 0x06 else []):
                ids = [((value>>16)&255)//2,((value>>8)&255)//2,(value&255)//2]
                paths = {cache[i][0] for i in ids}
                assert len(paths) == 1, 'mixed-source triangle'
                path = paths.pop()
                sources.append(path)
                total[path] += 1
            omitted = [p in REMOVE for p in sources]
            assert all(omitted) or not any(omitted), 'mixed TRI2 removal'
            omit = all(omitted)
            if omit:
                removed.update(sources)
        if not omit:
            kept.extend(raw)
    return bytes(kept),dict(removed),dict(total)


def build_texture(frame,eye,size=32):
    y,x=np.mgrid[:size,:size]
    u=2*(x.ravel()+.5)/size-1
    v=1-(y.ravel()+.5)/size
    rgb,alpha=flame_texture(u,v,frame,eye)
    rgba=np.rint(np.c_[rgb,alpha*255]).clip(0,255).astype(np.uint8)
    body=rgba.tobytes()
    return _header(T_OTEX)+struct.pack('<IIII',TEX_RGBA32,size,size,len(body))+body


def write_header(vertices,path):
    lines=['// Generated from unchanged POC8 page glow vertex resource.',
           '// Positions and RGBA are exact raw actor units; normalize before NeiGi draw.',
           '#pragma once','#include <cstddef>','#include <cstdint>',
           'namespace ComboOwlPageGlow {',
           'struct Vertex { int16_t x, y, z; uint32_t rgb; uint8_t alpha; };',
           'inline constexpr Vertex kVertices[] = {']
    for x,y,z,flag,s,t,r,g,b,a in vertices:
        lines.append(f'    {{ {x}, {y}, {z}, 0x{r:02X}{g:02X}{b:02X}u, {a} }},')
    lines += ['};','inline constexpr size_t kVertexCount = sizeof(kVertices) / sizeof(kVertices[0]);',
              'static_assert(kVertexCount == 1152, "POC8 page glow triangle count");',
              '} // namespace ComboOwlPageGlow','']
    path.write_text('\n'.join(lines))


def build(baseline,output,header):
    assert digest(baseline.read_bytes()) == BASELINE_SHA256, 'Unreviewed baseline'
    with zipfile.ZipFile(baseline) as z:
        original={n:z.read(n) for n in z.namelist()}
    resources=dict(original)
    hashes={crc64(n):n for n in original}
    changes=[]
    for path,data in original.items():
        if data[4:8] != b'TLDO':
            continue
        candidate,removed,total=filter_display_list(data,hashes)
        if removed:
            assert path in ('objects/object_sek/gOwlStatueOpenedDL','alt/objects/object_sek/gOwlStatueOpenedDL')
            assert removed == REMOVE
            resources[path]=candidate
            changes.append({'path':path,'removed_triangles_by_source':removed,
                            'remaining_triangles':sum(total.values())-sum(removed.values()),
                            'before_sha256':digest(data),'after_sha256':digest(candidate)})
    assert len(changes)==2
    for eye,label in [(-1,'left'),(1,'right')]:
        for index in range(60):
            data=build_texture(index*3,eye)
            assert read_texture(data)[:3] == (TEX_RGBA32,32,32)
            for prefix in ('','alt/'):
                resources[f'{prefix}{PREFIX}flames/{label}_{index:03}']=data
    for prefix in ('','alt/'):
        path=f'{prefix}{PREFIX}Enabled'
        marker=DLBuilder();marker.marker(path);marker.end()
        resources[path]=marker.to_resource()
    resource_hashes={crc64(n):n for n in resources}
    assert len(resource_hashes)==len(resources), 'Resource hash collision'
    for path,data in resources.items():
        if data[4:8]==b'TLDO':
            for op,w0,w1,reference,raw in commands(data):
                if reference is not None:
                    assert reference in resource_hashes, (path,reference)
    output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(output,'w',zipfile.ZIP_STORED) as z:
        for path in sorted(resources):
            info=zipfile.ZipInfo(path,date_time=(1980,1,1,0,0,0))
            info.compress_type=zipfile.ZIP_STORED
            info.external_attr=0o100644<<16
            z.writestr(info,resources[path])
    page_path='objects/owl_reconstruction_poc3/page_glow_poc1/PageSurfaceVertices'
    vertices=read_vertex_array(original[page_path])
    assert len(vertices)==1152
    header.parent.mkdir(parents=True,exist_ok=True)
    write_header(vertices,header)
    changed_paths={c['path'] for c in changes}
    assert all(resources[n]==b for n,b in original.items() if n not in changed_paths)
    # Header is triangle-order exact: the direct list consumes vertices sequentially.
    page_indices=[];cache={}
    for op,w0,w1,h,raw in commands(original['objects/object_sek/gOwlStatueOpenedDL']):
        if op==0x32:
            count=(w0>>12)&255;dst=((w0>>1)&127)-count
            for i in range(count):cache[dst+i]=(hashes[h],w1//16+i)
        elif op in (0x05,0x06):
            for value in [w0]+([w1] if op==0x06 else []):
                ids=[((value>>16)&255)//2,((value>>8)&255)//2,(value&255)//2]
                if cache[ids[0]][0]==page_path:page_indices += [cache[i][1] for i in ids]
    assert page_indices==list(range(1152)), 'Header triangle order differs'
    report={'candidate':output.name,'baseline':baseline.name,'baseline_sha256':BASELINE_SHA256,
            'candidate_sha256':digest(output.read_bytes()),'resource_count':len(resources),
            'modified_original_resources':changes,'unchanged_original_resource_count':len(original)-len(changes),
            'added_resources':len(resources)-len(original),'texture_dimensions':[32,32],
            'texture_format':'RGBA32','frames_per_eye':60,'preview_frames_sampled':'0,3,...177',
            'texture_paths':PREFIX+'flames/{left,right}_{000..059}',
            'texture_row_orientation':'top row mathematical v=1; bottom v=0; runtime bottom UV t=1, top t=0',
            'marker':PREFIX+'Enabled','native_alt_texture_parity':True,
            'all_display_list_references_resolve':True,'resource_hashes_unique':True,
            'flame_field':'Accepted POC4, portable flame_field.py','removed_eye_triangles':1650,'dynamic_page_glow_triangles':384,
            'static_page_border_triangles_preserved':960,'header_vertex_count':len(vertices),
            'page_bounds_raw':[np.array(vertices)[:,:3].min(0).tolist(),np.array(vertices)[:,:3].max(0).tolist()],
            'page_header_requires_position_normalization':True,'runtime':'UNTESTED'}
    (output.parent/'Asset_Validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('candidate','candidate_sha256','resource_count','unchanged_original_resource_count','added_resources','texture_dimensions','header_vertex_count','page_bounds_raw','runtime')},indent=2))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline',type=Path,required=True,help='Unchanged accepted POC8 archive; SHA256 is pinned')
    parser.add_argument('--output',type=Path,required=True,help='Candidate o2r output; replaces this path if present')
    parser.add_argument('--header',type=Path,required=True,help='Generated ComboOwlPageGlow.h output')
    args=parser.parse_args();build(args.baseline,args.output,args.header)
