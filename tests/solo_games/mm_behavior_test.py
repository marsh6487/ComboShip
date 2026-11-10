#!/usr/bin/env python3
"""Compile MM's production boundary bodies and exercise native solo versus Combo sessions."""
from pathlib import Path
import re
import os
import subprocess
import tempfile
import argparse

ROOT = Path(__file__).resolve().parents[2]
DEPS = Path(os.environ.get('COMBO_TEST_DEPS', str(ROOT / 'build-deps'))) / 'include'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()

def block(source, marker):
    start = source.index(marker)
    opening = source.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

COMMON = r'''
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#define COMBO_BUILD
#define COMBO_EXPORT
#include "combo/ComboGameMode.h"
extern "C" { int gComboGameMode = COMBO_GAME_MM; }
#define SPDLOG_ERROR(...) ((void)0)
#define SPDLOG_INFO(...) ((void)0)
'''

save = (ROOT/'mm/2s2h/SaveManager/SaveManager.cpp').read_text()
if 'SaveManager_GetSavesFolderPath() {' in save:
    path_impl = block(save, 'static std::filesystem::path SaveManager_GetSavesFolderPath()')
else:
    path_impl = re.search(r'const std::filesystem::path savesFolderPath[^;]+;', save).group(0)
save_program = COMMON + r'''
#include <nlohmann/json.hpp>
#include <ctime>
const std::string appShortName="2ship";
namespace Ship { struct Context {
static std::string GetPathRelativeToAppDirectory(const std::string& s,const std::string& = "") {return s;}
}; struct Config {std::string GetString(const char*,const std::string&){return "stale-oot.sav";}}; }
namespace ComboRando { constexpr int GAME_MM=1; }
int writes=0,reads=0;
int marketWrites=0;
void ComboBlackMarket_SaveState(nlohmann::json&,int){++marketWrites;}
int forestWrites=0,forestSlot=-1;
void ComboSoloForestMm_SaveState(nlohmann::json&,int slot){++forestWrites;forestSlot=slot;}
void WriteCombined(int,int,const char*) {++writes;}
const char* ReadCombined(int,int) {++reads;return "{\"combo\":true}";}
auto gComboWriteGameSave=WriteCombined;
auto gComboReadGameSave=ReadCombined;
int Combo_MM_SaveCrossDungeonCheckpoint() {return -1;}
namespace Notification { struct Notice {std::string message;}; void Emit(Notice) {} }
struct SaveOptions {int optionId=0,audioSetting=0,zTargetSetting=0,language=0,languageSetting=0;};
void to_json(nlohmann::json& j,const SaveOptions& o){j={{"optionId",o.optionId},{"audioSetting",o.audioSetting},{"zTargetSetting",o.zTargetSetting}};}
void from_json(const nlohmann::json& j,SaveOptions& o){j.at("optionId").get_to(o.optionId);j.at("audioSetting").get_to(o.audioSetting);j.at("zTargetSetting").get_to(o.zTargetSetting);}
#define SAVE_OPTIONS_VALID_ID 0xA51D
#define CVAR_AUDIO_SETTING "gSettings.AudioSetting"
#define CVAR_ZTARGET_SETTING "gSettings.ZTargetSetting"
constexpr int LANGUAGE_ENG=0,SAVE_AUDIO_STEREO=0;
int cvarWrites=0;
void CVarSetInteger(const char*,int){++cvarWrites;}
void CVarSave(){}
void* CVarGet(const char*){return nullptr;}
int CVarGetInteger(const char*,int def){return def;}
''' + path_impl + '\n' + block(save, 'static int SaveManager_ClassifySaveFile(') + '\n'
for marker in ['void SaveManager_WriteSaveFile(', 'void SaveManager_DeleteSaveFile(',
               'int SaveManager_ReadSaveFile(const std::filesystem::path& fileName, nlohmann::json& j) {',
               'void SaveManager_MoveInvalidSaveFile(', 'int SaveManager_GetOpenFileSlot()',
               'void SaveManager_WriteGlobalOptions(', 'bool SaveManager_ReadGlobalOptions(']:
    save_program += block(save, marker) + '\n'
