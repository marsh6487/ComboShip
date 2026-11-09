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
 spawn_source=subprocess.check_output(['git','show','0c2e93bb:mm/2s2h/CustomItem/CustomItem.cpp'],cwd=ROOT,text=True) if red_spawn else (ROOT/'mm/2s2h/CustomItem/CustomItem.cpp').read_text()
 (build/'environment_spawn.inc').write_text(function(spawn_source,'CustomItem::Spawn'))
 src=(ROOT/'mm/src/code/z_actor.c').read_text();a=src.index('    SetActorListIndex(actor, currentActorListIndex);');b=src.index('    // #endregion',a)
 (build/'environment_actor_index.inc').write_text(src[a:b])
 src=(ROOT/'mm/2s2h/BenJsonConversions.hpp').read_text();a=src.index('inline void to_json(json& j, const RandoSaveCheck&');b=src.index('inline void to_json(json& j, const Vec3f&',a)
 (build/'environment_json.inc').write_text('using json=nlohmann::json;\n'+src[a:b])
 inventory=(ROOT/'mm/src/code/z_inventory.c').read_text()
 (build/'environment_slots.inc').write_text('extern "C" {\n'+re.search(r'^u8 gItemSlots\[77\] = \{.*?^\};',inventory,re.M|re.S)[0]+'\n'+'\n'.join(re.search(r'^(?:u32|u8) '+name+r'\[\] = \{.*?^\};',inventory,re.M|re.S)[0] for name in ('gUpgradeMasks','gUpgradeShifts'))+'\n}')
 regions=(ROOT/'mm/2s2h/Rando/Logic/Regions/SnowheadTemple.cpp').read_text()
 (build/'environment_rules.inc').write_text('\n'.join(line for line in regions.splitlines() if 'CHECK(RC_SNOWHEAD_TEMPLE_ICICLE_ENTRANCE' in line))
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
