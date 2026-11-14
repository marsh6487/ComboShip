#!/usr/bin/env python3
"""Actual-source wallet receipts in isolated native-host components; no graph substitute."""
import argparse, hashlib, itertools, json, os, re, subprocess, tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
def read(p):return (ROOT/p).read_text()
def block(text, marker):
 a=text.index(marker);b=text.index('{',a)
 while ';' in text[a:b]:a=text.index(marker,a+len(marker));b=text.index('{',a)
 n=b+1;depth=1
 while depth:
  depth+=(text[n]=='{')-(text[n]=='}');n+=1
 return text[a:n]
def cases(text,first,last):return text[text.index(first):text.index(last,text.index(first))]
def flags(game, build, c=False):
 args=['-std=gnu17' if c else '-std=c++20','-D'+('SOH_BUILD_DLL' if game=='soh' else 'MM_BUILD_DLL'),'-DF3DEX_GBI_2','-DCOMBO_BUILD','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-DCONTROLLERBUTTONS_T=uint32_t','-I'+str(ROOT),'-I'+str(build),'-ffunction-sections','-fdata-sections']
 for p in ((game+'/include',game+'/include/PR',game,game+'/src',game+'/assets',game+('/soh' if game=='soh' else '/2s2h'),'libultraship/include','combo','combo/menu')):args+=['-isystem',str(ROOT/p)]
 if not c:
  # Reuse CI's real pinned header dependencies; never manufacture host headers.
  dependencies=[p for p in os.environ.get('COMBO_NEI_FIXTURE_CPATH','').split(os.pathsep) if p]
  if os.environ.get('IMGUI_DIR'):dependencies.insert(0,os.environ['IMGUI_DIR'])
  for p in ('/usr/include/SDL2',):
   if Path(p).is_dir():dependencies.append(p)
  if not os.environ.get('IMGUI_DIR') and Path('/usr/include/imgui').is_dir():dependencies.append('/usr/include/imgui')
  for p in dependencies:args+=['-isystem',p]
  args+=['-Wl,--gc-sections']
  if game=='mm':args+=['-include',str(ROOT/'tests/shuffle_parity/souls/headless.h')]
 if ARGS.sanitize:args+=['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie']+([] if c else ['-no-pie'])
 return args
COMMON=r'''
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include "combo/ComboGameMode.h"
#include "combo/ComboExport.h"
#include "combo/vanilla/ComboVanillaMode.h"
#include "combo/menu/ItemGrantAuditBridge.h"
using json=nlohmann::json;
#define SPDLOG_INFO(...) ((void)0)
#define SPDLOG_WARN(...) ((void)0)
#define SPDLOG_ERROR(...) ((void)0)
#define FS_TRACE(...) ((void)0)
extern "C" {int gComboGameMode=COMBO_GAME_BOTH;int gComboVanillaMode=0;void ItemGrantAudit_Begin(const char*,int,int,int){} void ItemGrantAudit_End(){} void lusprintf(const char*,int32_t,int32_t,const char*,...) {}}
static unsigned persisted=0, forwards=0;
static bool foreground=true;
static void SendPeer(const char* name) {
 ++forwards;std::cout<<json({{"peer",name}}).dump()<<std::endl;
 std::string ack;assert(std::getline(std::cin,ack));assert(json::parse(ack).at("ack").get<bool>());
}
static void* Combo_ResolveSym(const char*,const char* name){assert(std::string(name).find("_GrantSharedItem")!=std::string::npos);return reinterpret_cast<void*>(SendPeer);}
static void ShareNativeMagicFloor(int){assert(false);}
'''
def native_c(game):
 inv=read(game+'/src/code/z_inventory.c');actor=read(game+'/src/code/z_actor.c')
 names=('gUpgradeMasks','gUpgradeNegMasks','gUpgradeShifts','gUpgradeCapacities','gItemSlots')
 tables='\n'.join(re.search(r'^(?:u32|u16|u8) '+n+r'\[[^]]*\](?:\[4\])? = \{.*?\};',inv,re.M|re.S)[0] for n in names)
 fs=('Flags_GetRandomizerInf','Flags_SetRandomizerInf') if game=='soh' else ('Flags_GetRandoInf','Flags_SetRandoInf','Flags_ClearRandoInf')
 hooks='soh/Enhancements/game-interactor/GameInteractor_Hooks.h' if game=='soh' else '2s2h/GameInteractor/GameInteractor.h'
 return '#include "global.h"\n#include "variables.h"\n#include "functions.h"\n#include "'+hooks+'"\n'+tables+'\n'+block(inv,'void Inventory_ChangeUpgrade(')+'\n'+'\n'.join(block(actor,('s32 ' if 'Get' in n else 'void ')+n+'(') for n in fs)+'\n'
