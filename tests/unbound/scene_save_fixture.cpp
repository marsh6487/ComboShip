#include "global.h"
#include "soh/unbound/SceneDB.h"
#include <nlohmann/json.hpp>
#include <cassert>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>

#undef COMBO_BUILD
#define SPDLOG_INFO(...) ((void)0)
#define SPDLOG_ERROR(...) ((void)0)
#define SECTION_ID_BASE 0
#define FILE_TYPE_SAVE_VANILLA 0
#define FILE_TYPE_SAVE_RANDO 1
#define FILE_TYPE_SAVE_SPEEDRUN 2
extern "C" { SaveContext gSaveContext; }
std::unordered_map<int32_t,SavedSceneFlags> sCustomSceneFlags;
using ExtBitset = std::vector<uint32_t>;
std::unordered_map<int32_t,ExtBitset> sExtClearFlags,sExtLiveClearFlags,sExtTempClearFlags;
thread_local const SceneDBState* sSaveState;
/* ACTUAL SCENE PAYLOAD */

static std::unordered_map<int32_t,SceneDB::Entry> entries;
SceneDB::SceneDB() = default;
SceneDB::Entry& SceneDB::RetrieveEntry(int32_t id) { return entries[id]; }
static SceneDB scenes;
SceneDB* SceneDB::Instance=&scenes;
class GameInteractor {
  public:
    struct OnSaveFile {};
    static GameInteractor* Instance;
    template<typename T> void ExecuteHooks(int,int) {}
};
static GameInteractor interactor;
GameInteractor* GameInteractor::Instance=&interactor;
struct DeferredPool {
    std::function<void()> task;
    template<typename T> void detach_task(T callback) { task=std::move(callback); }
};
class SaveManager {
  public:
    static SaveManager* Instance;
    std::mutex saveMtx;
    nlohmann::json saveBlock;
    nlohmann::json* currentJsonContext=nullptr;
    struct SaveFuncInfo {
        std::string name;int version;
        std::function<void(const SaveContext&,int,bool)> func;
        bool saveWithBase;int parentSection=-1;
    };
    std::map<int,SaveFuncInfo> sectionSaveHandlers;
    int sectionIndex=2;
    std::shared_ptr<DeferredPool> smThreadPool=std::make_shared<DeferredPool>();
    void SaveFileThreaded(int,const SaveContext&,int,const SceneDBState&);
    void SaveSection(int,int,bool);
    nlohmann::json SaveToJsonObject();
    static bool WriteFileSafely(const std::filesystem::path&,const std::string&) { return true; }
    std::filesystem::path GetFileName(int) { return "unused"; }
    void InitMeta(int) {}
    template<typename T> void SaveData(const std::string& name,const T& value) {
        if(name.empty()) currentJsonContext->push_back(value);else (*currentJsonContext)[name]=value;
    }
    void SaveStruct(const std::string& name,std::function<void()> callback) {
        auto* previous=currentJsonContext;currentJsonContext=&(*previous)[name];
        *currentJsonContext=nlohmann::json::object();callback();currentJsonContext=previous;
    }
    void SaveArray(const std::string& name,size_t size,std::function<void(size_t)> callback) {
        auto* previous=currentJsonContext;currentJsonContext=&(*previous)[name];
        *currentJsonContext=nlohmann::json::array();
        for(size_t i=0;i<size;++i) callback(i);
        currentJsonContext=previous;
    }
};
SaveManager* SaveManager::Instance;
void SaveHorseScene(const SaveContext*) {}
void SaveSavedSceneName(const SaveContext*) {}
void SaveEntranceName(const char*,int32_t) {}
/* ACTUAL UNBOUND SAVE */
/* ACTUAL SAVE DISPATCH */

int main() {
    SaveManager manager;SaveManager::Instance=&manager;
    entries[128].valid=true;entries[128].isCustom=true;entries[128].name="custom";
    manager.sectionSaveHandlers[0]={"base",4,[&](const SaveContext& context,int,bool) { manager.SaveData("health",context.health); },true};
    manager.sectionSaveHandlers[1]={"unbound",1,SaveUnboundSection,true};
    gSaveContext.ship.quest.id=QUEST_NORMAL;
    gSaveContext.health=7;sCustomSceneFlags[128].chest=7;sExtClearFlags[128]={1,7};
    manager.SaveSection(0,SECTION_ID_BASE,true);
    assert(manager.smThreadPool->task);
    // The game changes before the queued worker runs; its whole payload must remain at the save boundary.
    gSaveContext.health=99;sCustomSceneFlags[128].chest=99;sExtClearFlags[128]={99};
    gSaveContext.ship.quest.id=QUEST_OOTXMM;
    std::thread worker(manager.smThreadPool->task);worker.join();
    auto& written=manager.saveBlock["sections"];
    assert(manager.saveBlock["fileType"]==FILE_TYPE_SAVE_VANILLA && manager.saveBlock["fleetCombo"]==false);
    assert(written["base"]["data"]["health"]==7);
    assert(written["unbound"]["data"]["sceneFlags"]["custom"]["chest"]==7);
    assert(written["unbound"]["data"]["roomClearExt"]["custom"]["bits"]==nlohmann::json({1,7}));
    auto synced=manager.SaveToJsonObject();
    assert(synced["fileType"]==FILE_TYPE_SAVE_RANDO && synced["fleetCombo"]==true);
    assert(synced["sections"]["base"]["data"]["health"]==99);
    assert(synced["sections"]["unbound"]["data"]["sceneFlags"]["custom"]["chest"]==99);
    assert(synced["sections"]["unbound"]["data"]["roomClearExt"]["custom"]["bits"]==nlohmann::json({99}));
    assert(sSaveState==nullptr); // RAII scope unwinds on both the worker and synchronous dispatch.
    auto outer=SceneDB_CaptureState();sCustomSceneFlags[128].chest=123;auto inner=SceneDB_CaptureState();
    { SceneDBSaveScope one(outer);assert(sSaveState->customSceneFlags.at(128).chest==99);
      { SceneDBSaveScope two(inner);assert(sSaveState->customSceneFlags.at(128).chest==123); }
      assert(sSaveState->customSceneFlags.at(128).chest==99); }
    assert(sSaveState==nullptr);
}
