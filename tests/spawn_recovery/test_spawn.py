import pathlib, subprocess, sys, tempfile

root = pathlib.Path(__file__).parent
variant = sys.argv[1] if len(sys.argv) > 1 else 'candidate'
source_root = root / variant if (root / variant).is_dir() else root.parents[1]
source = (source_root / 'soh/soh/z_play_otr.cpp').read_text()
def function(signature):
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

prefix = r'''
#include <cassert>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#define COMBO_BUILD 1
#define SPDLOG_ERROR(...) ((void)0)
#define SPDLOG_INFO(...) ((void)0)
void trace(const char*,...) __attribute__((format(printf,1,2)));
void trace(const char*,...) {}
#define LUSLOG_INFO(...) trace(__VA_ARGS__)
#define LUSLOG_ERROR(...) trace(__VA_ARGS__)
#define osSyncPrintf(...) ((void)0)
void lusprintf(const char*, int, int, const char*, ...) {}
using s32=int;
constexpr int SCENE_DODONGOS_CAVERN=1, ENTR_DODONGOS_CAVERN_ENTRANCE=4, SDC_MAX=100, SDC_DEFAULT=0;
struct Save {int entranceIndex=582, sceneLayer=5, cutsceneIndex=0xfff1, nextCutsceneIndex=0xffef, cutsceneTrigger=0, respawnFlag=2, linkAge=1;} gSaveContext;
struct ActorEntry {struct {int16_t x=0,y=0,z=0;} pos; int16_t params=0;};
struct PlayState {int sceneNum=0,sceneConfig=0,roomCtx=0,numRooms=1; void* sceneSegment=nullptr; ActorEntry* linkActorEntry=nullptr;};
namespace Ship {
struct Init {std::string Path="resolved";};
struct IResource {virtual ~IResource()=default; Init init; Init* GetInitData(){return &init;}};
struct Archive {bool HasFile(const std::string&){return true;}};
struct RM {bool missing=false; int calls=0; std::shared_ptr<IResource> resource;
auto LoadResource(const char* p){if(++calls>4) throw std::runtime_error("recursive fallback"); return missing || std::string(p)=="missing" || std::string(p).ends_with("/scene.json") ? std::shared_ptr<IResource>{}:resource;}
auto GetArchiveManager(){return std::make_shared<Archive>();} bool IsAltAssetsEnabled(){return true;}};
struct Context {std::shared_ptr<RM> rm; static Context* GetRawInstance(){static Context c; return &c;} auto GetResourceManager(){return rm;}};
struct CrossRMRegistry {inline static std::shared_ptr<RM> oot; static auto GetOrActive(const char*){return oot;}};
struct OwnRMScope {std::shared_ptr<RM> prior; OwnRMScope(const char*){prior=Context::GetRawInstance()->rm; Context::GetRawInstance()->rm=CrossRMRegistry::oot;} ~OwnRMScope(){Context::GetRawInstance()->rm=prior;}};
}
namespace SOH {struct Scene:Ship::IResource {};}
struct SceneDB {struct Entry {bool valid=true; int drawConfig=0;}; inline static SceneDB* Instance; Entry entry; Entry& RetrieveEntry(int id){entry.valid=id!=999;return entry;} std::string GetScenePath(int id){return id==1?"fallback":id>=6&&id<=8?"scenes/request/scene.json":"missing";} std::string GetLegacyScenePath(int id,bool){return id==6?"native":id==7?"missing":"";}};
bool ResourceMgr_IsGameMasterQuest(){return false;}
static int observedRespawn=-1, observedEntrance=-1, observedLayer=-1, observedCutscene=-1;
void OTRPlay_InitScene(PlayState*,int){observedRespawn=gSaveContext.respawnFlag;observedEntrance=gSaveContext.entranceIndex;observedLayer=gSaveContext.sceneLayer;observedCutscene=gSaveContext.cutsceneIndex;}
int func_80096FE8(PlayState*,int*){return 0;}
void InitSceneRainPolicy(PlayState*,Ship::Archive*,int,int){}
void GameInteractor_ExecuteOnSceneInit(int){}
'''
main = r'''
int main(int argc,char** argv){
 SceneDB db;SceneDB::Instance=&db;PlayState play;
 auto oot=std::make_shared<Ship::RM>(), mm=std::make_shared<Ship::RM>();
 oot->resource=std::make_shared<SOH::Scene>();mm->resource=std::make_shared<SOH::Scene>();
 Ship::CrossRMRegistry::oot=oot;Ship::Context::GetRawInstance()->rm=oot;
 std::string test=argv[1];
 if(test=="owner") {Ship::Context::GetRawInstance()->rm=mm;assert(OTRPlay_LoadFile(&play,"ok")==oot->resource.get());assert(Ship::Context::GetRawInstance()->rm==mm);}
 else if(test=="type") {oot->resource=std::make_shared<Ship::IResource>();assert(OTRPlay_LoadFile(&play,"ok")==nullptr);}
 else if(test=="terminal") {oot->missing=true;bool failed=false;try{OTRPlay_SpawnScene(&play,5,0);}catch(const std::runtime_error&){failed=true;}assert(failed && oot->calls==2);}
 else if(test=="native-recovery") {OTRPlay_SpawnScene(&play,6,1);assert(play.sceneNum==6 && observedRespawn==2 && observedEntrance==582 && observedLayer==5 && oot->calls==2);}
 else if(test=="missing-native") {OTRPlay_SpawnScene(&play,7,1);assert(play.sceneNum==1 && observedRespawn==0 && observedEntrance==4 && oot->calls==3);}
 else if(test=="custom-no-native") {OTRPlay_SpawnScene(&play,8,1);assert(play.sceneNum==1 && oot->calls==2);}
 else if(test=="success") {gSaveContext.respawnFlag=2;OTRPlay_SpawnScene(&play,1,0);assert(observedRespawn==2 && observedEntrance==582 && observedLayer==5);}
 else {OTRPlay_SpawnScene(&play,test=="invalid"?999:5,0);assert(play.sceneNum==1);assert(observedRespawn==0 && observedEntrance==4 && observedLayer==0 && observedCutscene==0);}
 puts("PASS");
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=pathlib.Path(tmp)/'test.cpp'; binary=pathlib.Path(tmp)/'test'
    cpp.write_text(prefix+function('Ship::IResource* OTRPlay_LoadFile(')+'\n'+function('extern "C" void OTRPlay_SpawnScene(')+main)
    subprocess.run(['c++','-std=c++20','-Werror=format','-fsanitize=undefined','-fno-sanitize-recover=all',str(cpp),'-o',str(binary)],check=True)
    failed=[]
    for case in ['fallback','invalid','terminal','success','owner','type','native-recovery','missing-native','custom-no-native']:
        p=subprocess.run([str(binary),case],capture_output=True,text=True)
        print(case, 'PASS' if p.returncode==0 else 'FAIL', p.stderr.strip())
        if p.returncode:failed.append(case)
    sys.exit(bool(failed))