def glue(game):
 text=read(game+('/soh' if game=='soh' else '/2s2h')+'/FleetShipCombo/FleetComboItemsGlue.cpp')
 return text[text.index('namespace {'):text.index('extern "C" void FcCombo_ValidateTable')]
def transport(game):
 p=game+('/soh' if game=='soh' else '/2s2h')+'/FleetShipCombo/FleetSharedItems.cpp';text=read(p)
 s='static int sReceiveDepth=0;static bool sPullPending=false;\n'
 for name in ('FleetShared_RequestPullFromPeer','FleetShared_BeginReceive','FleetShared_EndReceive','FleetShared_IsReceiving'):
  s+=block(text,'extern "C" '+('int ' if name.endswith('IsReceiving') else 'void ')+name+'(')+'\n'
 if game=='mm':s+=text[text.index('static const struct {'):text.index('typedef void (*FnGrantSharedItem)')]
 else:
  x=read('soh/soh/FleetShipCombo/FleetComboRando.cpp')
  # Preserve the complete authored concrete->chain alias catalogue.
  a=x.rfind('const struct {',0,x.index('{ RG_ADULT_WALLET'))
  s+=x[a:x.index('int FleetCombo_ChainForItem(',a)]
 s+=block(text,'static FnGrantSharedItem ResolvePeerGrant()')+'\n'+block(text,'extern "C" void FleetShared_OnNativeObtained(')+'\n'
 return 'typedef void (*FnGrantSharedItem)(const char*);\n'+s