save_program += block((ROOT/'mm/2s2h/Enhancements/Items/ColorPictograph.cpp').read_text(), 'static std::filesystem::path PictoDir()') + '\n'
save_program += block((ROOT/'mm/2s2h/BenPort.cpp').read_text(), 'std::filesystem::path GetSaveFile(std::shared_ptr<Ship::Config> Conf)') + '\n'
save_program += r'''
int main(int argc,char** argv) {
    std::filesystem::current_path(argv[1]);
    assert(PictoDir()=="saves/mm");
    auto config=std::make_shared<Ship::Config>();
    assert(GetSaveFile(config)==std::filesystem::absolute("saves/mm/mm_save.sav"));
    const nlohmann::json source={{"native",true}};
    // Stale callbacks must not divert solo writes, reads, backups, copy or erasure.
    SaveManager_WriteSaveFile("file1.json",source);
    SaveManager_WriteSaveFile("file1backup.json",source);
    SaveManager_WriteSaveFile("global.json",source);
    assert(writes==0);
    assert(marketWrites==2); // native slot + backup; global options are unrelated
    assert(forestWrites==2 && forestSlot==0);
    for (const char* name:{"file1.json","file1backup.json","global.json"})
        assert(std::filesystem::exists(std::filesystem::path("saves/mm")/name));
    assert(!std::filesystem::exists("saves/file1.json"));
    nlohmann::json loaded;
    assert(SaveManager_ReadSaveFile("file1.json",loaded)==0 && loaded==source && reads==0);
    assert(SaveManager_GetOpenFileSlot()==2);
    SaveManager_WriteSaveFile("file2.json",loaded); // native copy uses these same primitives
    assert(forestWrites==3 && forestSlot==1);
    SaveManager_DeleteSaveFile("file1.json");
    SaveManager_DeleteSaveFile("file1backup.json");
    assert(SaveManager_GetOpenFileSlot()==1);
    assert(SaveManager_ReadSaveFile("file2.json",loaded)==0 && loaded==source);
    SaveManager_MoveInvalidSaveFile("file2.json","bad save");
    assert(!std::filesystem::exists("saves/mm/file2.json"));
    bool moved=false;for(const auto& e:std::filesystem::directory_iterator("saves/mm"))
        moved|=e.path().filename().string().starts_with("file2_invalid_");
    assert(moved);
    SaveOptions options;options.optionId=SAVE_OPTIONS_VALID_ID;options.audioSetting=2;options.zTargetSetting=1;
    SaveManager_WriteGlobalOptions(options);assert(cvarWrites==0);
    SaveOptions reread;assert(SaveManager_ReadGlobalOptions(reread));
    assert(reread.audioSetting==2 && reread.zTargetSetting==1);
    // Combined slots retain the container route; backups stay suppressed.
    gComboGameMode=COMBO_GAME_BOTH;
    assert(PictoDir()=="Save");
    assert(GetSaveFile(config)==std::filesystem::absolute("stale-oot.sav"));
    SaveManager_WriteSaveFile("file1.json",source);
    SaveManager_WriteSaveFile("file1backup.json",source);
    assert(writes==1);
    assert(forestWrites==3); // the private native forest never writes into a combined slot
    assert(SaveManager_ReadSaveFile("file1.json",loaded)==0 && loaded["combo"]==true && reads==1);
    SaveManager_DeleteSaveFile("file1.json");
    assert(writes==2);
    SaveManager_WriteGlobalOptions(options);assert(cvarWrites==2);
    std::cout<<"MM native save boundaries passed\n";
}
'''

gui = (ROOT/'mm/2s2h/BenGui/BenGui.cpp').read_text()
menu_program = COMMON + r'''
struct Menu {};
std::shared_ptr<Menu> mBenMenu=std::make_shared<Menu>();
struct Gui {int assignments=0;void SetMenu(std::shared_ptr<Menu>) {++assignments;}} gui;
struct Window {Gui* GetGui() {return &gui;}} window;
namespace Ship {struct Context {static Context* GetRawInstance(){static Context c;return &c;}
Window* GetWindow(){return &window;}};}
void SetupMenu(){mBenMenu=std::make_shared<Menu>();}
''' + block(gui, 'void ActivateMenu()') + r'''
int main() {
    ActivateMenu();assert(gui.assignments==1);
    gComboGameMode=COMBO_GAME_BOTH;ActivateMenu();assert(gui.assignments==1);
    std::cout<<"MM native menu activation passed\n";
}
'''

