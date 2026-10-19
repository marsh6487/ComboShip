"""The supplied floor artwork must own both texture stages of the native floor."""
from pathlib import Path
import sys

root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools/woods_mystery'))
from scene_assets import Resources,commands,crc64

candidate=Resources(sys.argv[1])
base='misc/scene_texture_06/scene_texture_06_Tex_002460'
detail='misc/scene_texture_06/scene_texture_06_Tex_004460'
grass='custom/cor_woods_mystery/poc1/textures/grass'
floors=0
for name,data in candidate.files.items():
    if '/Z2_26SARUNOMORI_room_' not in name or 'DL_' not in name or data.startswith(b'<'):continue
    loads=[h for off,op,w0,w1,h,size in commands(data) if op==32]
    if crc64(grass) not in loads:continue
    floors+=1
    assert crc64(base) not in loads and crc64(detail) not in loads, f'Native floor artwork still wins a texture stage in {name}'
    assert loads.count(crc64(grass))==2, f'Both native floor stages must use the supplied grass in {name}'
assert floors==9, f'Expected nine floor lists, found {floors}'
print('PASS: supplied grass resolves on both native texture stages in every room')
