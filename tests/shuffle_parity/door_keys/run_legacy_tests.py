#!/usr/bin/env python3
"""Execute production versioned key branch and native serialization callbacks.

Only metadata/context allocation and byte/JSON storage are service boundaries;
key ownership, pool policy, parsers, array walkers and native door/oracle branch
are production source. This is bounded native evidence, not full-world replay.
"""
from pathlib import Path
import argparse,os,re,subprocess,tempfile,sys
ROOT=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--sanitize',action='store_true');args=p.parse_args()
sys.path.insert(0,str(ROOT/'tests/shuffle_parity/boss_keys'))

def block(s,signature):
 a=s.index(signature);b=s.index('{',a)+1;depth=1;i=b
 while depth:depth+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[a:i]
def function(s,name):
 m=re.search(r'^[^()\n;{}]*\b'+re.escape(name)+r'\([^;{}]*\)\s*(?:const\s*)?\{',s,re.M)
 if not m:raise ValueError('Missing production function '+name)
 return block(s[m.start():],s[m.start():s.index('{',m.start())].strip())
def statement(s,signature):
 a=s.index(signature);i=s.index('(',a)+1;depth=1
 while depth:depth+=(s[i]=='(')-(s[i]==')');i+=1
 assert s[i]==';'
 return s[a:i+1]
