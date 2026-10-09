"""Constrain this visual revision to the approved source diff, including RNG cadence."""
from pathlib import Path
import re, subprocess, sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import functions
BASE='cec63fce86b1f582f6e61ad6a98eca6c3cca784b'
def baseline(p):return subprocess.check_output(['git','show',f'{BASE}:{p}'],cwd=ROOT,text=True)
def tokens(s):return re.sub(r'\s+','',re.sub(r'//[^\n]*|/\*.*?\*/','',s,flags=re.S))
def gameplay(s):
 s=re.sub(r'    set->drawEpoch = \+\+s(?:Fire|Ice)DrawEpoch;\n', '', s)
 s=s.replace('EffectSsEnIce_Spawn(play, &sparkPos, 0.0f,', 'EffectSsEnIce_Spawn(play, &sparkPos, scale * 0.3f,')
 s=s.replace('&primColor, &envColor, 0, 10);', '&primColor, &envColor, 1000, 10);')
 s=re.sub(r'\s*(?:FX_DrawChargeAura|FX_DrawSpinFireCylinder|NeiUsedMagic_DrawCharge|NeiUsedMagic_DrawSpin)\([^;]*;', '',s)
 s=s.replace('RodCommon_PreserveChargeSparkCadence','FX_SpawnRodSwingParticles')
 s=re.sub(r'    // Use bright yellow.*?    if \(\(play->gameplayFrames % 3\)',
          '    if ((play->gameplayFrames % 3)',s,flags=re.S)
 s=re.sub(r'        // Sample the existing wave/beam state;[^\n]*\n        for \(s32 i = 0; i < .*?\n        }\n','',s,flags=re.S)
 return tokens(s)
for element in ('fire','ice','light'):
 path=f'soh/mods/items/logic/item_rod_{element}.c'
 old=functions(baseline(path))
 # Separately tested put-away audio fix is now part of this combined revision.
 # Normalize only the exact per-rod ownership wrappers, retaining gameplay checks.
 current=(ROOT/path).read_text()
 for kind in ('Equip','Unequip'):
  current=current.replace(f'ItemEquip_Play{kind}SFXForAction(play, p, PLAYER_IA_ROD_{element.upper()})',
                          f'ItemEquip_Play{kind}SFX(play, p)')
 new=functions(current)
 assert old.keys()==new.keys(),(element,'unexpected function additions/deletions')
 for name in old:
  assert gameplay(old[name])==gameplay(new[name]),(element,name,'nonvisual logic changed')
# The invisible original particle still occupies the same effect slot for the
# same life, running the real unmodified GSpk update (two RNG draws per tick).
old=functions((ROOT/'soh/mods/items/helpers/fx_helper.c').read_text())['FX_SpawnRodSwingParticles']
new=functions((ROOT/'soh/mods/items/logic/item_rod_common.c').read_text())['RodCommon_PreserveChargeSparkCadence']
old=old.replace('FX_SpawnRodSwingParticles','RodCommon_PreserveChargeSparkCadence').replace('&env, 100, 10','&env, 0, 0')
assert tokens(old)==tokens(new)
for path in ('soh/src/overlays/effects/ovl_Effect_Ss_G_Spk/z_eff_ss_g_spk.c',
             'soh/src/overlays/effects/ovl_Effect_Ss_En_Ice/z_eff_ss_en_ice.c',
             'soh/src/overlays/effects/ovl_Effect_Ss_KiraKira/z_eff_ss_kirakira.c'):
 assert (ROOT/path).read_text()==baseline(path),path
