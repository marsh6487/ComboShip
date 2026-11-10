#!/usr/bin/env python3
"""Independent MM soul identity/asset/selection contract; no proprietary assets required."""
import json, pathlib, re, subprocess, sys, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[2]
expected = json.loads((pathlib.Path(__file__).parent / 'expected_npcs.json').read_text())
assert len(expected) == 53
native = set(re.findall(r'SOUL\(MM, NPC, (\w+),', (ROOT/'combo/rando/SoulShuffleCatalog.inc').read_text()))
assert {r['key'] for r in expected} == native
if '--baseline-enemies' in sys.argv:
    baseline = subprocess.check_output(['git','show','0063920e18145299afc1682aab98407ebd366fc5:combo/menu/ComboItemDrawMM.h'],cwd=ROOT,text=True)
    table = baseline.split('static const MmSoulModel kMmSoulModels[] = {',1)[1].split('};',1)[0]
    existing = set(re.findall(r'RI_SOUL_ENEMY_([A-Z_]+)',table))
    required = set(re.findall(r'RI_SOUL_ENEMY_([A-Z_]+)',(ROOT/'mm/2s2h/Rando/Types.h').read_text()))
    assert existing == required, 'baseline detailed enemies missing: '+', '.join(sorted(required-existing))
if '--baseline-npcs' in sys.argv:
    baseline = subprocess.check_output(['git','show','0063920e18145299afc1682aab98407ebd366fc5:combo/menu/ComboItemDrawMM.h'],cwd=ROOT,text=True)
    assert all('RI_SOUL_NPC_'+r['key'] in baseline for r in expected), 'baseline missing all 53 NPC miniature recipes'
header = ROOT/'combo/menu/ComboSoulMiniMM.h'
assert header.exists(), 'missing detailed MM NPC / special-enemy recipe catalog'
text = header.read_text()
for r in expected:
    assert 'RI_SOUL_NPC_'+r['key'] in text, r['key']
    assert r['skel'] in text, r['key']+' representative skeleton'
    assert r['limbCount'] > 0
    asset = ROOT/'mm/assets'/r['header']
    assert r['skel'] in asset.read_text(), r['key']+' owner resource'
for key in ('BAD_BAT','BOE','CHUCHU','FREEZARD','LIKE_LIKE','DEXIHAND','GOMESS','IRON_KNUCKLE'):
    assert 'RI_SOUL_ENEMY_'+key in text, key+' detailed composite'
assert 'MM_FillNpcSoulAnim' in (ROOT/'combo/menu/ComboItemDrawMM.h').read_text()
assert 'ComboDrawNativeMmSoul' in (ROOT/'mm/2s2h/Rando/DrawItem.cpp').read_text()
for resource in ('gSwampGuideHeadDL', 'gTownShootingGalleryManHeadDL', 'gMamamuYanHeadDL', 'object_boj_DL_00B1D8'):
    assert resource in text, 'native identity-specific head '+resource
print('PASS: independent 53 NPC identities and eight special enemy paths')