goal = (ROOT/'mm/2s2h/FleetShipCombo/FleetComboGoal.cpp').read_text()
goal_program = COMMON + r'''
#include "combo/vanilla/ComboVanillaMode.h"
extern "C" { int gComboVanillaMode = 0; }
int CVarGetInteger(const char*,int) {return 0;}
#define CVAR_COMBO 1
''' + block(goal, 'static bool BeatBothBosses()') + r'''
int main() {
    assert(!BeatBothBosses());
    gComboGameMode=COMBO_GAME_BOTH;assert(BeatBothBosses());
    std::cout<<"MM native boss goal passed\n";
}
'''
pools = (ROOT/'mm/2s2h/Rando/Logic/GeneratePools.cpp').read_text()
pool_prefix = pools[pools.index('void GeneratePools('):pools.index('    std::vector<RandoItemId> startingItems')]
fleet = (ROOT/'mm/2s2h/FleetShipCombo/FleetShipCombo.cpp').read_text()
legacy_start=fleet.index('constexpr uint32_t kFscMagic')
legacy_end=fleet.index('#ifdef _WIN32',legacy_start)
# Keep the actual legacy getter and LazyOpen behavior. Only OS shared-memory
# acquisition is controlled; standalone has no attached legacy host process.
getter_marker='int FleetShipCombo_GetActiveGame(void)'
legacy_getter_start=fleet.index(getter_marker,fleet.index(getter_marker)+len(getter_marker))
legacy_access='#ifndef COMBO_BUILD\n'+fleet[legacy_start:legacy_end]+r'''
FscShared* sShared=nullptr;
bool sLazyOpenTried=false;
FscShared* MapShared(bool create) {assert(!create);return nullptr;}
'''+block(fleet,'FscShared* LazyOpen()')+'\n'+block(fleet[legacy_getter_start:],getter_marker)+'\n#endif\n'

# This boundary executes the current native helper dependencies with their real
# types and declaration/default argument, rather than a four-slot model save.
pool_program = COMMON.replace('#define COMBO_BUILD\n', '') + r'''
#include <vector>
#include "Rando/SongEvents.h"
#include "Rando/VanillaCompatibility.h"
#include "combo/rando/MagicMirrorSettings.h"
#include "mm/2s2h/FleetShipCombo/FleetShipCombo.h"
''' + legacy_access + block(fleet, 'int FleetCombo_UnifiedPoolActive(void)') + r'''
int mirrorRequested=0,mirrorReads=0;
extern "C" int32_t CVarGetInteger(const char* name,int32_t fallback) {
    assert(std::string(name)=="gRando.Generation.ShuffleMagicMirror" && fallback==0);
    ++mirrorReads;return mirrorRequested;
}
extern "C" { int gMMComboGoalHunt=0,gMMComboGoalRequired=99,gMMComboGoalPieces=100; }
namespace Rando::Logic {
''' + pool_prefix + '}\n}\n' + r'''
int main() {
    using Rando::Logic::GeneratePools;
    assert(!FleetCombo_UnifiedPoolActive()); // Actual modern launcher boundary, even with stale callbacks.
    RandoSaveInfo info{};std::vector<RandoCheckId> checks;std::vector<RandoItemId> items;
    info.randoSaveOptions[RO_SHUFFLE_TRIFORCE_PIECES]=1;
    info.randoSaveOptions[RO_TRIFORCE_PIECES_REQUIRED]=7;
    info.randoSaveOptions[RO_TRIFORCE_PIECES_MAX]=9;
    GeneratePools(info,checks,items); // Uses the actual header's default fourth argument.
    assert(info.randoSaveOptions[RO_SHUFFLE_TRIFORCE_PIECES]==1 &&
           info.randoSaveOptions[RO_TRIFORCE_PIECES_REQUIRED]==7 &&
           info.randoSaveOptions[RO_TRIFORCE_PIECES_MAX]==9);
    assert(mirrorReads==0 && info.randoSaveOptions[RO_SHUFFLE_MAGIC_MIRROR]==0);
#ifdef COMBO_BUILD
    gComboGameMode=COMBO_GAME_BOTH;GeneratePools(info,checks,items);
    assert(info.randoSaveOptions[RO_SHUFFLE_TRIFORCE_PIECES]==0);
    assert(mirrorReads==1 && info.randoSaveOptions[RO_SHUFFLE_MAGIC_MIRROR]==0);
    mirrorRequested=1;gComboGameMode=COMBO_GAME_MM;GeneratePools(info,checks,items);
    assert(mirrorReads==1 && info.randoSaveOptions[RO_SHUFFLE_MAGIC_MIRROR]==0);
    gComboGameMode=COMBO_GAME_BOTH;GeneratePools(info,checks,items);
    assert(mirrorReads==2 && info.randoSaveOptions[RO_SHUFFLE_MAGIC_MIRROR]==RO_GENERIC_ON);
    gMMComboGoalHunt=1;GeneratePools(info,checks,items);
    assert(info.randoSaveOptions[RO_TRIFORCE_PIECES_REQUIRED]==99 &&
           info.randoSaveOptions[RO_TRIFORCE_PIECES_MAX]==100);
#else
    // An original 2Ship build cannot enable combined overrides through globals.
    gComboGameMode=COMBO_GAME_BOTH;mirrorRequested=1;gMMComboGoalHunt=1;
    GeneratePools(info,checks,items);
    assert(!FleetCombo_UnifiedPoolActive() && mirrorReads==0);
    assert(info.randoSaveOptions[RO_SHUFFLE_TRIFORCE_PIECES]==1 &&
           info.randoSaveOptions[RO_TRIFORCE_PIECES_REQUIRED]==7 &&
           info.randoSaveOptions[RO_TRIFORCE_PIECES_MAX]==9);
    assert(info.randoSaveOptions[RO_SHUFFLE_MAGIC_MIRROR]==0);
#endif
    std::cout<<"MM native randomizer goals passed\n";
}
'''