# The native-bank regression in tests/oot_timegate covers this audio-only
# lifetime fix. Permit only its two source-owned stops and one cleanup call in
# each exit handler; the rest of Time Gate must still match the accepted source
# byte for byte, including controls, magic cost, animations and state changes.
path='soh/mods/items/logic/item_time_gate.c'
current=(ROOT/path).read_text()
helper=functions(current)['TimeGate_StopSounds']
assert tokens(helper)==tokens('''static void TimeGate_StopSounds(Player* p) {
    Audio_StopSfxByPosAndId(&p->actor.world.pos, NA_SE_EV_WARP_HOLE);
    Audio_StopSfxByPosAndId(&p->actor.world.pos, NA_SE_PL_MAGIC_WIND_WARP);
}'''), 'Time Gate cleanup must stop only its own two sounds'
current=current.replace(helper+'\n\n','',1)
for name,indent in (('TimeGate_Stop',4),('TimeGate_StateSwitching',4),('TimeGate_StateCancel',8)):
 body=functions(current)[name]
 cleanup=' '*indent+'TimeGate_StopSounds(p);\n\n'
 assert body.count(cleanup)==1,(name,'expected one audio cleanup')
 current=current.replace(body,body.replace(cleanup,'',1),1)
assert current==baseline(path),(path,'changes beyond the tested audio cleanup')
# Accepted Fire dispatch retains the GI17 center-history path. Ice now has
# independently tested per-head reconstructed trails, without gameplay edits.
fire=(ROOT/'soh/mods/items/objects/object_firerod.c').read_text()
ice=subprocess.check_output(['git','show','c77c18587a976f6d6cb5c8f91f27593286469218:soh/mods/items/objects/object_icerod.c'],cwd=ROOT,text=True)
marker='    // Item-local USED meshes.'
expected=ice.split(marker,1)[1].replace('IceRod','FireRod').replace('iceRod','fireRod')
expected=expected.replace('DrawTrail(play, 1,','DrawTrail(play, 0,').replace('DrawProjectile(play, 1,','DrawProjectile(play, 0,')
visual=fire.split(marker,1)[1]
visual=re.sub(r'\s*FrameInterpolation_Record(?:Open|Close)Child\([^;]*;', '', visual)
visual=re.sub(r'\s*Vec3f\s+direction\s*=\s*RodVisual_Heading\([^;]*;', '', visual)
visual=visual.replace('&set->pos[p], &direction,', '&set->pos[p], &set->vel[p],')
assert tokens(visual)==tokens(expected)
for element in ('fire','ice'):
 current=(ROOT/f'soh/mods/items/logic/item_rod_{element}.c').read_text()
 for suffix in ('SingleProjectile','TripleProjectile'):
  init=functions(current)[f'{element.capitalize()}Rod_Init{suffix}']
  assert init.count(f'set->drawEpoch = ++s{element.capitalize()}DrawEpoch;')==1
path='soh/mods/items/objects/object_lightrod.c'
assert (ROOT/path).read_text().count('Rand_ZeroOne()')==baseline(path).count('Rand_ZeroOne()')==1
path='soh/soh/Enhancements/randomizer/NeiGiPresentation.cpp'
old=functions(baseline(path))['NeiGi_DrawMesh']
new=functions((ROOT/'soh/soh/Enhancements/randomizer/NeiGiMeshRenderer.inc').read_text())['NeiGi_DrawMeshMaterial']
new=re.sub(r'static bool NeiGi_DrawMeshMaterial\(.*?\) \{',
           'void NeiGi_DrawMesh(PlayState* play, const NeiGi::Mesh& mesh, Kind orb) {',new,count=1,flags=re.S)
