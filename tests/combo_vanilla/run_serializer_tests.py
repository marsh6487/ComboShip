#!/usr/bin/env python3
"""Execute actual OoT SaveFileThreaded with a previously seeded member document."""
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'soh/soh/SaveManager.cpp').read_text()
start=source.index('void SaveManager::SaveFileThreaded(')
begin=source.index('{',start);depth,end=1,begin+1
while depth:
    depth+=(source[end]=='{')-(source[end]=='}');end+=1
code=r'''
#include <cassert>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <iostream>
#include <nlohmann/json.hpp>
using nlohmann::json;
#define COMBO_BUILD
#define SPDLOG_INFO(...) ((void)0)
#define SPDLOG_ERROR(...) ((void)0)
constexpr int QUEST_NORMAL=0,QUEST_RANDOMIZER=1,QUEST_OOTXMM=2,QUEST_SPEEDRUN=3,QUEST_SPEEDRUN_MASTER=4;
constexpr int FILE_TYPE_SAVE_VANILLA=0,FILE_TYPE_SAVE_RANDO=1,FILE_TYPE_SAVE_SPEEDRUN=4,SECTION_ID_BASE=0;
struct SaveContext { struct { struct { int id=QUEST_NORMAL; } quest; } ship; };
struct SceneDBState {};
struct SceneDBSaveScope { SceneDBSaveScope(const SceneDBState&){} };
static bool combined=true,vanilla=true;
int Combo_IsCombinedGame(){return combined;}
int Combo_PeekVanillaMode(int){return combined&&vanilla;}
int Combo_SOH_SaveCrossDungeonCheckpoint(){return -1;}
json Combo_SOH_SavePendingChecks(){return json::array();}
namespace ComboRando { enum { GAME_OOT=0 }; }
static json written;
static void Write(int,int,const char* payload){written=json::parse(payload);}
static auto gComboWriteGameSave=Write;
struct GameInteractor {
    struct OnSaveFile {};
    static GameInteractor* Instance;
    template<class Hook> void ExecuteHooks(int,int){}
};
static GameInteractor interactor;
GameInteractor* GameInteractor::Instance=&interactor;
class SaveManager {
  public:
    struct SaveFuncInfo {std::string name;int version=1;std::function<void(const SaveContext&,int,bool)> func;bool saveWithBase=true;int parentSection=-1;};
    std::mutex saveMtx;
    json saveBlock;
    json* currentJsonContext=nullptr;
    std::map<int,SaveFuncInfo> sectionSaveHandlers;
    int sectionIndex=2;
    void InitMeta(int){}
    std::string GetFileName(int){return "unused";}
    bool WriteFileSafely(const std::string&,const std::string& payload){written=json::parse(payload);return true;}
    void SaveFileThreaded(int,const SaveContext&,int,const SceneDBState&);
};
'''
code+=source[start:end]
code+=r'''
int main(){
    SaveManager manager;
    manager.sectionSaveHandlers[0]={"base",4,[&](const SaveContext&,int,bool){(*manager.currentJsonContext)["playerName"]=json::array({1,2,3,4,5,6,7,8});},true,-1};
    manager.sectionSaveHandlers[1]={"randomizer",1,[&](const SaveContext&,int,bool){(*manager.currentJsonContext)["seed"]=99;},true,-1};
    SaveContext save;
    const auto seedDocument=json{{"sections",{{"randomizer",{{"data",{{"seed",12}}}}},{"speedrun",{{"data",{{"old",true}}}}},{"unloadedMod",{{"keep",true}}}}}};
    manager.saveBlock=json::object();
    manager.SaveFileThreaded(1,save,SECTION_ID_BASE,{});
    assert(written["sections"]["base"]["data"]["playerName"].size()==8);
    manager.saveBlock=seedDocument;
    manager.SaveFileThreaded(1,save,SECTION_ID_BASE,{});
    assert(written["fileType"]==0&&!written["sections"].contains("randomizer")&&!written["sections"].contains("speedrun"));
    assert(written["sections"]["base"]["data"]["playerName"].size()==8);
    assert(written["sections"]["unloadedMod"]["keep"]==true);
    vanilla=false;save.ship.quest.id=QUEST_RANDOMIZER;manager.saveBlock=seedDocument;
    manager.SaveFileThreaded(0,save,SECTION_ID_BASE,{});
    assert(written["fileType"]==1&&written["sections"]["randomizer"]["data"]["seed"]==99);
    combined=false;vanilla=true;save.ship.quest.id=QUEST_NORMAL;manager.saveBlock=seedDocument;
    manager.SaveFileThreaded(2,save,SECTION_ID_BASE,{});
    assert(written["sections"].contains("randomizer")); // Solo serialization stays upstream.
    std::cout<<"PASS actual OoT serializer: no stale seed/speedrun sections in combined Vanilla; seeded/solo controls and unknown mod data retained\n";
}
'''
flags=['-std=c++20','-Wall','-Wextra','-Werror','-I',str(ROOT)]
try:
    import nlohmann_json
    flags+=['-I',str(nlohmann_json.get_include())]
except ImportError:
    pass
if '--sanitize' in sys.argv:
    flags+=['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
with tempfile.TemporaryDirectory(prefix='native-vanilla-serialize-') as directory:
    cpp=Path(directory)/'serialize.cpp';cpp.write_text(code);exe=Path(directory)/'serialize'
    subprocess.run([*shlex.split(os.environ.get('CXX','c++')),*flags,str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
