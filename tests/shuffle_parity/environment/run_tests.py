#!/usr/bin/env python3
"""Real native types/catalogues/pools/callbacks and CustomItem::Spawn; engine allocation is the boundary."""
from pathlib import Path
import os,re,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[3]
red_spawn='--red-spawn' in sys.argv
if red_spawn: sys.argv.remove('--red-spawn')
assert (ROOT/'mm/2s2h/Rando/MMEnvironmentIcicles.inc').exists(), 'Missing source-audited native MM icicle catalogue'
sys.path.insert(0,str(ROOT/'tests/shuffle_parity/boss_keys'))
from run_tests import function
with tempfile.TemporaryDirectory(prefix='mm-environment-') as td:
 build=Path(td)
 src=(ROOT/'mm/2s2h/Rando/ActorBehavior/BgIcicle.cpp').read_text()
 (build/'environment_callbacks.inc').write_text(src[src.index('namespace {'):])
 native=(ROOT/'mm/src/overlays/actors/ovl_Bg_Icicle/z_bg_icicle.c').read_text()
 (build/'environment_native_break.inc').write_text(re.sub(r'\bthis\b','self','\n'.join(function(native,n) for n in ('BgIcicle_Break','BgIcicle_UpdateAttacked','BgIcicle_Fall'))))
 # Actual source transition bodies with explicit engine/effect service boundaries.
 ishi=(ROOT/'mm/src/overlays/actors/ovl_En_Ishi/z_en_ishi.c').read_text()
 soil=(ROOT/'mm/src/overlays/actors/ovl_Obj_Swprize/z_obj_swprize.c').read_text()
 bomb=(ROOT/'mm/src/overlays/actors/ovl_Obj_Bombiwa/z_obj_bombiwa.c').read_text()
 red=(ROOT/'mm/src/overlays/actors/ovl_Obj_Hamishi/z_obj_hamishi.c').read_text()
 bean=(ROOT/'mm/src/overlays/actors/ovl_Obj_Bean/z_obj_bean.c').read_text()
 tables='\n'.join(re.search(r'^s16 '+n+r'\[\] = \{.*?\};',soil,re.M|re.S)[0] for n in ('D_80C257F0','D_80C257F8'))
 tables+='\n'+'\n'.join(re.search(r'^static ColliderCylinderInit '+n+r' = \{.*?^\};',bomb,re.M|re.S)[0] for n in ('sCylinderInit1','sCylinderInit2'))
 tables+='\n'+re.search(r'^typedef struct \{.*?\} ObjBombiwaStruct2;',bomb,re.M|re.S)[0]
 tables+='\n'+re.search(r'^static ObjBombiwaStruct2 D_8093A998\[\] = \{.*?^\};',bomb,re.M|re.S)[0]
 bodies='\n'.join([function(ishi,'EnIshi_DropItem'),function(soil,'func_80C253D0'),function(bomb,'func_809393B0'),function(bomb,'func_80939470'),function(bomb,'func_80939EF4'),function(red,'ObjHamishi_Update'),function(bean,'func_80938298'),function(soil,'ObjSwprize_DoNothing'),function(soil,'func_80C25640')])
 (build/'environment_native_sources.inc').write_text(re.sub(r'\bthis\b','self',tables+'\n'+bodies))
 spawn_source=subprocess.check_output(['git','show','0c2e93bb:mm/2s2h/CustomItem/CustomItem.cpp'],cwd=ROOT,text=True) if red_spawn else (ROOT/'mm/2s2h/CustomItem/CustomItem.cpp').read_text()
 (build/'environment_spawn.inc').write_text(function(spawn_source,'CustomItem::Spawn'))
 src=(ROOT/'mm/src/code/z_actor.c').read_text();a=src.index('    SetActorListIndex(actor, currentActorListIndex);');b=src.index('    // #endregion',a)
 (build/'environment_actor_index.inc').write_text(src[a:b])
 src=(ROOT/'mm/2s2h/BenJsonConversions.hpp').read_text();a=src.index('inline void to_json(json& j, const RandoSaveCheck&');b=src.index('inline void to_json(json& j, const Vec3f&',a)
 (build/'environment_json.inc').write_text('using json=nlohmann::json;\n'+src[a:b])
 inventory=(ROOT/'mm/src/code/z_inventory.c').read_text()
 (build/'environment_slots.inc').write_text('extern "C" {\n'+re.search(r'^u8 gItemSlots\[77\] = \{.*?^\};',inventory,re.M|re.S)[0]+'\n'+'\n'.join(re.search(r'^(?:u32|u8) '+name+r'\[\] = \{.*?^\};',inventory,re.M|re.S)[0] for name in ('gBitFlags','gUpgradeMasks','gUpgradeShifts'))+'\n}')
 (build/'environment_slots.inc').write_text((build/'environment_slots.inc').read_text().replace('1 <<','1u <<'))
 regions=(ROOT/'mm/2s2h/Rando/Logic/Regions/SnowheadTemple.cpp').read_text()
 (build/'environment_rules.inc').write_text('\n'.join(line for line in regions.splitlines() if 'CHECK(RC_SNOWHEAD_TEMPLE_ICICLE_ENTRANCE' in line))
 import json
 ids={c['id'] for c in json.loads((ROOT/'tests/shuffle_parity/environment/donor_manifest.json').read_text())['checks']}
 rules=[line for p in sorted((ROOT/'mm/2s2h/Rando/Logic/Regions').glob('*.cpp')) for line in p.read_text().splitlines() if any('CHECK('+check+',' in line for check in ids)]
 assert len(rules)==211
 (build/'environment_all_rules.inc').write_text('\n'.join(rules))

 tracker=(ROOT/'mm/2s2h/Rando/CheckTracker/CheckTracker.cpp').read_text()
 icon_table=tracker[tracker.index('std::vector<const char*> checkTypeIconList'):tracker.index('static constexpr ImVec4 tintColor')]
 (build/'environment_category_indices.inc').write_text(','.join(re.findall(r'/\*(RCTYPE_\w+)\*/',icon_table)))
 env={**os.environ,'CPATH':str(build)+os.pathsep+os.environ.get('CPATH','')}
 if red_spawn: env['MM_ENV_RED_SPAWN']='1'
 result=subprocess.run([sys.executable,str(ROOT/'tests/shuffle_parity/key_rings/run_tests.py'),'--fixture',str(Path(__file__).with_name('native_test.cpp')),*sys.argv[1:]],env=env,capture_output=red_spawn,text=red_spawn)
 if red_spawn:
  assert result.returncode != 0 and 'SIGSEGV' in result.stderr, 'Historical native null-allocation regression must fail specifically with SIGSEGV: '+result.stderr
  print('EXPECTED RED: unchanged baseline 0c2e93bb CustomItem::Spawn fails actual null allocation fixture',flush=True)
 else:
  result.check_returncode()