def oot(build):
 item=read('soh/soh/Enhancements/randomizer/item.cpp');rnd=read('soh/soh/Enhancements/randomizer/randomizer.cpp');bridge=read('soh/soh/OTRGlobals.cpp');logic=read('soh/soh/Enhancements/randomizer/logic.cpp');text=read('soh/soh/Enhancements/custom-message/text.cpp')
 s=COMMON+r'''
#include "global.h"
#include "soh/Enhancements/randomizer/randomizer.h"
#include "soh/Enhancements/randomizer/static_data.h"
#include "soh/Enhancements/randomizer/logic.h"
#include "soh/Enhancements/item-tables/ItemTableManager.h"
#include "soh/Enhancements/custom-message/CustomMessageTypes.h"
#include "soh/FleetShipCombo/FleetComboItems.h"
#include "soh/FleetShipCombo/FleetComboItemsGlue.h"
#include "soh/FleetShipCombo/FleetComboIds.h"
#include "soh/FleetShipCombo/FleetSharedItems.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "mods/nei_save.h"
#include "combo/rando/RpgStats.h"
extern "C" {#include "variables.h"
SaveContext gSaveContext{};PlayState* gPlayState=nullptr;
}
static NeiSaveData nei;
extern "C" NeiSaveData* Nei_Save(){return &nei;}
extern "C" void GameInteractor_ExecuteOnFlagSet(int16_t,int16_t){}
extern "C" void Flags_SetEventChkInf(s32){assert(false);}
extern "C" s32 Inventory_HasEmptyBottle(){return 1;}
extern "C" u8 Randomizer_GetSettingValue(RandomizerSettingKey){return 0;}
static bool SOH_ShareSoulReceipt(int){return false;}
static bool FleetSync_IsApplyingFc(){return false;}
namespace FleetRpg {ComboRpgState Read(){return {};}}
struct SaveManager {static SaveManager* Instance;void SaveFile(int){++persisted;}};SaveManager mgr;SaveManager* SaveManager::Instance=&mgr;
bool Combo_OotIsForeground(){return foreground;}
Randomizer::Randomizer(){} Randomizer::~Randomizer(){}
u8 Randomizer::GetRandoSettingValue(RandomizerSettingKey){return 0;}
static Randomizer randomizer;
struct OTRGlobals {static OTRGlobals* Instance;Randomizer* gRandomizer=&randomizer;};OTRGlobals globals;OTRGlobals* OTRGlobals::Instance=&globals;
'''.replace('extern "C" {#include','extern "C" {\n#include')
 s+='ItemObtainability Randomizer::GetItemObtainabilityFromRandomizerGet(RandomizerGet randoGet){bool tycoonWallet=false;u8 infiniteUpgrades=0;'+re.search(r'u8 numWallets = [^;]+;',rnd)[0]+'switch(randoGet){'+cases(rnd,'        case RG_PROGRESSIVE_WALLET:','        case RG_PROGRESSIVE_SCALE:')+'default:assert(false);return CANT_OBTAIN_ALREADY_HAVE;}}\n'
 s+='Text::Text()=default;\n'+block(text,'Text::Text(std::string english_, std::string french_, std::string german_)')+'\n'
 s+='namespace Rando {\n'+block(item,'Item::Item()')+'\n'
 starts=[m.start() for m in re.finditer(r'Item::Item\(const RandomizerGet',item)]
 for a in starts:s+=block(item[a:],'Item::Item(')+'\n'
 for fn in ('Logic::Logic()','void Logic::SetSaveContext(','uint32_t Logic::CurrentUpgrade(','bool Logic::CheckRandoInf('):s+=block(logic,fn)+'\n'
 s+='static std::shared_ptr<Logic> walletLogic=std::make_shared<Logic>();\nstd::array<Item,RG_MAX> StaticData::itemTable;\nstd::unordered_map<std::string,RandomizerGet> StaticData::itemNameToEnum;\nItem& StaticData::RetrieveItem(RandomizerGet id){return itemTable.at(id);}\nvoid StaticData::InitItemTable(){\n'
 rows=read('soh/soh/Enhancements/randomizer/item_list.cpp')
 for token in ('RG_PROGRESSIVE_WALLET','RG_ADULT_WALLET','RG_GIANT_WALLET','RG_TYCOON_WALLET','RG_CHILD_WALLET','RG_BLUE_RUPEE'):
  line=re.search(r'^\s*itemTable\['+token+r'\].*$',rows,re.M);assert line,token;s+=line[0].split('.CustomIcon(')[0].rstrip(';')+';\n'
 s+='itemNameToEnum["Progressive Wallet"]=RG_PROGRESSIVE_WALLET; }\n'
 s+=block(item,'static bool ItemResolvesFromState(')+'\n'
 wallet=cases(item[item.index('std::shared_ptr<GetItemEntry> Item::GetGIEntry('):],'        case RG_PROGRESSIVE_WALLET:','        case RG_PROGRESSIVE_SCALE:')
 footer=item[item.index('    if (giEntry != nullptr && (actual == RG_NONE'):item.index('GetItemEntry Item::GetGIEntry_Copy()')]
 s+='std::shared_ptr<GetItemEntry> Item::GetGIEntry(RandomizerGet* actualOut) const {if(giEntry&&!ItemResolvesFromState(randomizerGet))return giEntry;auto logic=walletLogic;RandomizerGet actual=RG_NONE;bool tycoonWallet=false;u8 infiniteUpgrades=0;switch(randomizerGet){'+wallet+'default:assert(false);}\n'+footer+'\n'+block(item,'GetItemEntry Item::GetGIEntry_Copy()')+'\n}\n'
 s+='ItemTableManager::ItemTableManager(){} ItemTableManager::~ItemTableManager(){}\nItemTableManager itm;ItemTableManager* ItemTableManager::Instance=&itm;\nGetItemEntry ItemTableManager::RetrieveItemEntry(uint16_t table,uint16_t id){assert(table==MOD_NONE&&id==GI_RUPEE_BLUE);return Rando::StaticData::RetrieveItem(RG_BLUE_RUPEE).GetGIEntry_Copy();}\n'
 s+=glue('soh')+'\n'+transport('soh')+'\n'
 param=read('soh/src/code/z_parameter.c');a=param.index('    } else if (item == ITEM_WALLET_ADULT) {');b=param.index('    } else if (item == ITEM_STICK_UPGRADE_20)',a)
 s+='extern "C" u8 Return_Item(u8,ModIndex,ItemID){return ITEM_NONE;}\nextern "C" void Rupees_ChangeBy(s16){}\nextern "C" u8 Item_Give(PlayState*,u8 item){if(false){'+param[a:b]+'}else{assert(item==ITEM_RUPEE_BLUE);}return ITEM_NONE;}\nextern "C" u16 Randomizer_Item_Give(PlayState*,GetItemEntry){assert(false&&"finite OoT wallets must dispatch through native MOD_NONE");return 0;}\n'
 s+=block(bridge,'void Combo_ApplyItemReceiveSideEffects(')+'\n'+block(bridge,'void Combo_FlushDormantAccumulators(')+'\n'+block(bridge,'void Combo_GrantResolvedOOT(')+'\n'+block(bridge,'static bool GrantOotItemByName(')+'\n'+block(bridge,'extern "C" COMBO_EXPORT void SOH_GrantCrossItem(')+'\n'+block(bridge,'extern "C" COMBO_EXPORT void SOH_GrantSharedItem(')+'\n'
 s+=r'''
static void Reset(){gSaveContext={};nei={};persisted=forwards=0;sReceiveDepth=0;sPullPending=false;Rando::walletLogic->SetSaveContext(&gSaveContext);Flags_SetRandomizerInf(RAND_INF_HAS_WALLET);}
static void Home(){const bool can=randomizer.GetItemObtainabilityFromRandomizerGet(RG_PROGRESSIVE_WALLET)==CAN_OBTAIN;auto gie=can?Rando::StaticData::RetrieveItem(RG_PROGRESSIVE_WALLET).GetGIEntry_Copy():itm.RetrieveItemEntry(MOD_NONE,GI_RUPEE_BLUE);assert(gie.modIndex==MOD_NONE);Item_Give(gPlayState,gie.itemId);}
static json State(){return {{"level",CUR_UPG_VALUE(UPG_WALLET)},{"capacity",CUR_CAPACITY(UPG_WALLET)},{"childFlag",bool(Flags_GetRandomizerInf(RAND_INF_HAS_WALLET))},{"hasWallet",bool(Flags_GetRandomizerInf(RAND_INF_HAS_WALLET))},{"obtained",nei.comboObtainedFc[FCI_WALLET]},{"applied",nei.comboAppliedFc[FCI_WALLET]},{"forwards",forwards},{"persisted",persisted},{"receiveDepth",sReceiveDepth}};}
'''
 sync=read('soh/soh/FleetShipCombo/FleetSync.cpp');a=sync.index('        auto upg = [&u]');b=sync.index('\n        Inventory_ChangeUpgrade(UPG_QUIVER',a)
 s+='void ApplyWalletSnapshot(const json& sh){const auto& u=sh["upgrades"];'+sync[a:b]+'}\n'
 return s+protocol('soh')

