#!/usr/bin/env python3
"""Verify native eye assets supplied by the user, without committing ROM data.
Usage: native_assets_test.py /path/to/mm.o2r /path/to/evidence-output
"""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zipfile
from PIL import Image, ImageDraw

root = Path(__file__).resolve().parents[2]
archive, output = Path(sys.argv[1]), Path(sys.argv[2])
output.mkdir(parents=True, exist_ok=True)
z = zipfile.ZipFile(archive)

def crc64(path):
    crc = (1 << 64) - 1
    for byte in path.encode():
        crc ^= byte << 56
        for _ in range(8):
            crc = ((crc << 1) ^ (0x42f0e1eba9ea3693 if crc & (1 << 63) else 0)) & ((1 << 64) - 1)
    return crc

def decode(raw):
    values = struct.unpack('>2048H', raw)
    im = Image.new('RGBA', (64, 32))
    im.putdata([((v >> 11)*255//31, ((v >> 6)&31)*255//31, ((v >> 1)&31)*255//31, (v&1)*255) for v in values])
    return im

cpp = r'''
#include "soh/mods/transformation_masks/mm_form_blink_pixels.h"
#include <fstream>
#include <array>
#include <cassert>
int main(int argc, char** argv) {
    assert(argc==4);
    std::array<uint8_t,MmFormBlink::Bytes> source{}, target{};
    std::ifstream(argv[1],std::ios::binary).read(reinterpret_cast<char*>(source.data()),source.size());
    for (unsigned state=1;state<=2;++state) {
        assert(MmFormBlink::Derive(source.data(),source.size(),argv[3][0]=='1',state,target.data()));
        std::ofstream(std::string(argv[2])+std::to_string(state),std::ios::binary).write(reinterpret_cast<char*>(target.data()),target.size());
    }
}
'''
report = {'archive_sha256': hashlib.sha256(archive.read_bytes()).hexdigest(), 'gameplay': 'not run', 'forms': {}}
review = Image.new('RGB', (1536, 2*280), '#303030')
draw = ImageDraw.Draw(review)
with tempfile.TemporaryDirectory() as temp:
    temp = Path(temp)
    (temp/'derive.cpp').write_text(cpp)
    subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror','-I',str(root),str(temp/'derive.cpp'),'-o',str(temp/'derive')],check=True)
    for row, (form, folder, texture_name, head_name) in enumerate([
        ('FD','object_link_boy','gLinkFierceDeityEyesTex','gLinkFierceDeityHeadDL'),
        ('Deku','object_link_nuts','object_link_nuts_Tex_0048B0','gLinkDekuHeadDL')]):
        texture_path = f'objects/{folder}/{texture_name}'
        head_path = f'objects/{folder}/{head_name}'
        data = z.read(texture_path)
        assert struct.unpack_from('<IIII',data,64) == (2,64,32,4096)
        raw = data[80:]; assert len(raw)==4096
        (temp/'source').write_bytes(raw)
        subprocess.run([str(temp/'derive'),str(temp/'source'),str(temp/'state'),str(row)],check=True)
        frames = [raw]+[(temp/f'state{i}').read_bytes() for i in (1,2)]
        assert len(set(frames))==3
        values = [struct.unpack('>2048H',f) for f in frames]
        if row==0:
            for state in (1,2):
                assert all((a&1)==(b&1) for a,b in zip(values[0],values[state]))
                assert frames[state][:15*64*2] == raw[:15*64*2], 'brows/forehead changed'
                for y in range(19,32):
                    for x in list(range(12))+list(range(52,64)):
                        v=values[0][y*64+x];r,g,b=v>>11,(v>>6)&31,(v>>1)&31
                        if r>12 and r>g*2 and r>b*2: assert values[state][y*64+x]==v, 'cheek slash changed'
            bright=lambda v: min(v>>11,(v>>6)&31,(v>>1)&31)>23
            counts=[sum(map(bright,v)) for v in values]
            assert counts[0]>counts[1]>counts[2] and counts[2]==0
        else:
            counts=[sum(v&1 for v in state) for state in values]
            assert counts[0]>counts[1]>counts[2]>0
            for state in (1,2):
                assert all((a&~1)==(b&~1) and (not b&1 or a&1) for a,b in zip(values[0],values[state]))
        head = z.read(head_path); matches=[]; i=72
        while i<len(head):
            a,b=struct.unpack_from('<II',head,i);op=a>>24
            if op==0x20:
                c,d=struct.unpack_from('<II',head,i+8)
                if (c<<32)|d == crc64(texture_path): matches.append(i)
            i+=16 if op in (0x20,0x31,0x32,0x33,0x35,0x36,0x42) else 8
        assert len(matches)==1, (form,matches)
        report['forms'][form]={'texture':texture_path,'texture_sha256':hashlib.sha256(data).hexdigest(),
            'head':head_path,'head_sha256':hashlib.sha256(head).hexdigest(), 'exact_native_eye_hash_loads':len(matches),
            'eye_hash_load_file_offset':matches[0], 'open_half_closed_counts':counts,
            'count_kind':'white eye pixels' if row==0 else 'opaque pixels',
            'frame_sha256':[hashlib.sha256(f).hexdigest() for f in frames]}
        for col,frame in enumerate(frames):
            image=decode(frame).resize((512,256),Image.Resampling.NEAREST)
            review.paste(image,(col*512,row*280+24),image)
            draw.text((col*512+8,row*280+6),f'{form}: {("Open","Half","Closed")[col]}',fill='white')
review.save(output/'native_blink_review.png')
(output/'native_blink_verification.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