port=(ROOT/'mm/2s2h/BenPort.cpp').read_text()
entry_program = COMMON + r'''
bool sComboTransitionActive=false;
struct OTRGlobals {inline static void* Instance=nullptr;};
int gComboStartFileNum=2,runs=0;
void MM_RunMain(){assert(sComboTransitionActive && gComboStartFileNum==-1);OTRGlobals::Instance=&runs;++runs;}
'''+block(port,'extern "C" COMBO_EXPORT void MM_SetGameMode(')+'\n'+block(port,'extern "C" COMBO_EXPORT void MM_RunSolo(')+r'''
int main(){
    MM_SetGameMode(999);assert(Combo_IsCombinedGame());
    MM_SetGameMode(COMBO_GAME_MM);assert(!Combo_IsCombinedGame());MM_RunSolo();assert(runs==1);
    MM_SetGameMode(COMBO_GAME_BOTH);assert(!Combo_IsCombinedGame());
    MM_SetGameMode(999);assert(!Combo_IsCombinedGame());
    std::cout<<"MM solo entry boundary passed\n";
}
'''
opening=(ROOT/'mm/2s2h/Enhancements/Gameplay/ComboOpeningForest.h').read_text()
opening_program=COMMON+r'''
#include "combo/vanilla/ComboVanillaMode.h"
#include "combo/vanilla/ComboVanillaPolicy.h"
// The existing combined opening control is a seeded save.
extern "C" { int gComboVanillaMode = 0; }
constexpr int GAMEMODE_NORMAL=0,SCENE_LOST_WOODS=1,ENTR_SCENE_OPENING_DUNGEON=2;
#define ENTRANCE(scene,spawn) (0x600+(spawn))
struct PlayState {int sceneId=SCENE_LOST_WOODS;};
struct {int gameMode=GAMEMODE_NORMAL,fileNum=0,sceneLayer=0,nextCutsceneIndex=17,cutsceneTrigger=1,respawnFlag=2;
struct {int entrance=ENTRANCE(LOST_WOODS,0),cutsceneIndex=0;
struct {struct {unsigned int threeDayResetCount=0;} playerData;} saveInfo;} save;} gSaveContext;
bool ComboLostWoods_IsPlayableSlot(int slot){return slot>=0&&slot<3;}
'''+block(opening,'static inline int ComboOpeningForest_IsExploration(')+'\n'+block(opening,'static inline void ComboOpeningForest_NormalizeEntry(')+r'''
int main(){
    PlayState play;assert(!ComboOpeningForest_IsExploration(&play));
    gSaveContext.save.entrance=ENTR_SCENE_OPENING_DUNGEON<<9;ComboOpeningForest_NormalizeEntry();
    assert(gSaveContext.save.entrance==(ENTR_SCENE_OPENING_DUNGEON<<9) && gSaveContext.cutsceneTrigger==1);
    gComboGameMode=COMBO_GAME_BOTH;ComboOpeningForest_NormalizeEntry();
    assert(gSaveContext.save.entrance==ENTRANCE(CLOCK_TOWER_INTERIOR,0) && gSaveContext.cutsceneTrigger==0);
    gSaveContext.save.entrance=ENTRANCE(LOST_WOODS,0);assert(ComboOpeningForest_IsExploration(&play));
    std::cout<<"MM native prologue preservation passed\n";
}
'''

give=(ROOT/'mm/2s2h/Rando/GiveItem.cpp').read_text()
simulation_declaration=re.search(r'^extern thread_local bool gItemGrantSimulation;',
                                (ROOT/'mm/2s2h/Rando/Rando.h').read_text(),re.M).group(0)
