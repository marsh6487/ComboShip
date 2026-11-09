#!/usr/bin/env python3
"""Execute MM native save creation/generation/spoiler, reset and owl-exit production bodies."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
import argparse

ROOT = Path(__file__).resolve().parents[2]
DEPS = Path(os.environ.get('COMBO_TEST_DEPS', '/workspace/scratch/4288e889429f/build-deps'))
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sanitize',action='store_true')
args=parser.parse_args()

def block(source, marker):
    start=source.index(marker); opening=source.index('{',start); end=opening+1; depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]

COMMON=r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#define COMBO_BUILD
#define COMBO_EXPORT
#include "combo/ComboGameMode.h"
extern "C" {int gComboGameMode=COMBO_GAME_MM;}
#define SPDLOG_INFO(...) ((void)0)
#define SPDLOG_WARN(...) ((void)0)
#define SPDLOG_ERROR(...) ((void)0)
#define SPDLOG_DEBUG(...) ((void)0)
'''
source=(ROOT/'mm/2s2h/Rando/MiscBehavior/OnFileCreate.cpp').read_text()
generation=(ROOT/'mm/2s2h/Rando/Logic/SeedGeneration.cpp').read_text()
spoiler=(ROOT/'mm/2s2h/Rando/Spoiler/File.cpp').read_text()
apply=(ROOT/'mm/2s2h/Rando/Spoiler/Apply.cpp').read_text()
inventory=(ROOT/'mm/src/code/z_inventory.c').read_text()
actor=(ROOT/'mm/src/code/z_actor.c').read_text()
starting=(ROOT/'mm/2s2h/Rando/StartingItems.cpp').read_text()
pool_body=block((ROOT/'mm/2s2h/Rando/Logic/GeneratePools.cpp').read_text(),'void GeneratePools(')
pool_prefix=pool_body[:pool_body.index('    const bool hasOotGameplay')]
pool_boundary=pool_prefix+'    ++pools; checkPool={RC_STARTING_ITEM_DEKU_MASK}; itemPool={RI_HEART_PIECE};\n}\n'
native_services='#include "global.h"\n#include "variables.h"\n#include "2s2h/GameInteractor/GameInteractor.h"\n'+'\n'.join(re.search(r'^(?:u16|u8) '+name+r'\[\] = \{.*?^\};',inventory,re.M|re.S)[0] for name in ('gEquipMasks','gEquipNegMasks','gEquipShifts'))+'\n'+block(actor,'s32 Flags_GetRandoInf(')+'\n'+block(actor,'void Flags_SetWeekEventReg(')+'\n'
flag_notify=block((ROOT/'mm/2s2h/GameInteractor/GameInteractor.cpp').read_text(),'void GameInteractor_ExecuteOnFlagSet(')
creation=COMMON.replace('#define COMBO_BUILD\n','')+r'''
#include <nlohmann/json.hpp>
#include <imgui.h>
#include "Rando/SongEvents.h"
#include "Rando/VanillaCompatibility.h"
#include "overlays/actors/ovl_En_Sth/z_en_sth.h"
extern "C" {SaveContext gSaveContext{};}
std::map<std::string,int> cvars;
extern "C" int32_t CVarGetInteger(const char* key,int32_t def) {auto i=cvars.find(key);return i==cvars.end()?def:i->second;}
extern "C" const char* CVarGetString(const char* key,const char*) {return std::string(key)=="gRando.SpoilerFile"?"chosen.json":"native-seed";}
std::string Ship_RemoveSpecialCharacters(const char* s) {return s;}
extern "C" s32 Ship_Random(s32,s32) {return 42;}
uint32_t Ship_Hash(std::string) {return 123;}
void Ship_Random_Seed(u64) {}
int sound=0;extern "C" void Audio_PlaySfx(u16 s) {sound=s;}
namespace Notification {struct Notice {std::string prefix;ImVec4 prefixColor;std::string message;};void Emit(Notice){}}
GameInteractor gi;
GameInteractor* GameInteractor::Instance=&gi;
'''+flag_notify+r'''
int generatedHooks=0;
const std::string appShortName="2ship";
namespace Ship {struct Context {static std::string GetPathRelativeToAppDirectory(const std::string& p,const std::string&){return p;}};}
int pools=0,fills=0,grants=0,applies=0,spoilers=0;
namespace Rando {
namespace StaticData {
std::map<RandoOptionId,RandoStaticOption> Options={
    {RO_LOGIC,{RO_LOGIC,"Logic","logic",RO_LOGIC_NO_LOGIC}},
    {RO_SHUFFLE_SONG_EVENTS,{RO_SHUFFLE_SONG_EVENTS,"Song Events","song-events",0}}};
std::map<RandoItemId,RandoStaticItem> Items={
    {RI_WOODFALL_SMALL_KEY,{.randoItemId=RI_WOODFALL_SMALL_KEY,.randoItemType=RITYPE_SMALL_KEY}}};
std::map<RandoCheckId,RandoStaticCheck> Checks={
    {RC_STARTING_ITEM_DEKU_MASK,{.randoCheckId=RC_STARTING_ITEM_DEKU_MASK,.name="Native Check",.randoItemId=RI_SWORD_KOKIRI}},
    {RC_STARTING_ITEM_SONG_OF_HEALING,{.randoCheckId=RC_STARTING_ITEM_SONG_OF_HEALING,.name="Small Key",.randoItemId=RI_WOODFALL_SMALL_KEY}}};
RandoItemId GetItemIdFromName(const char* name){return std::string(name)=="RI_SWORD_KOKIRI"?RI_SWORD_KOKIRI:RI_UNKNOWN;}
}
std::vector<RandoItemId> GetStartingItemsFromConfig(){return {RI_SHIELD_HERO};}
std::vector<RandoItemId> GetStartingItemsFromSpoiler(nlohmann::json&){++applies;return {RI_SHIELD_HERO};}
std::vector<RandoItemId> GetSariaPriorityItemsFromSpoiler(nlohmann::json&){return {};}
'''+block(starting,'void SetStartingItemsInSave(')+'\n'+block(starting,'std::vector<RandoItemId> GetStartingItemsFromSave(')+r'''
std::vector<RandoItemId> GetSariaPriorityItemsFromConfig(){return {};}
void SetSariaPriorityItemsInSave(RandoSaveInfo&,std::vector<RandoItemId>&){}
void GrantStartingItems(){++grants;}
void GiveItem(RandoItemId,RandoCheckId){}
namespace Logic {
// The full placement engine remains the original controlled service boundary.
// Generation/import/normalization/song policy and actual save layouts are real.
'''+pool_boundary+r'''
void ApplyNoLogicToSaveContext(std::vector<RandoCheckId>&,std::vector<RandoItemId>&){++fills;}
void ApplyNearlyNoLogicToSaveContext(std::vector<RandoCheckId>&,std::vector<RandoItemId>&){++fills;}
void ApplyGlitchlessLogicToSaveContext(std::vector<RandoCheckId>&,std::vector<RandoItemId>&){++fills;}
}
namespace Spoiler {
nlohmann::json GenerateFromSaveContext(){return {{"type","2S2H_RANDO_SPOILER"}};}
void SaveToFile(const std::string&,nlohmann::json){++spoilers;}
void RefreshOptions(){}
void SaveOotItemAreas(uint32_t,nlohmann::json){assert(Combo_IsCombinedGame());}
''' + block(apply,'void ApplyToSaveContext(')+r'''
''' + block(spoiler,'nlohmann::json LoadFromFile(')+r'''
}
namespace MiscBehavior {void OnFileCreate(s16);}
}
'''+ 'namespace Rando {\n'+block(generation,'void PrepareNewRandomizedSave(')+'\n'+block(generation,'std::string GenerateSeedInSaveContext(')+'\n}\n'+block(source,'void Rando::MiscBehavior::OnFileCreate(')+r'''
int main(int argc,char** argv) {
    std::filesystem::current_path(argv[1]);
    gSaveContext.save.playerForm=PLAYER_FORM_DEKU;
    gSaveContext.save.cutsceneIndex=17;
    memset(gSaveContext.save.saveInfo.inventory.dungeonKeys,-1,sizeof(gSaveContext.save.saveInfo.inventory.dungeonKeys));
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnRandoSeedGeneration>([]{++generatedHooks;});
    Rando::MiscBehavior::OnFileCreate(0); // native vanilla
    assert(gSaveContext.save.shipSaveInfo.saveType==SAVETYPE_VANILLA);
    assert(gSaveContext.save.playerForm==PLAYER_FORM_DEKU && gSaveContext.save.cutsceneIndex==17);
    assert(pools==0 && fills==0 && grants==0 && generatedHooks==0);
    cvars["gRando.Enabled"]=1;
    Rando::MiscBehavior::OnFileCreate(1); // native generation
    assert(gSaveContext.save.shipSaveInfo.saveType==SAVETYPE_RANDO);
    assert(gSaveContext.save.shipSaveInfo.rando.finalSeed==123);
    assert(pools==1 && fills==1 && grants==1 && spoilers==1 && generatedHooks==1);
    assert(gSaveContext.save.shipSaveInfo.rando.foundDungeonKeys[0]==-1);
    assert(Rando::GetStartingItemsFromSave(gSaveContext.save.shipSaveInfo.rando)==std::vector<RandoItemId>{RI_SHIELD_HERO});
    assert(RANDO_SAVE_CHECKS[RC_STARTING_ITEM_DEKU_MASK].eligible);
    assert(RANDO_SAVE_CHECKS[RC_STARTING_ITEM_SONG_OF_HEALING].eligible);
    cvars["gRando.SpoilerFileIndex"]=1;
    std::filesystem::create_directory("randomizer");
    {std::ofstream f("randomizer/chosen.json");f<<R"({"type":"2S2H_RANDO_SPOILER","finalSeed":456,"options":{},"checks":{"Native Check":{"randoItemId":"RI_SWORD_KOKIRI"}}})";}
    Rando::MiscBehavior::OnFileCreate(2); // native spoiler loading
    assert(applies==1 && grants==2 && generatedHooks==2 && pools==1);
    assert(gSaveContext.save.shipSaveInfo.rando.finalSeed==456);
    assert(Rando::GetStartingItemsFromSave(gSaveContext.save.shipSaveInfo.rando)==std::vector<RandoItemId>{RI_SHIELD_HERO});
    assert(RANDO_SAVE_CHECKS[RC_STARTING_ITEM_DEKU_MASK].randoItemId==RI_SWORD_KOKIRI && RANDO_SAVE_CHECKS[RC_STARTING_ITEM_DEKU_MASK].shuffled);
    assert(RANDO_SAVE_CHECKS[RC_STARTING_ITEM_SONG_OF_HEALING].randoItemId==RI_WOODFALL_SMALL_KEY && !RANDO_SAVE_CHECKS[RC_STARTING_ITEM_SONG_OF_HEALING].shuffled && !RANDO_SAVE_CHECKS[RC_STARTING_ITEM_SONG_OF_HEALING].skipped);
    {std::ofstream f("randomizer/chosen.json");f<<"{\"type\":\"COMBO_RANDO_SPOILER\"}";}
    Rando::MiscBehavior::OnFileCreate(2); // stale Combo seed cannot silently create a solo file
    assert(applies==1 && generatedHooks==2 && sound==NA_SE_SY_QUIZ_INCORRECT);
    assert(gSaveContext.save.shipSaveInfo.saveType==SAVETYPE_VANILLA);
    assert(gSaveContext.save.saveInfo.playerData.newf[0]=='\0');
    // Exercise the actual spoiler import and exact production pool policy
    // prefix with a deliberately distinctive valid assignment.
    auto& info=gSaveContext.save.shipSaveInfo.rando;
    namespace SE=ComboRando::SongEvents;
    nlohmann::json payload={{"finalSeed",42},{"options",{{"Logic",RO_LOGIC_NO_LOGIC},{"Song Events",1}}},{"checks",nlohmann::json::object()}};
    payload["songEvents"]=std::vector<uint8_t>(13,3);
    Rando::Spoiler::ApplyToSaveContext(payload);
    assert(std::vector<uint8_t>(info.songEvents,info.songEvents+13)==std::vector<uint8_t>(13,3));
    memset(info.songEvents,0,sizeof(info.songEvents));
    Rando::SongEvents::RestoreForOracle(info);
    assert(std::vector<uint8_t>(info.songEvents,info.songEvents+13)==std::vector<uint8_t>(13,3));
    std::vector<RandoCheckId> checks;std::vector<RandoItemId> items;
    Rando::Logic::GeneratePools(info,checks,items,true); // preview keeps imported provenance
    Rando::SongEvents::RestoreForOracle(info);
    assert(std::vector<uint8_t>(info.songEvents,info.songEvents+13)==std::vector<uint8_t>(13,3));
    Rando::Logic::GeneratePools(info,checks,items); // new generation clears prior import
    const auto generated=SE::Generate(SE::Game::Mm,42,true);
    assert(generated!=std::vector<uint8_t>(13,3));
    memset(info.songEvents,0,sizeof(info.songEvents));
    Rando::SongEvents::RestoreForOracle(info);
    assert(std::vector<uint8_t>(info.songEvents,info.songEvents+13)==generated);
    payload.erase("songEvents");
    Rando::Spoiler::ApplyToSaveContext(payload); // legacy missing map regenerates
    assert(std::vector<uint8_t>(info.songEvents,info.songEvents+13)==generated);
    payload["songEvents"]=std::vector<uint8_t>(12,3);
    bool rejected=false;
    try {Rando::Spoiler::ApplyToSaveContext(payload);} catch(const std::invalid_argument&){rejected=true;}
    assert(rejected);
    payload["songEvents"]=std::vector<uint8_t>(13,2); // foreign-only unsupported native identity
    rejected=false;
    try {Rando::Spoiler::ApplyToSaveContext(payload);} catch(const std::invalid_argument&){rejected=true;}
    assert(rejected);
    for(auto option:Rando::VanillaCompatibility::UnsupportedOptions)info.randoSaveOptions[option]=1;
    auto& night=info.randoSaveChecks[RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_2];
    night.shuffled=night.eligible=night.obtained=night.skipped=true;night.price=17;night.randoItemId=RI_SWORD_KOKIRI;
    payload["options"]["Logic"]=RO_LOGIC_VANILLA;
    Rando::Spoiler::ApplyToSaveContext(payload); // Vanilla normalizes before invalid map parsing
    for(auto option:Rando::VanillaCompatibility::UnsupportedOptions)assert(info.randoSaveOptions[option]==0);
    assert(!night.shuffled && !night.eligible && night.obtained && night.skipped && night.price==17 && night.randoItemId==RI_SWORD_KOKIRI);
    assert(std::vector<uint8_t>(info.songEvents,info.songEvents+13)==SE::Defaults(SE::Game::Mm));
    std::cout<<"MM native vanilla/generation/spoiler/foreign-seed boundaries passed\n";
}
'''
reset_source=(ROOT/'mm/2s2h/DeveloperTools/DebugConsole.cpp').read_text()
reset=COMMON+r'''
struct State {bool running=true;int next=0;} state,*gGameState=&state;
struct ConsoleLogoState {};
namespace Ship {struct Console{};}
#define STOP_GAMESTATE(s) ((s)->running=false)
#define SET_NEXT_GAMESTATE(s,init,size) ((s)->next=1)
#define ERROR_MESSAGE(...) ((void)0)
int returns=0;void MM_RequestComboReturn(){++returns;}
'''+block(reset_source,'static bool ResetHandler(')+r'''
int main() {
    assert(!ResetHandler(nullptr,{},nullptr));
    assert(!state.running && state.next==1 && returns==0);
    state={};gComboGameMode=COMBO_GAME_BOTH;assert(!ResetHandler(nullptr,{},nullptr));
    assert(state.running && returns==1);
    std::cout<<"MM native reset boundary passed\n";
}
'''
play=(ROOT/'mm/src/code/z_play.c').read_text()
start=play.index('                    if (gSaveContext.gameMode == GAMEMODE_OWL_SAVE) {')
end=play.index('                    } else if (gSaveContext.gameMode != GAMEMODE_FILE_SELECT)',start)
owl=COMMON+r'''
struct TitleSetupState {};
constexpr int GAMEMODE_OWL_SAVE=3,TRANS_MODE_OFF=0;
struct {int gameMode=GAMEMODE_OWL_SAVE;} gSaveContext;
struct State {bool running=true;int next=0;};
struct Play {State state;int transitionMode=3;};
#define STOP_GAMESTATE(s) ((s)->running=false)
#define SET_NEXT_GAMESTATE(s,init,size) ((s)->next=1)
int returns=0;void Combo_RequestOwlSaveQuit(){++returns;}
void Quit(Play* self) {
'''+play[start:end].replace('this','self')+r'''
                    }
}
int main(){
    Play p;Quit(&p);assert(!p.state.running && p.state.next==1 && returns==0);
    p={};gComboGameMode=COMBO_GAME_BOTH;Quit(&p);assert(p.state.running && p.transitionMode==0 && returns==1);
    std::cout<<"MM native owl-save exit boundary passed\n";
}
'''
failed=[]
with tempfile.TemporaryDirectory(prefix='mm-native-flow-') as directory:
    tmp=Path(directory)
    for name,program in [('create',creation),('reset',reset),('owl',owl)]:
        for combined in ((False,True) if name=='create' else (True,)):
            label=name+('-combo' if combined else '-original')
            cpp=tmp/(label+'.cpp');binary=tmp/label;cpp.write_text(program)
            flags=['-std=c++20','-I'+str(ROOT),'-I'+str(DEPS/'include')]
            sources=[str(cpp)]
            if name=='create':
                flags+=['-DMM_BUILD_DLL','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX',
                        '-DCONTROLLERBUTTONS_T=uint32_t','-include',str(ROOT/'tests/shuffle_parity/souls/headless.h'),
                        '-ffunction-sections','-fdata-sections','-Wl,--gc-sections']
                for path in ('mm/include','mm/include/PR','mm/2s2h','mm','mm/src','mm/assets','libultraship/include','combo','combo/menu'):
                    flags+=['-isystem',str(ROOT/path)]
                if combined:flags+=['-DCOMBO_BUILD']
                sources+=[str(ROOT/'mm/2s2h/Rando/SongEvents.cpp')]
                # Preserve native C initializer/conversion semantics for the
                # equipment masks and real flag reader; no C++ narrowing bypass.
                native_source=tmp/(label+'-services.c');native_object=tmp/(label+'-services.o')
                native_source.write_text(native_services)
                cflags=['-std=gnu17','-DMM_BUILD_DLL','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX',
                        '-DCONTROLLERBUTTONS_T=uint32_t','-I'+str(ROOT)]
                for path in ('mm/include','mm/include/PR','mm/2s2h','mm','mm/src','mm/assets','libultraship/include','combo','combo/menu'):
                    cflags+=['-isystem',str(ROOT/path)]
                if combined:cflags+=['-DCOMBO_BUILD']
                if args.sanitize:cflags+=['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie']
                subprocess.run([os.environ.get('CC','cc'),*cflags,'-c',str(native_source),'-o',str(native_object)],check=True)
                sources+=[str(native_object)]
            if args.sanitize:
                flags+=['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
            result=subprocess.run([os.environ.get('CXX','c++'),*flags,*sources,'-o',str(binary)])
            if result.returncode:failed.append(label+' (compile)');continue
            run=tmp/(label+'-run');run.mkdir()
            if subprocess.run([str(binary),str(run)]).returncode:failed.append(label)
            else:print('PASS MM native flow',label,flush=True)
if failed:raise SystemExit('MM native flow failures: '+', '.join(failed))
