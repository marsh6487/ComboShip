"""Verify independent Ikana roots and the archive-bound native dependency graph."""
from pathlib import Path
import xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[2]
constants=(ROOT/'combo/menu/ComboIkanaShieldResources.h').read_text()
assert 'objects/ikana_shield_gi/gIkanaShieldGiOpaDL' in constants, 'Ikana GI still shares the Mirror Shield mod root'
assert 'objects/ikana_shield_gi/gIkanaShieldGiXluDL' in constants
for name,native in [('gIkanaShieldGiOpaDL','gGiMirrorShieldEmptyDL'),('gIkanaShieldGiXluDL','gGiMirrorShieldDL')]:
    path=ROOT/'mm/assets/custom/objects/ikana_shield_gi'/name
    root=ET.parse(path).getroot()
    calls=root.findall('CallDisplayList')
    assert len(calls)==1
    assert calls[0].get('Path')=='__OTR__@mm-ikana-base:objects/object_gi_shield_3/'+native
    assert root[-1].tag=='EndDisplayList'
print('PASS independent Ikana named GI roots route their full native graph to the stock-only MM owner')

shutdown=(ROOT/"mm/2s2h/BenPort.cpp").read_text()
assert 'Ship::CrossRMRegistry::Unregister("mm-ikana-base");' in shutdown, "stock owner must release workers on normal MM shutdown"
