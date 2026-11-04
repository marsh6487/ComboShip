// The runner inserts the complete production translation unit below, using
// the native game structs. Only hook dispatch, CVars and camera requests are
// service boundaries; the backup, restoration and scene-state code are real.
#include "z64.h"
extern "C" {
#include "functions.h"
#include "variables.h"
}
#include "soh/cvar_prefixes.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <memory>
#include <new>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <array>
#include <algorithm>


namespace TestHooks {
struct OnSceneInit { using Callback=std::function<void(int32_t)>; static inline Callback callback; };
struct OnPlayDestroy { using Callback=std::function<void()>; static inline Callback callback; };
template<class H,class F> void Set(bool on,F f){ H::callback=on?typename H::Callback(f):typename H::Callback{}; }
}
#define COND_HOOK(h,e,...) TestHooks::Set<TestHooks::h>(e,__VA_ARGS__)
#define SPDLOG_ERROR(...) ((void)0)
struct RegisterShipInitFunc { RegisterShipInitFunc(void(*)(),std::initializer_list<const char*>){} };
namespace SOH {
enum class SceneCommandID { SetCollisionHeader };
struct CollisionHeader { ::CollisionHeader collisionHeaderData{}; uint32_t surfaceTypesCount=0; };
struct Command { SceneCommandID cmdId=SceneCommandID::SetCollisionHeader; virtual ~Command()=default; };
struct SetCollisionHeader:Command { std::string fileName="collision"; std::shared_ptr<SOH::CollisionHeader> collisionHeader; void* GetRawPointer(){return &collisionHeader->collisionHeaderData;} };
struct Scene { std::vector<std::shared_ptr<Command>> commands; };
}
static std::shared_ptr<SOH::Scene> scene;
namespace Ship {
struct RM { std::shared_ptr<SOH::Scene> LoadResource(const std::string&){return scene;} };
struct Context { static Context* GetRawInstance(){static Context c;return &c;} RM* GetResourceManager(){static RM r;return &r;} };
}
struct SceneDB { static inline SceneDB* Instance; std::string GetScenePath(int){return "graveyard";} };
static int enabled=0;
static bool modAsset=false;
extern "C" int ResourceMgr_IsModAsset(const char*) { return modAsset; }
extern "C" { PlayState* gPlayState=nullptr; }
extern "C" int32_t CVarGetInteger(const char*,int32_t){return enabled;}
extern "C" SurfaceType SurfaceType_Unpack(u32 a,u32 b){SurfaceType s{};s.camera=a&255;s.exit=(a>>8)&31;s.material=b&15;return s;}
/* PRODUCTION */
static PlayState play;
static std::vector<SurfaceType> surfaces;
static std::vector<CollisionPoly> polys;
static void Attach(unsigned count, bool canonical=true){
 scene=std::make_shared<SOH::Scene>(); auto cmd=std::make_shared<SOH::SetCollisionHeader>();cmd->collisionHeader=std::make_shared<SOH::CollisionHeader>();
 scene->commands.push_back(cmd);surfaces.assign(count,{});polys.assign(675,{});
 for(auto range:std::array<std::pair<int,int>,2>{{{487,509},{651,658}}})for(int i=range.first;i<=range.second;i++)polys[i].type=20;
 if(!canonical)polys[487].type=2;
 if(modAsset)polys[487].normal.x=1234;
 if(count>33)polys[87].type=33;
 auto& h=cmd->collisionHeader->collisionHeaderData;h.surfaceTypeList=surfaces.data();h.polyList=polys.data();h.numPolygons=polys.size();cmd->collisionHeader->surfaceTypesCount=count;
 play.sceneNum=SCENE_GRAVEYARD;play.colCtx.colHeader=&h;gPlayState=&play;
 RegisterGraveHoleJumps();if(TestHooks::OnSceneInit::callback)TestHooks::OnSceneInit::callback(SCENE_GRAVEYARD);
}
int main(int argc,char** argv){
 SceneDB db;SceneDB::Instance=&db;std::string test=argv[1];
 if(test=="mod32-retained-types"){modAsset=true;Attach(32);assert(play.colCtx.colHeader->surfaceTypeList==surfaces.data());assert(polys[487].type==20);}
 else if(test=="custom39"){Attach(39);assert(play.colCtx.colHeader->surfaceTypeList==surfaces.data());assert(polys[87].type==33);assert(polys[487].type==20);}
 else if(test=="custom32"){Attach(32,false);assert(play.colCtx.colHeader->surfaceTypeList==surfaces.data());assert(polys[487].type==2);}
 else if(test=="vanilla31"||test=="vanilla32"){
  unsigned count=test=="vanilla31"?31:32;Attach(count);assert(polys[487].type==count);assert(play.colCtx.colHeader->surfaceTypeList[count].exit==0);
  enabled=1;RegisterGraveHoleJumps();assert(polys[487].type==20);assert(play.colCtx.colHeader->surfaceTypeList==surfaces.data());
 }
 else if(test=="teardown-reentry"){
  Attach(32);auto old=std::static_pointer_cast<SOH::SetCollisionHeader>(scene->commands[0])->collisionHeader;
  assert(TestHooks::OnPlayDestroy::callback);TestHooks::OnPlayDestroy::callback();assert(old->collisionHeaderData.surfaceTypeList==surfaces.data());assert(polys[487].type==20);
  old.reset();Attach(39);assert(play.colCtx.colHeader->surfaceTypeList==surfaces.data());TestHooks::OnPlayDestroy::callback();
 }
 else return 2;
 if(TestHooks::OnPlayDestroy::callback)TestHooks::OnPlayDestroy::callback();
 puts("PASS graveyard collision preservation/lifetime");
}