with tempfile.TemporaryDirectory(prefix='rusty-legacy-') as td:
 d=Path(td);settings=(ROOT/'soh/soh/Enhancements/randomizer/settings.cpp').read_text();option=(ROOT/'soh/soh/Enhancements/randomizer/option.cpp').read_text();otr=(ROOT/'soh/soh/OTRGlobals.cpp').read_text();pool=(ROOT/'soh/soh/Enhancements/randomizer/3drando/item_pool.cpp').read_text();logic=(ROOT/'soh/soh/Enhancements/randomizer/logic.cpp').read_text();save=(ROOT/'soh/soh/SaveManager.cpp').read_text();spoiler=(ROOT/'soh/soh/Enhancements/randomizer/3drando/spoiler_log.cpp').read_text();play=(ROOT/'soh/soh/Enhancements/randomizer/3drando/playthrough.cpp').read_text()
 (d/'legacy_options.inc').write_text('namespace Rando {\n'+'\n'.join((block(option,'OptionValue::operator bool() const') if n=='OptionValue::operator bool' else function(option,n)) for n in ('OptionValue::Get','OptionValue::Set','OptionValue::operator bool','Option::GetCVarName','Option::GetOptionIndex','Option::GetValueFromText'))+'\n'+function(settings,'Settings::GetInstance')+'\n'+function(settings,'Settings::GetAllOptions')+'\n'+function(settings,'Settings::ParseJson')+'\n}')
 (d/'legacy_snapshot.inc').write_text('\n'.join(function(otr,n) for n in ('Combo_ClearNumericExclusions','SOH_DumpRandoSettings','SOH_RestoreRandoSettings')))
 (d/'legacy_spoiler.inc').write_text(function(spoiler,'WriteSettings'))
 (d/'legacy_save_arrays.inc').write_text('\n'.join(function(save,n) for n in ('SaveManager::SaveArray','SaveManager::LoadArray')))
 (d/'legacy_save_callbacks.inc').write_text(statement(save,'SaveManager::Instance->SaveArray("randoSettings"')+'\n'+statement(save,'SaveManager::Instance->SaveArray("randomizerInf"'))
 (d/'legacy_load_callbacks.inc').write_text(statement(save,'SaveManager::Instance->LoadArray("randoSettings"')+'\n'+statement(save,'SaveManager::Instance->LoadArray("randomizerInf"'))
 (d/'legacy_add_pool.inc').write_text(function(pool,'AddItemToPool'))
 (d/'legacy_pool.inc').write_text(block(pool[pool.index('    // Keys'):],'if (ctx->GetOption(RSK_LOCK_OVERWORLD_DOORS))'))
 (d/'legacy_oracle.inc').write_text(block(logic,'if (RustyDoorKeys::Find(itemName))'))
 (d/'legacy_hash.inc').write_text(block(play,'if (ctx->GetOption(RSK_LOCK_OVERWORLD_DOORS) &&'))
 # Real Entrance declaration/methods and shuffle connection routine; only Region allocation is a service seam.
 entrance=(ROOT/'soh/soh/Enhancements/randomizer/entrance.cpp').read_text()
 header=(ROOT/'soh/soh/Enhancements/randomizer/entrance.h').read_text()
 market=(ROOT/'soh/soh/Enhancements/randomizer/location_access/overworld/market.cpp').read_text()
 decl=block(header,'enum class EntranceType')+';\n'+block(header,'class Entrance {')+';'
 (d/'legacy_entrance_decl.inc').write_text('namespace Rando {\n'+decl+'\n}')
 methods=('Entrance::GetConditionsMet','Entrance::GetConnectedRegionKey','Entrance::GetIndex','Entrance::GetReverse','Entrance::IsDecoupled','Entrance::SetDecoupled','Entrance::GetAssumed','Entrance::GetReplacement','Entrance::SetReplacement','Entrance::Connect','Entrance::Disconnect','Entrance::BindTwoWay','Entrance::GetConnectedRegion','Entrance::SetName','Entrance::GetNewTarget','Entrance::AssumeReachable')
 (d/'legacy_entrance_methods.inc').write_text('namespace Rando {\n'+block(entrance,'Entrance::Entrance(RandomizerRegion')+'\n'+'\n'.join(function(entrance,n) for n in methods)+'\n'+function(entrance,'ChangeConnections')+'\n}')
 # Extract exact ENTRANCE expressions inside the specific native source region.
 def edge(region,target):
  a=market.index('areaTable['+region+']');b=market.find('areaTable[',a+1);regiontext=market[a:b if b>=0 else len(market)]
  m=re.search(r'ENTRANCE\('+target+r'\s*,',regiontext);assert m,(region,target)
  a=m.start();i=m.end();depth=1
  while depth:depth+=(regiontext[i]=='(')-(regiontext[i]==')');i+=1
  return regiontext[a:i]
 (d/'legacy_entrance_edges.inc').write_text('auto guardEntry='+edge('RR_MARKET_ENTRANCE','RR_MARKET_GUARD_HOUSE')+';\n'+'auto guardReturn='+edge('RR_MARKET_GUARD_HOUSE','RR_MARKET_ENTRANCE')+';\n'+'auto richardEntry='+edge('RR_MARKET_BACK_ALLEY','RR_MARKET_DOG_LADY_HOUSE')+';\n'+'auto richardReturn='+edge('RR_MARKET_DOG_LADY_HOUSE','RR_MARKET_BACK_ALLEY')+';\n')
 # Compile actual native Castle collision gate and guard choice as C, using target resource flags.
 gate=(ROOT/'soh/src/overlays/actors/ovl_Bg_Spot15_Saku/z_bg_spot15_saku.c').read_text()
 guard=(ROOT/'soh/src/overlays/actors/ovl_En_Heishi2/z_en_heishi2.c').read_text()
 actors=(ROOT/'soh/src/code/z_actor.c').read_text();parameter=(ROOT/'soh/src/code/z_parameter.c').read_text()
 inventory=(ROOT/'soh/src/code/z_inventory.c').read_text()
 tables='\n'.join(re.search(r'^(?:u32|u8|u16) '+name+r'\[\](?:\[4\])? = \{.*?\};',inventory,re.M|re.S)[0] for name in ('gUpgradeMasks','gUpgradeShifts','gUpgradeCapacities'))
 (d/'legacy_castle_native.c').write_text(r'''#include "global.h"
#include "overlays/actors/ovl_Bg_Spot15_Saku/z_bg_spot15_saku.h"
#include "overlays/actors/ovl_En_Heishi2/z_en_heishi2.h"
#include "objects/object_spot15_obj/object_spot15_obj.h"
#include <stdlib.h>
extern int Randomizer_RustyCastleGateEnabled(PlayState*);
extern int Randomizer_RustyCastleGateLocked(PlayState*);
void func_808B4930(BgSpot15Saku*,PlayState*);void func_808B4978(BgSpot15Saku*,PlayState*);void func_808B4A04(BgSpot15Saku*,PlayState*);
// Opaque native guard continuations are intentionally outside this choice test;
// trap if executed, rather than replacing any tested behavior with a no-op.
void func_80A531E4(EnHeishi2* a,PlayState* p){abort();}
void func_80A54954(EnHeishi2* a,PlayState* p){abort();}
void func_80A53538(EnHeishi2* a,PlayState* p){abort();}
PlayState* gPlayState;
int castleSoundCount,castleMessageClosed,castleMessageContinued;
void DynaPolyActor_Init(DynaPolyActor* a,s32 flags){}
s32 DynaPoly_SetBgActor(PlayState* p,DynaCollisionContext* d,Actor* a,CollisionHeader* h){return 7;}
void CollisionHeader_GetVirtual(void* source,CollisionHeader** dest){*dest=NULL;}
void Audio_PlayActorSound2(Actor* a,u16 sfx){++castleSoundCount;}
s32 SkelAnime_Update(SkelAnime* a){return 0;}
u8 Message_GetState(MessageContext* m){return TEXT_STATE_CHOICE;}
u8 Message_ShouldAdvance(PlayState* p){return 1;}
void Message_CloseTextbox(PlayState* p){++castleMessageClosed;}
void Message_ContinueTextbox(PlayState* p,u16 id){++castleMessageContinued;}
'''+tables+'\n'+'\n'.join(function(gate,n) for n in ('BgSpot15Saku_Init','func_808B4930','func_808B4978','func_808B4A04','BgSpot15Saku_Update'))+'\n'+function(actors,'Flags_GetInfTable')+'\n'+function(parameter,'Rupees_ChangeBy')+'\n'+function(guard,'func_80A5344C'))
 selector=function(guard,'func_80A53278');branch=block(selector,'if (this->type == 2 && Randomizer_RustyCastleGateEnabled(play))')
 assert '0x0203' in branch and '0x7099' in branch and 'TEXT_STATE_DONE' in branch and 'return;' in branch
 assert selector.index(branch)<selector.index('VB_CAN_BRIBE_HEISHI2')
 choice=function(guard,'func_80A5344C');branch=block(choice,'if (this->type == 2 && Randomizer_RustyCastleGateEnabled(play))')
 assert 'return;' in branch and 'Rupees_ChangeBy' not in branch and 'Flags_' not in branch
 assert choice.index(branch)<choice.index('Rupees_ChangeBy')
 assert re.search(r'case RSK_RUSTY_DOOR_KEY_VERSION:.*?continue;',function(settings,'Settings::RandomizeAllSettings'),re.S)
 market=(ROOT/'soh/soh/Enhancements/randomizer/location_access/overworld/castle_grounds.cpp').read_text()
 (d/'legacy_castle_edges.inc').write_text('auto gateEdge='+edge('RR_HC_GATE','RR_HC_PAST_GATE')+';\n'+'auto vineEdge='+edge('RR_HC_GATE','RR_HC_ABOVE_VINE')+';\n'+'auto backEdge='+edge('RR_HC_PAST_GATE','RR_HC_GATE')+';\n')
 copied=function(settings,'Settings::SetAllToContext');start=copied.index('    for (int i = 0; i < RSK_MAX; i++)');(d/'legacy_copy_options.inc').write_text(block(copied[start:],'for (int i = 0; i < RSK_MAX; i++)'))
 # Generic load/replay/reset boundaries must never turn legacy snapshots into new generation.
 for name in ('SOH_PrepRandoContext','SOH_RestoreRandoSettings'):
  body=function(otr,name);assert 'PrepareNewGeneration(' not in body and 'StampNewGeneration(' not in body,name
 assert 'PrepareNewGeneration(' not in copied and 'StampNewGeneration(' not in copied
 assert 'RustyDoorKeys::ResetImportedCatalog(*mContext)' in function(settings,'Settings::ParseJson')
 # Verify the fixture metadata boundary agrees with the actual registered saved setting.
 registered=statement(settings,'OPT_U8(RSK_RUSTY_DOOR_KEY_VERSION')
 assert '"Rusty Door Key Catalog"' in registered and '{"Legacy24", "Rusty27"}' in registered
 assert 'CVAR_RANDOMIZER_SETTING("RustyDoorKeyCatalogVersion")' in registered
 assert re.search(r'WIDGET_CVAR_COMBOBOX,\s*0,\s*true\s*\)',registered)
 for mode in ('original','combo'):
  exe=d/mode;flags=['-std=c++20','-O0','-fmax-errors=5','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-DCONTROLLERBUTTONS_T=uint32_t','-DCVAR_PREFIX_RANDOMIZER_SETTING="gRandoSettings"','-I'+str(d),'-I'+str(ROOT)]
  if mode=='combo':flags+=['-DCOMBO_BUILD']
  if args.sanitize:flags+=['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
  for p in ('soh','soh/include','soh/src','soh/assets','libultraship/include','combo'):flags+=['-isystem',str(ROOT/p)]
  native=d/(mode+'-castle.o')
  cflags=[f for f in flags if f not in ('-std=c++20',)]
  subprocess.run([os.environ.get('CC','cc'),'-std=gnu11','-Wno-int-conversion','-Wno-incompatible-pointer-types',*cflags,'-c',str(d/'legacy_castle_native.c'),'-o',str(native)],check=True)
  subprocess.run([os.environ.get('CXX','c++'),*flags,str(Path(__file__).with_name('legacy_test.cpp')),str(native),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