simulation_definition=re.search(r'^thread_local bool Rando::gItemGrantSimulation[^;]*;',give,re.M).group(0)
simulation_state=('namespace Rando {\n'+simulation_declaration+'\n}\n'+simulation_definition+
                  '\nusing Rando::gItemGrantSimulation;\n')
triforce_case=give[give.index('        case RI_TRIFORCE_PIECE:'):give.index('        // Technically these should never be used')]
triforce_program=COMMON+simulation_state+r'''
#include <vector>
constexpr int RI_TRIFORCE_PIECE=1,RI_TRIFORCE_PIECE_PREVIOUS=2,RI_SOUL_BOSS_MAJORA=3;
constexpr int RO_TRIFORCE_PIECES_REQUIRED=0,RANDO_INF_OBTAINED_SOUL_OF_BOSS_MAJORA=0;
constexpr int TRANS_TRIGGER_START=1,TRANS_TYPE_FADE_BLACK=0;
#define ENTRANCE(scene,spawn) (0x1234+(spawn))
struct {int fileNum=0;struct {struct {struct {int foundTriforcePieces=0;int options[1]={2};} rando;} shipSaveInfo;} save;} gSaveContext;
#define RANDO_SAVE_OPTIONS gSaveContext.save.shipSaveInfo.rando.options
struct GIEventTransition {int entrance,cutsceneIndex,transitionTrigger,transitionType;};
struct GameInteractor {static GameInteractor* Instance;std::vector<GIEventTransition> events;} gi;
GameInteractor* GameInteractor::Instance=&gi;
int completions=0,majoraSouls=0,comboProgress=0;
void GameInteractor_ExecuteOnGameCompletion(){++completions;}
bool Flags_GetRandoInf(int){return false;}
namespace Rando {void GiveItem(int item){assert(item==RI_SOUL_BOSS_MAJORA);++majoraSouls;}}
void Progress(int,int){++comboProgress;}
void (*gMMComboTriforceProgress)(int,int)=Progress;
void Collect(int item){switch(item){
'''+triforce_case+r'''
}}
int main(){
    Collect(RI_TRIFORCE_PIECE);assert(!completions && !comboProgress);
    Collect(RI_TRIFORCE_PIECE);assert(completions==1 && majoraSouls==1 && !comboProgress);
    assert(gi.events.size()==1 && gi.events[0].cutsceneIndex==0xFFF7);
    gSaveContext.save.shipSaveInfo.rando.foundTriforcePieces=1;gComboGameMode=COMBO_GAME_BOTH;
    Collect(RI_TRIFORCE_PIECE);assert(completions==1 && comboProgress==1);
    std::cout<<"MM native Triforce completion passed\n";
}
'''

failed=[]
with tempfile.TemporaryDirectory(prefix='mm-solo-behavior-') as d:
    tmp=Path(d)
    for name,program in [('save',save_program),('menu',menu_program),('boss',goal_program),('pool',pool_program),('entry',entry_program),('opening',opening_program),('triforce',triforce_program)]:
        # Pool helper uses both real native build guards; the other legacy
        # boundary programs retain their combined-host solo/session controls.
        for combined in ((False,True) if name=='pool' else (True,)):
            label=name+('-combo' if combined else '-original')
            cpp=tmp/(label+'.cpp');exe=tmp/label;cpp.write_text(program)
            flags=['-std=c++20','-Wall','-Wextra','-Wno-unused-parameter','-I'+str(ROOT),'-I'+str(DEPS)]
            sources=[str(cpp)]
            if name=='pool':
                flags+=['-DMM_BUILD_DLL','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX',
                        '-DCONTROLLERBUTTONS_T=uint32_t','-include',str(ROOT/'tests/shuffle_parity/souls/headless.h'),
                        '-ffunction-sections','-fdata-sections','-Wl,--gc-sections']
                for path in ('mm/include','mm/include/PR','mm/2s2h','mm','mm/src','mm/assets','libultraship/include','combo','combo/menu'):
                    flags+=['-isystem',str(ROOT/path)]
                if combined:flags+=['-DCOMBO_BUILD']
                sources+=[str(ROOT/'mm/2s2h/Rando/SongEvents.cpp')]
            if args.sanitize:
                flags+=['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
            compiled=subprocess.run([os.environ.get('CXX','c++'),*flags,*sources,'-o',str(exe)])
            if compiled.returncode:
                failed.append(label+' (compile)');continue
            run_dir=tmp/(label+'-run');run_dir.mkdir()
            ran=subprocess.run([str(exe),str(run_dir)])
            if ran.returncode:failed.append(label)
            else:print('PASS MM solo behavior',label,flush=True)
if failed:
    raise SystemExit('MM behavior failures: '+', '.join(failed))
