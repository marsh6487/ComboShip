"""Execute the shared native adapter at stubbed engine boundaries, in both ABI variants."""
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[2]

STUB=r'''
#include <cstdint>
#include <cstring>
#include <string>
#include <map>
#include <cassert>
using s16=int16_t;
struct Vec3f{float x,y,z;};
struct Actor {int id=0,category=0,room=0;void(*update)()=nullptr;Actor*next=nullptr;struct{Vec3f pos{};}world,home;};
struct ActorListEntry{Actor*head=nullptr;Actor*first=nullptr;};
struct Player{Actor actor;};
struct PlayState{int sceneNum=81,sceneId=45;struct{ActorListEntry actorLists[12];}actorCtx;
 struct{struct{int num=0;}curRoom;}roomCtx;struct{int state=0;}pauseCtx,csCtx;
 struct{int msgMode=0;}msgCtx;int transitionTrigger=0,transitionMode=0,colCtx=0;};
struct CollisionPoly{struct{int y=32767;}normal;};struct WaterBox{};
constexpr int ACTOR_EN_TITE=1,ACTOR_EN_DEKUBABA=2,ACTOR_EN_ENCOUNT1=3;
constexpr int ACTOR_EN_MINIBLIN=101,ACTOR_EN_HAMMERGEIST=102,ACTOR_EN_SBEETLE=103,ACTOR_EN_NEI_LYNEL=104;
constexpr int ACTORCAT_NPC=4,ACTORCAT_DOOR=5,ACTORCAT_ENEMY=6,BGCHECK_SCENE=50;
constexpr int MSGMODE_NONE=0,CS_STATE_IDLE=0,TRANS_TRIGGER_OFF=0,TRANS_MODE_OFF=0;
PlayState state;PlayState*gPlayState=&state;Player testPlayer;
#define GET_PLAYER(play) (&testPlayer)
std::map<std::string,int>cv;bool assets=true,ground=true,wall=false,active=true;
int CVarGetInteger(const char*key,int fallback){auto i=cv.find(key);return i==cv.end()?fallback:i->second;}
bool Partners_IsActive(){return active;}
extern "C" void CustomEnemies_ReportSkipped(int,int,int,int){}
extern "C" uint8_t ResourceMgr_FileExists(const char*){return assets;}
extern "C" int NativeLynel_Register(){return assets?104:-1;}
extern "C" int NativeLynel_Id(){return assets?104:-1;}
extern "C" {s16 gEnMiniblinId=101,gEnHammergeistId=102,gEnSbeetleId=103;void Trutefel_EnsureActorsRegistered(){}}
CollisionPoly poly;
float floorStub(CollisionPoly**p,int*bg){*p=ground?&poly:nullptr;*bg=BGCHECK_SCENE;return 0;}
float BgCheck_EntityRaycastFloor5(int*,CollisionPoly**p,int*bg,Actor*,Vec3f*){return floorStub(p,bg);}
float BgCheck_EntityRaycastFloor5(PlayState*,int*,CollisionPoly**p,int*bg,Actor*,Vec3f*){return floorStub(p,bg);}
bool BgCheck_EntityCheckCeiling(int*,float*,Vec3f*,float,CollisionPoly**,int*,Actor*){return false;}
bool WaterBox_GetSurface1(PlayState*,int*,float,float,float*,WaterBox**){return false;}
bool BgCheck_EntityLineTest1(int*,Vec3f*,Vec3f*,Vec3f*,CollisionPoly**,bool,bool,bool,bool,int*){return wall;}
void update(){}
Actor pool[32];int spawned=0,killed=0;
Actor*Actor_Spawn(decltype(state.actorCtx)*ctx,PlayState*,int id,float x,float y,float z,int,int,int,int){
 Actor*a=&pool[spawned++];*a={};a->id=id;a->world.pos={x,y,z};a->update=update;
 a->next=ctx->actorLists[6].head;ctx->actorLists[6].head=ctx->actorLists[6].first=a;return a;}
void Actor_Kill(Actor*a){a->update=nullptr;++killed;}
#include "combo/encounters/NativeCustomEnemies.inc.h"
int main(){
 testPlayer.actor.world.pos=testPlayer.actor.home.pos={-1000,0,-1000};
 cv["gEnhancements.CustomEnemies.Molmauk"]=0;cv["gEnhancements.CustomEnemies.Beetle"]=0;cv["gEnhancements.CustomEnemies.Lynel"]=0;
 CustomEnemies_RecordEntry(&state,ACTOR_EN_TITE,0,0,0,-1);
 CustomEnemies_RecordEntry(&state,ACTOR_EN_TITE,0,0,0,-1);
 CustomEnemies_Tick();assert(spawned==0); // default off
 cv["gEnhancements.CustomEnemies.NaturalEncounters"]=1;assets=false;
 CustomEnemies_Tick();assert(spawned==0);assets=true;
 CustomEnemies_Tick();assert(spawned==1);CustomEnemies_Tick();assert(spawned==1);
 pool[0].update=nullptr;CustomEnemies_Tick();CustomEnemies_Tick();assert(spawned==1); // no repeat defeat
 cv["gEnhancements.CustomEnemies.NaturalEncounters"]=0;CustomEnemies_Tick();
 cv["gEnhancements.CustomEnemies.NaturalEncounters"]=1;CustomEnemies_Tick();assert(spawned==1);
 CustomEnemies_Reset();CustomEnemies_RecordEntry(&state,ACTOR_EN_TITE,0,0,0,-1);
 ground=false;CustomEnemies_Tick();assert(spawned==1);ground=true;CustomEnemies_Tick();assert(spawned==1); // invalid skipped
 CustomEnemies_Reset();CustomEnemies_RecordEntry(&state,ACTOR_EN_TITE,0,0,0,-1);
 wall=true;CustomEnemies_Tick();assert(spawned==1);wall=false;
 CustomEnemies_Reset();CustomEnemies_RecordEntry(&state,ACTOR_EN_TITE,0,0,0,-1);
 CustomEnemies_Tick();assert(spawned==2);
 Actor vanilla;vanilla.id=1;vanilla.update=update;pool[1].next=&vanilla;
 cv["gEnhancements.CustomEnemies.NaturalEncounters"]=0;CustomEnemies_Tick();assert(killed==1&&vanilla.update==update);
 cv["gEnhancements.CustomEnemies.Randomizer"]=1;
 int16_t id=ACTOR_EN_TITE,params=-1;CustomEnemies_ReplaceEntry(&state,&id,&params,0,0,0);
#if NEI_MM
 assert(id==101&&params==0);
#else
 assert(id==ACTOR_EN_TITE&&params==-1); // OoT selection lives in existing randomizer
#endif
 assets=false;id=ACTOR_EN_TITE;params=-1;CustomEnemies_ReplaceEntry(&state,&id,&params,0,0,0);assert(id==ACTOR_EN_TITE&&params==-1);
 assets=true;
 CustomEnemies_Reset();CustomEnemies_RecordEntry(&state,ACTOR_EN_TITE,0,0,0,-1);
 cv["gEnhancements.CustomEnemies.NaturalEncounters"]=1;CustomEnemies_Tick();int before=spawned;
 CustomEnemies_ActorDestroyed(&pool[spawned-1]);pool[spawned-1].update=update;
 CustomEnemies_Tick();assert(spawned==before); // destroyed identity cannot alias a reused allocation
 id=999;CustomEnemies_ReplaceEntry(&state,&id,&params,0,0,0);assert(id==999);
}
'''

class NativeRuntime(unittest.TestCase):
    def test_spawn_lifecycle_and_replacement_fallback(self):
        for mm in (0,1):
            with self.subTest(mm=mm),tempfile.TemporaryDirectory() as tmp:
                p=Path(tmp)/'t.cpp';p.write_text(STUB);exe=Path(tmp)/'t'
                subprocess.run(['c++','-std=c++17','-DNEI_MM='+str(mm),'-I'+str(ROOT),str(p),'-o',str(exe)],check=True)
                subprocess.run([str(exe)],check=True)
if __name__=='__main__':unittest.main()