def mm(build):
 bridge=read('mm/2s2h/BenPort.cpp');conv=read('mm/2s2h/Rando/ConvertItem.cpp');give=read('mm/2s2h/Rando/GiveItem.cpp');param=read('mm/src/code/z_parameter.c')
 s=COMMON+r'''
#include "Rando/Rando.h"
#include "Rando/Wallets.h"
#include "FleetShipCombo/FleetComboItems.h"
#include "FleetShipCombo/FleetComboItemsGlue.h"
#include "FleetShipCombo/FleetSharedItems.h"
#include "FleetShipCombo/FleetShipCombo.h"
#include "mods/nei_save.h"
#include "mods/combo_rpg.h"
#include "2s2h/GameInteractor/GameInteractor.h"
extern "C" {
#include "variables.h"
#include "functions.h"
SaveContext gSaveContext{};PlayState* gPlayState=nullptr;
int gFcCombo_SuppressRecord=0;
void GameInteractor_ExecuteOnFlagSet(FlagType,uint32_t){}
void GameInteractor_ExecuteOnFlagUnset(FlagType,uint32_t){}
s32 Inventory_HasEmptyBottle(){return 1;}
int ComboRpg_IsEnabled(int){return 0;}
}
static NeiSaveData nei;
extern "C" NeiSaveData* Nei_Save(){return &nei;}
static bool MM_ShareSoulReceipt(int){return false;}
void SaveManager_SaveCurrentForCombo(){++persisted;}
namespace Rando {bool gComboDormantGive=false;thread_local bool gItemGrantSimulation=false;}
'''
 s+=glue('mm')+'\n'+transport('mm')+'\n'
 items=read('mm/2s2h/Rando/StaticData/Items.cpp');a=items.index('#define RI(');b=items.index('\n\n',a)
 s+='namespace Rando::StaticData {\n'+items[a:b]+'\nstd::map<RandoItemId,RandoStaticItem> Items={\n'
 for token in ('RI_PROGRESSIVE_WALLET','RI_WALLET_CHILD','RI_WALLET_ADULT','RI_WALLET_GIANT','RI_WALLET_TYCOON','RI_RUPEE_RED'):
  s+=re.search(r'^\s*RI\('+token+r',.*$',items,re.M)[0]+'\n'
 s+='};\n#undef RI\n'+block(items,'static const std::unordered_map<RandoItemId, std::string>& ComboItemEmitNames()')+'\n'+block(items,'const std::string& GetItemDisplayName(')+'\n}\n'
 obtainable=cases(conv,'        case RI_PROGRESSIVE_WALLET:','        case RI_WALLET_ADULT:')
 resolve=cases(conv[conv.index('RandoItemId Rando::ConvertItem('):],'            case RI_PROGRESSIVE_WALLET:','            case RI_PROGRESSIVE_SWORD:')
 s+='namespace Rando {\nbool IsItemObtainable(RandoItemId randoItemId,RandoCheckId){bool hasObtainedCheck=false;switch(randoItemId){'+obtainable+'default:return true;}}\nRandoItemId ConvertItem(RandoItemId randoItemId,RandoCheckId rc){if(IsItemObtainable(randoItemId,rc)){switch(randoItemId){'+resolve+'default:return randoItemId;}}return RI_JUNK;}\n'
 # The actual outermost record/relay path executes before the actual wallet grant arms.
 a=give.index('    static thread_local int sGiveDepth');b=give.index('#ifdef COMBO_BUILD',a)
 depth=give[a:b]
 record=block(give,'    if (sGiveDepth == 1 && gFcCombo_SuppressRecord == 0) {');record=record[:record.index('        // "You found X"')]+'    }\n'
 walletcases=cases(give,'        case RI_WALLET_ADULT:','        case RI_GS_TOKEN_SWAMP:')
 s+='u32 gRandoPickupSerial=0;\nvoid GiveItem(RandoItemId randoItemId,RandoCheckId){'+depth+record+'switch(randoItemId){case RI_WALLET_CHILD:Wallets::Give(randoItemId);break;'+walletcases+'case RI_RUPEE_RED:case RI_JUNK:break;default:assert(false);}}\n}\n'
 a=param.index('    } else if (item == ITEM_WALLET_ADULT)');b=param.index('    } else if',param.index('    } else if (item == ITEM_WALLET_GIANT)',a)+1)
 s+='extern "C" u8 Item_Give(PlayState*,u8 item){if(false){'+param[a:b]+'}else{assert(false);}return ITEM_NONE;}\n'
 for marker in ('static bool Combo_IsBottleRefill(','void Combo_MM_FlushAccumulators(','static void Combo_MM_GiveDormantResolvedInternal(','void Combo_MM_GiveDormantResolved(','static const std::unordered_map<std::string, RandoItemId>& Combo_MM_SpoilerNameToItemId('):
  s+=block(bridge,marker)+'\n'
 for marker in ('static bool GrantMmItemByName(','extern "C" COMBO_EXPORT void MM_GrantCrossItem(','extern "C" COMBO_EXPORT void MM_GrantSharedItem('):s+=block(bridge,marker)+'\n'
 s+=r'''
static void Reset(){gSaveContext={};nei={};persisted=forwards=0;sReceiveDepth=0;sPullPending=false;gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_RANDO;RANDO_SAVE_OPTIONS[RO_SHUFFLE_CHILD_WALLET]=1;}
static void Home(){Rando::GiveItem(Rando::ConvertItem(RI_PROGRESSIVE_WALLET));}
static json State(){return {{"level",CUR_UPG_VALUE(UPG_WALLET)},{"capacity",CUR_CAPACITY(UPG_WALLET)},{"childFlag",bool(Flags_GetRandoInf(RANDO_INF_WALLET_CHILD))},{"hasWallet",bool(RandoCapacity_HasWallet())},{"obtained",nei.comboObtainedFc[FCI_WALLET]},{"applied",nei.comboAppliedFc[FCI_WALLET]},{"forwards",forwards},{"persisted",persisted},{"receiveDepth",sReceiveDepth}};}
'''
 sync=read('mm/2s2h/FleetShipCombo/FleetSync.cpp');a=sync.index('        auto upg = [&u]');b=sync.index('\n        // Wallet level 3',a)
 s+='void ApplyWalletSnapshot(const json& sh){const auto& u=sh["upgrades"];'+sync[a:b]+'}\n'
 return s+protocol('mm')


