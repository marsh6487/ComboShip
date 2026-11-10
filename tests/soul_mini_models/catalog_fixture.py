"""Compile owner recipes unchanged into the shared native-traversal fixture.

Only engine resources are synthetic. Identity selection, recipe construction,
descriptor validation, owner routing and both host limb drawers are production code.
Private archive validation independently verifies that the real rigs match these
descriptors; no private geometry or animation bytes are needed by CI.
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]


def write_catalog_fixture(build):
    bridge = build / 'libultraship/bridge/consolevariablebridge.h'
    bridge.parent.mkdir(parents=True, exist_ok=True)
    bridge.write_text('#pragma once\n')
    mm = (ROOT / 'combo/menu/ComboItemDrawMM.h').read_text()
    types = (ROOT / 'mm/2s2h/Rando/Types.h').read_text()
    enemies = list(dict.fromkeys(re.findall(r'\bRI_SOUL_ENEMY_\w+\b', types)))
    catalog = (ROOT / 'combo/rando/SoulShuffleCatalog.inc').read_text()
    npcs = ['RI_SOUL_NPC_' + name for name in re.findall(r'SOUL\(MM, NPC, (\w+),', catalog)]
    assert len(enemies) == 47 and len(npcs) == 53
    result = ['#include "' + str(ROOT / 'combo/menu/ComboSoulMiniOOT.h') + '"',
              'namespace SoulCatalogMm {',
              'enum RandoItemId {' + ','.join(enemies + npcs) + ', RI_UNKNOWN};']
    includes = re.findall(r'#include "((?:assets/)?objects/[^\"]+)"', mm)
    for inc in dict.fromkeys(includes + ['objects/object_bigslime/object_bigslime.h']):
        asset = ROOT / 'mm' / inc if inc.startswith('assets/') else ROOT / 'mm/assets' / inc
        result.append('#include "' + str(asset) + '"')
    # The legacy boss fixture already has three identically named helpers.
    # Rename this copy only to avoid C++ ADL finding both names for ABI POD args.
    for helper in ('MM_AnimSeg', 'MM_AnimTexSeg', 'MM_AnimSoulFlame'):
        result.append('#define ' + helper + ' Catalog_' + helper)
    start, end = mm.index('struct MmSoulModel {'), mm.index('// Native MM boss models whose draw routines fit')
    result.append(mm[start:end])
    start = mm.index('static int32_t MM_FillEnemySoulAnim(')
    result.append(mm[start:mm.index('// DrawMinifrog:', start)])
    result.append('static const RandoItemId EnemyIds[] = {' + ','.join(enemies) + '};')
    result.append('static const char* EnemyNames[] = {' + ','.join('"' + n + '"' for n in enemies) + '};')
    result.append('static const RandoItemId NpcIds[] = {' + ','.join(npcs) + '};')
    result.append('static const char* NpcNames[] = {' + ','.join('"' + n + '"' for n in npcs) + '};')
    for helper in ('MM_AnimSeg', 'MM_AnimTexSeg', 'MM_AnimSoulFlame'):
        result.append('#undef ' + helper)
    result.append('} // namespace SoulCatalogMm')
    (build / 'catalog_production.inc').write_text('\n'.join(result) + '\n')
