"""Check actual Henriko pixels, leaf transparency and unchanged forest geometry."""
from pathlib import Path
import hashlib,json,sys,xml.etree.ElementTree as ET
import numpy as np
from PIL import Image

root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools/woods_mystery'))
from scene_assets import Resources,commands,crc64
from build_candidate import SCENE,ROOT,read_mesh_header

candidate=Resources(sys.argv[1]);native=Resources(sys.argv[2]);source=Path(sys.argv[3])
report=json.loads(candidate.get('woods-mystery-poc4.json') or candidate.get('woods-mystery-poc3.json') or candidate.get('woods-mystery-poc2.json'))
assert 'foliage' in report, 'Original low-resolution canopy materials are still active'
manifest=json.loads((source/'manifest.json').read_text())
for role,item in manifest['materials'].items():
    assert hashlib.sha256((source/item['file']).read_bytes()).hexdigest()==item['sha256']
    pixels=np.asarray(Image.open(source/item['file']).convert('RGBA'))
    assert np.array_equal(pixels,np.asarray(candidate.t(ROOT+'textures/henriko_'+role))), 'Henriko artwork or alpha changed'
    assert pixels[:,:,3].min()<64 and pixels[:,:,3].max()==255
masks=[SCENE+'Z2_26SARUNOMORITex_'+s for s in ['0091C0','0099C0']]
masks+=[SCENE+'Z2_26SARUNOMORI_room_02Tex_003A60']
masks+=['misc/scene_texture_06/scene_texture_06_Tex_'+s for s in ['000C20','001440']]
hashes={crc64(n) for n in masks};checked=0
for room in range(9):
    _,_,entries=read_mesh_header(native.get(SCENE+f'Z2_26SARUNOMORI_room_{room:02d}'))
    for path in dict.fromkeys(p for _,opa,xlu in entries for p in [opa,xlu] if p):
        if not any(op==32 and h in hashes for off,op,w0,w1,h,size in commands(native.get(path))):continue
        before=native.mesh(path);after=candidate.mesh(path);checked+=1
        assert len(before)==len(after)
        a=np.concatenate([t['v'] for t in before]);b=np.concatenate([t['v'] for t in after])
        assert np.array_equal(a[:,:3],b[:,:3]), 'Foliage position changed'
        assert np.array_equal(a[:,5:],b[:,5:]), 'Foliage native lighting normals or alpha changed'
        assert all(t['lit'] and 'henriko_' in t['texture'] for t in after)
        xml=ET.fromstring(candidate.get(path))
        assert any('G_RM_AA_ZB_TEX_EDGE2' in c.attrib for c in xml), 'Leaf gaps lack texture-edge coverage'
        for tile in [c for c in xml if c.tag=='LoadBlock']:
            assert (int(tile.get('Lrs'))+1)*2<=4096, 'RGBA16 logical tile exceeds TMEM'
assert checked==36, f'Expected four foliage lists per room, checked {checked}'
print('PASS: real Henriko RGBA/alpha, 36 replaced foliage lists, native positions/normals, cutout coverage and TMEM bounds')
