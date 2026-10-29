// Run the complete native Actor_DrawAll body. Catch duplicate receiver draw/SFX,
// stale shadow capture, lens/lantern bracket loss, or altered host culling.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>
using s32=int32_t; using u8=uint8_t;
struct Vec3f { float x=0,y=0,z=0; };
struct Gfx {};
struct GraphicsContext { Gfx* opa; Gfx* xlu; };
struct Actor { int id=0; int flags=0; int sfx=0; int room=0; bool isDrawn=false; bool visible=true; bool receiver=false; Actor* next=nullptr; void* init=nullptr; void* draw=(void*)1; struct {Vec3f pos;} world; Vec3f projectedPos; float projectedW=1; };
struct ActorListEntry { Actor* head=nullptr; };
struct ActorContext { ActorListEntry actorLists[3]; bool lensActive=false,lensFromLantern=false; int titleCtx=0; };
struct PlayState { struct {GraphicsContext* gfxCtx;} state; ActorContext actorCtx; struct { struct {int lensMode=0,num=0;} curRoom; } roomCtx; struct {int state=0;} csCtx; int viewProjectionMtxF=0,colChkCtx=0; };
struct FrameTimingSpan { int epoch=1; };
constexpr int INVISIBLE_ACTOR_MAX=64, ACTOR_PLAYER=10;
constexpr int ACTOR_FLAG_DRAW_CULLING_DISABLED=1, ACTOR_FLAG_INSIDE_CULLING_VOLUME=2, ACTOR_FLAG_REACT_TO_LENS=4;
constexpr int LENS_MODE_HIDE_ACTORS=1,CS_STATE_IDLE=0;
int hregs[100]{},iregs[100]{}; bool cel=false,shadows=false,transformed=true;
std::vector<std::string> events; std::map<int,int> draws,audio; std::map<std::string,uint64_t> metrics;
Gfx opa[256],xlu[256];
#define HREG(i) hregs[i]
#define IREG(i) iregs[i]
#define ARRAY_COUNT(a) int(sizeof(a)/sizeof((a)[0]))
#define CVAR_ENHANCEMENT(s) s
#define OPEN_DISPS(ctx) auto* testGfx=(ctx)
#define CLOSE_DISPS(ctx) ((void)(ctx))
#define POLY_OPA_DISP testGfx->opa
#define POLY_XLU_DISP testGfx->xlu
#define gDPNoOpString(pkt,name,i) ((void)(pkt),(void)(name),(void)(i))
void Toon(Gfx* pkt,bool enabled) { if (pkt>=opa && pkt<opa+256) events.push_back(enabled?"T+":"T-"); }
#define gSPToon(pkt,state) Toon(pkt,state)
#define gSPToonShadowFlush(pkt) ((void)(pkt),events.push_back("F"))
#define gSPToonShadow(pkt,a,b,c,d) ((void)(pkt),events.push_back("X"))
FrameTimingSpan FrameTiming_BeginSpan(){return {};}
void FrameTiming_EndNamedSpan(const char*,const char*,FrameTimingSpan){}
void FrameTiming_Count(const char* name,uint64_t count){metrics[name]=count;}
struct { char* name; } actorEntry{(char*)"actor"};
auto ActorDB_Retrieve(int){return &actorEntry;}
int CVarGetInteger(const char*,int){return 0;}
void SkinMatrix_Vec3fMtxFMultXYZW(int*,Vec3f* p,Vec3f* q,float* w){*q=*p;*w=1;}
u8 TransformMasks_IsTransformed(){return transformed;}
void Actor_UpdateFlaggedAudio(Actor* a){audio[a->id]++;}
void Ship_CalcShouldDrawAndUpdate(PlayState*,Actor*,Vec3f*,float,bool* draw,bool* update){*draw=false;*update=false;}
bool Actor_CullingCheck(PlayState*,Actor* a){return a->visible;}
void Actor_Draw(PlayState*,Actor* a){draws[a->id]++;events.push_back("D"+std::to_string(a->id));}
int ToonLighting_CelEnabled(){return cel;} int ToonLighting_ShadowsEnabled(){return shadows;}
int ToonLighting_IsShadowReceiver(Actor* a){return a->receiver;}
void GameInteractor_ExecuteOnPlayDrawWorldLights(void*){events.push_back("W");}
void Effect_DrawAll(GraphicsContext*){events.push_back("E");}
void EffectSs_DrawAll(PlayState*){events.push_back("S");}
void ToonLighting_LensBracketBegin(GraphicsContext*){events.push_back("B");}
void ToonLighting_LensBracketEnd(GraphicsContext*){events.push_back("Z");}
void Actor_DrawLensActors(PlayState*,int count,Actor** actors){for(int i=0;i<count;i++){draws[actors[i]->id]++;events.push_back("L"+std::to_string(actors[i]->id));}}
bool Player_InCsMode(PlayState*){return false;} void Actor_DisableLens(PlayState*){}
void DinFireSword_FinishDrawAll(PlayState*){} void Actor_DrawFaroresWindPointer(PlayState*){}
void Lights_DrawGlow(PlayState*){} void TitleCard_Draw(PlayState*,int*){}
void CollisionCheck_DrawCollision(PlayState*,int*){}
/* NATIVE_ACTOR_DRAW_ALL */
static int Position(const char* event){for(size_t i=0;i<events.size();i++)if(events[i]==event)return int(i);return -1;}
static void Require(bool value,const char* why){if(!value){std::fprintf(stderr,"FAIL: %s\n",why);for(auto& e:events)std::fprintf(stderr,"%s ",e.c_str());std::fputs("\n",stderr);std::exit(1);}}
int main(){
 for(int mask=0;mask<4;mask++){
  cel=mask&1;shadows=mask&2;events.clear();draws.clear();audio.clear();metrics.clear();
  GraphicsContext gfx{opa,xlu};PlayState play{};play.state.gfxCtx=&gfx;play.actorCtx.lensFromLantern=true;
  Actor player{},floor{},lens{},culled{};
  player.id=10;player.sfx=0x1801;player.next=&lens;
  lens.id=30;lens.flags=ACTOR_FLAG_REACT_TO_LENS;lens.sfx=123;
  floor.id=20;floor.receiver=true;floor.sfx=123;
  culled.id=40;culled.visible=false;
  play.actorCtx.actorLists[0].head=&player;play.actorCtx.actorLists[1].head=&floor;play.actorCtx.actorLists[2].head=&culled;
  Actor_DrawAll(&play,&play.actorCtx);
  Require(draws[10]==1&&draws[20]==1&&draws[30]==1&&draws[40]==0,"actors/lens/receiver draw exactly once; culled actor stays culled");
  Require(audio[10]==0&&audio[20]==1&&audio[30]==1,"transformed player SFX suppression and single receiver SFX retained");
  Require(metrics["actors_draw_visited"]==4&&metrics["actors_draw_culled"]==1&&metrics["actors_lens_queued"]==1,"host traversal/culling/lens metrics retained");
  Require(Position("W")>=0&&Position("W")<Position("D10"),"world lights run before nonreceiver actor");
  if(shadows)Require(Position("D20")<Position("W")&&Position("W")<Position("F")&&Position("F")<Position("D10"),"receiver depth precedes world pools and shadow flush");
  else Require(Position("F")==-1&&Position("D10")<Position("D20"),"shadow-off path keeps native order without flush");
  Require(Position("B")<Position("L30")&&Position("L30")<Position("Z"),"NEI lantern-only lens pass has balanced cel brackets");
  if(cel)Require(Position("T+")>=0&&Position("T-")>Position("D10")&&Position("T-")<Position("E"),"toon actor bracket closes before effects");
  if(shadows&&!cel)Require(Position("X")>Position("D10")&&Position("X")<Position("E"),"shadow-only final capture disarms before effects");
 }
 std::puts("cel native actor draw: PASS");
}
