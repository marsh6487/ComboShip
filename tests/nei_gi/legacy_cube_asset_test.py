"""Require the portable fallback to preserve the native placeholder exactly."""
from pathlib import Path
import re
import xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'soh/soh/Enhancements/randomizer/draw.cpp').read_text()
macro=source[source.index('#define DEFINE_GREEN_CUBE_ITEM'):source.index('Gfx gRandoBallandChainDL[]')]
root=ROOT/'soh/assets/custom/objects/nei_legacy_gi/dominion_rod'
assert (root/'gi_dl').exists(), 'native inline Dominion/Elemental fallback still has no portable resource'
vertices=[tuple(int(v.strip(),0) for v in row.split(',')) for row in re.findall(r'VTX\(([^)]+)\)',macro)]
serialized=[tuple(int(v.get(a)) for a in ('X','Y','Z','S','T','R','G','B','A')) for v in ET.parse(root/'vertices').getroot()]
assert serialized==vertices and len(serialized)==24
dl=ET.parse(root/'gi_dl').getroot()
assert [e.tag for e in dl]==['PipeSync','SetCombineLERP','SetPrimColor','ClearGeometryMode','SetGeometryMode','LoadVertices']+['Triangles2']*6+['EndDisplayList']
combine=dl[1]
assert list(combine.attrib.values())==['G_CCMUX_PRIMITIVE','G_CCMUX_0','G_CCMUX_SHADE','G_CCMUX_0','G_ACMUX_PRIMITIVE','G_ACMUX_0','G_ACMUX_SHADE','G_ACMUX_0']*2
assert dl[2].attrib==dict(M='0',L='0',R='0',G='255',B='0',A='255')
assert dl[3].attrib==dict(G_CULL_BACK='1',G_FOG='1',G_LIGHTING='1',G_TEXTURE_GEN='1',G_TEXTURE_GEN_LINEAR='1')
assert dl[4].attrib==dict(G_ZBUFFER='1',G_SHADE='1',G_SHADING_SMOOTH='1')
assert dl[5].attrib==dict(Path='objects/nei_legacy_gi/dominion_rod/vertices',VertexBufferIndex='0',VertexOffset='0',Count='24')
triangles=[tuple(int(v.strip(),0) for v in row.split(',')) for row in re.findall(r'gsSP2Triangles\(([^)]+)\)',macro)]
serialized_triangles=[tuple(int(e.get(a)) for a in ('V00','V01','V02','Flag0','V10','V11','V12','Flag1')) for e in dl.findall('Triangles2')]
assert serialized_triangles==triangles
print('PASS portable Dominion/Elemental native placeholder: exact 24 vertices, six triangle pairs, green material, combine and geometry flags')