def protocol(game):
 prefix='SOH' if game=='soh' else 'MM'
 init='Rando::StaticData::InitItemTable();' if game=='soh' else ''
 return '\nint main(){'+init+r'''std::string line;while(std::getline(std::cin,line)){auto c=json::parse(line);auto op=c.at("op").get<std::string>();if(op=="reset")Reset();else if(op=="home"){foreground=true;Home();}else if(op=="cross"){foreground=false;'''+prefix+r'''_GrantCrossItem("Progressive Wallet");}else if(op=="shared"){foreground=false;'''+prefix+r'''_GrantSharedItem(c.at("name").get<std::string>().c_str());}else if(op=="sync")ApplyWalletSnapshot(c.at("state"));else if(op=="reload"){auto saved=gSaveContext;gSaveContext={};gSaveContext=saved;}else assert(op=="state");std::cout<<json({{"state",State()}}).dump()<<std::endl;}}
'''

def exercise(build):
 errors={g:(build/(g+'-stderr.log')).open('w') for g in ('soh','mm')}
 procs={g:subprocess.Popen([str(build/g)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errors[g],text=True,bufsize=1,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'}) for g in errors}
 def request(game,command,depth=0):
  assert depth<3,'peer receipt echoed through receive guard'
  proc=procs[game];proc.stdin.write(json.dumps(command)+'\n');proc.stdin.flush()
  while True:
   line=proc.stdout.readline()
   assert line,game+' native component exited; see '+str(build/(game+'-stderr.log'))
   message=json.loads(line)
   if 'peer' in message:
    request('mm' if game=='soh' else 'soh',{'op':'shared','name':message['peer']},depth+1)
    proc.stdin.write('{"ack":true}\n');proc.stdin.flush()
   else:return message['state']
 def sync(order):
  for game in order:
   source=request(game,{'op':'state'})
   request('mm' if game=='soh' else 'soh',{'op':'sync','state':{'upgrades':{'wallet':source['level']}}})
 results=[]
 try:
  for order in itertools.permutations(('O1','O2','M')):
   for routes in itertools.product(('home','cross'),repeat=3):
    for mask in range(8):
     for exchange in (('soh','mm'),('mm','soh')):
      for game in procs:request(game,{'op':'reset'})
      trace=[]
      for i,token in enumerate(order):
       game='mm' if token=='M' else 'soh'
       request(game,{'op':routes[i]})
       if mask&(1<<i):sync(exchange)
       for host in procs:
        state=request(host,{'op':'reload'})
        assert state['receiveDepth']==0
       trace.append({host:request(host,{'op':'state'}) for host in procs})
      sync(exchange);sync(exchange)
      final={game:request(game,{'op':'state'}) for game in procs}
      for game,state in final.items():
       assert state['level']==2 and state['capacity']==500 and state['hasWallet'],(order,routes,mask,exchange,final)
       assert state['obtained']==state['applied']==0,(game,'wallet tier double-recorded as progressive FC count',state)
      results.append({'order':order,'routes':routes,'syncMask':mask,'handoffOrder':exchange,'trace':trace,'final':final})
  output={'result':'PASS','fixtureSha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'sourceCommit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),'nativeHostSaves':'real OoT/MM SaveContext and native item/flag/upgrade layouts','scenarioCount':len(results),'explicitChildFlagFinalVariants':sorted({r['final']['mm']['childFlag'] for r in results}),'scope':'actual source-component grant/dispatch/conversion/relay/wallet-snapshot paths; renderer, notification, transport scheduling and persistence I/O are controlled; not gameplay or graph substitution','scenarios':results}
  ((build.parent if ARGS.output else build)/'result.json').write_text(json.dumps(output,indent=2)+'\n')
  print('PASS actual native-wallet source components:',len(results),'receipt/order/physical-delivery/switch schedules; MM final native base/Adult/Giant capacity500; FC counters stable; save reloads')
 finally:
  for p in procs.values():p.stdin.close()
  for p in procs.values():p.wait(timeout=10)
  for p in errors.values():p.close()

def main():
 with tempfile.TemporaryDirectory(prefix='native-wallet-receipts-') as tmp:
  if ARGS.output:
   ARGS.output.mkdir(parents=True,exist_ok=False)
   build=ARGS.output/'build';build.mkdir()
  else:build=Path(tmp)
  for game in ('soh','mm'):
   p=build/(game+'.cpp');p.write_text(oot(build) if game=='soh' else mm(build))
   cs=build/(game+'.c');cs.write_text(native_c(game))
   # Complete owning host native headers; unrelated flag notifications are services.
   subprocess.run([os.environ.get('CC','cc'),*flags(game,build,True),'-c',str(cs),'-o',str(build/(game+'.o'))],check=True)
   subprocess.run([os.environ.get('CXX','c++'),*flags(game,build),str(p),str(build/(game+'.o')),'-o',str(build/game)],check=True)
  exercise(build)
if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--sanitize',action='store_true');parser.add_argument('--output',type=Path);ARGS=parser.parse_args();main()
