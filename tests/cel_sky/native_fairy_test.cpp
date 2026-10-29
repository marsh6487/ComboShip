// Execute native fairy light production and update policy, preserving Midna,
// Black Market halo suppression and Unbound world coordinates.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
using s16=int16_t;using s32=int32_t;using u8=uint8_t;using u32=uint32_t;using f32=float;
#define CVAR_ENHANCEMENT(s) s
constexpr int FAIRY_NAVI=0,FAIRY_KOKIRI=1,FAIRY_HEAL=2,FAIRY_HEAL_BIG=3,FAIRY_HEAL_TIMED=4,FAIRY_FLAG_BIG=0x40;
constexpr int LIGHT_POINT_NOGLOW=0,LIGHT_POINT_GLOW=2;
struct Vec3f{float x=0,y=0,z=0;};
struct Actor{int params=0;struct{Vec3f pos;}world;Vec3f velocity;struct{struct{int y;}rot;}shape;Vec3f scale;};
struct LightInfo{int type=0;struct{struct{float x,y,z;u8 color[3];s16 radius;}point;}params;};
struct Player{Actor actor;u32 stateFlags2=0;};
struct PlayState{Player* player;};
struct EnElf{Actor actor;LightInfo lightInfoGlow{},lightInfoNoGlow{};struct{float r,g,b;}outerColor;int fairyFlags=0,unk_2A8=0,unk_2BC=0,timer=0;void(*actionFunc)(EnElf*,PlayState*)=nullptr;};
std::map<std::string,double>cvars;bool resourcePresent=false,quiet=false;
int CVarGetInteger(const char* k,int d){return cvars.contains(k)?cvars[k]:d;}
float CVarGetFloat(const char* k,float d){return cvars.contains(k)?cvars[k]:d;}
#define GET_PLAYER(p) ((p)->player)
bool ResourceMgr_FileExists(const char*){return resourcePresent;}
void* ResourceMgr_LoadGfxByName(const char*){return resourcePresent?(void*)1:nullptr;}
bool EnElf_BlackMarketQuiet(EnElf*,PlayState*){return quiet;}
void Lights_PointNoGlowSetInfo(LightInfo* i,f32 x,f32 y,f32 z,u8 r,u8 g,u8 b,s16 radius){i->type=LIGHT_POINT_NOGLOW;i->params.point={x,y,z,{r,g,b},radius};}
void Lights_PointGlowSetInfo(LightInfo* i,f32 x,f32 y,f32 z,u8 r,u8 g,u8 b,s16 radius){Lights_PointNoGlowSetInfo(i,x,y,z,r,g,b,radius);i->type=LIGHT_POINT_GLOW;}
int Math_Atan2S(float,float){return 0;}void Actor_SetScale(Actor*,float){}
void EnElf_UpdateMidnaBlink(EnElf*){}void func_80A04D90(EnElf*,PlayState*){}
/* NATIVE_FAIRY_LIGHTS */
/* NATIVE_FAIRY_UPDATE */
void Idle(EnElf*,PlayState*){}
void Require(bool value,const char* why){if(!value){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
int main(){
 Player player;player.actor.world.pos={50000.5f,40000.25f,-60000.75f};PlayState play{&player};EnElf fairy{};fairy.actor.params=FAIRY_NAVI;fairy.fairyFlags=0x20;fairy.outerColor={100,50,0};
 EnElf_UpdateLights(&fairy,&play);
 Require(fairy.lightInfoNoGlow.params.point.y==40060.25f,"fairy light keeps wide f32 world Y");
 Require(fairy.lightInfoGlow.params.point.color[0]==224&&fairy.lightInfoGlow.params.point.color[1]==214&&fairy.lightInfoGlow.params.point.color[2]==204,"Navi targeting tint reaches source light");
 resourcePresent=true;cvars["MidnaCompanion"]=1;EnElf_UpdateLights(&fairy,&play);Require(fairy.lightInfoGlow.type==LIGHT_POINT_NOGLOW,"Midna halo suppression survives cel tint");
 resourcePresent=false;quiet=true;player.stateFlags2=1u<<29;EnElf_UpdateLights(&fairy,&play);Require(fairy.lightInfoGlow.type==LIGHT_POINT_NOGLOW,"Black Market quiet halo suppression survives cel tint");
 quiet=false;fairy={};fairy.actor.params=FAIRY_HEAL;fairy.actionFunc=Idle;fairy.actor.world.pos={50000,45000,70000};cvars["Graphics.WorldLighting.OtherFairyLights"]=1;EnElf_Update(&fairy.actor,&play);Require(fairy.lightInfoNoGlow.params.point.radius==100&&fairy.lightInfoNoGlow.params.point.y==45000,"optional wild fairy source light");
 std::puts("cel native fairy source: PASS");
}