# The renderer input/capacity guards have their own real-arena regressions.
# Normalize only those exact additions; retain the fixed historical batcher
# and untextured material/command behavior as the comparison baseline.
start=new.index('{')+1;end=new.index('    // Reuse shared vertices')
assert tokens(new[start:end])==tokens('''
    if (!play || !play->state.gfxCtx || !NeiGi_ValidMesh(mesh, material, seasonSunRays) || int(orb) < 0 ||
        int(orb) > int(Kind::Gold))
        return false;
''')
new=new[:start]+'\n    if (mesh.count == 0)\n        return;\n'+new[end:]
start=new.index('    size_t triangleCommands = 0;');end=new.index('    auto* vertices',start)
assert tokens(new[start:end])==tokens('''
    size_t triangleCommands = 0;
    for (size_t b = 0; b < batchCount; ++b)
        triangleCommands += (batches[b].indexCount + 5) / 6;
    const bool scrolling = material && material->scrolling;
    const size_t scrollBytes = scrolling ? 2 * sizeof(Gfx) : 0;
    if (!NeiGi_ArenaHasRoom(play, vertexCount * sizeof(Vtx) + scrollBytes, 1, 2,
                           32 + batchCount + triangleCommands + (scrolling ? 1 : 0)))
        return false;
''')
new=new[:start]+new[end:]
# The reward-only scroll fragment is absent when no texture material is
# supplied. Require its exact allocation and commands before normalizing it;
# the complete untextured batcher below still compares with the fixed source.
allocation='auto* vertices = static_cast<Vtx*>(Graph_Alloc(play->state.gfxCtx, vertexCount * sizeof(Vtx) + scrollBytes));'
assert new.count(allocation)==1,'expected exactly the scroll-aware vertex allocation'
new=new.replace(allocation,allocation.replace(' + scrollBytes',''),1)
start=new.index('    Gfx* scroll = nullptr;');end=new.index('    OPEN_DISPS',start)
assert tokens(new[start:end])==tokens('''
    Gfx* scroll = nullptr;
    if (scrolling) {
        scroll = reinterpret_cast<Gfx*>(vertices + vertexCount);
        const auto offset = RewardGi_Scroll(play->gameplayFrames);
        gDPSetTileSize(scroll, G_TX_RENDERTILE, offset.s, offset.t, offset.s + 31 * 4, offset.t + 31 * 4);
        gSPEndDisplayList(scroll + 1);
    }
'''),'unexpected reward-only scroll fragment'
new=new[:start]+new[end:]
for op in ('Push','Pop'):
    new,count=re.subn(r'    if \(owner\)\n        gSPComboRM'+op+r'\(POLY_XLU_DISP\+\+(?:, owner)?\);\n','',new)
    assert count==1,'expected exactly the private-material owner '+op
assert new.count('return false;')==1
new=new.replace('return false;','return;')
new,count=re.subn(r'    return true;\n}$','}',new)
assert count==1,'expected only the renderer success result'
# With both optional texture modes disabled, the retained GI batcher must
# still match the approved untextured renderer. The seasonal ray lane is
# independently exercised by the production weather renderer fixtures.
assert new.count('(material || seasonSunRays ? 32 : 63)')==1
new=new.replace('(material || seasonSunRays ? 32 : 63)','63')
new,count=re.subn(r'\(seasonSunRays \? 31\s*: material\s*\? 32\s*: 63\)', '63',new)
assert count==1,'expected the one seasonal/material V texture coordinate'
a=new.index('    if (seasonSunRays) {');b=new.index('    } else if (orb != Kind::Neutral) {',a)
new=new[:a]+'    if (orb != Kind::Neutral) {'+new[b+len('    } else if (orb != Kind::Neutral) {'):]
# Accepted POC6 permits an owner/editor palette at this one color selection.
# Normalize that exact addition only; retain the complete historical batcher.
palette='palette ? NeiGi::OrbColors{palette->hot, palette->edge} : NeiGi::OrbPalette(orb)'
assert new.count(palette)==1,'expected the one accepted elemental palette selection'
new=new.replace(palette,'NeiGi::OrbPalette(orb)',1)
assert tokens(old)==tokens(new),'Existing GI batcher command path changed'
print('USED VFX source contract: rod gameplay, Time Gate state, local/remote shot dispatch, flight particle size alone suppressed; impact particles, gameplay and RNG cadence preserved')

assert 'gEffFire1DL' not in fire and 'objects/gameplay_keep' not in fire
print('PASS Fire projectile draw is independent of shared gameplay_keep fire material')
