// Execute the native Play_Draw sky phase, including current host timing calls.
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
struct GraphicsContext {};
struct Vec3f {float x,y,z;};
struct PlayState {int skyboxId=1;struct {bool skyboxDisabled=false,sunMoonDisabled=false;int skyboxBlend=0;} envCtx;struct{int unk_140=0;} skyboxCtx;struct{Vec3f eye{};}view;};
constexpr int SKYBOX_NORMAL_SKY=1,SKYBOX_UNSET_1D=29,SKYBOX_CUTSCENE_MAP=2;
int hregs[100]{};
#define HREG(i) hregs[i]
std::vector<std::string> events;
int FrameTiming_BeginSpan(){return 0;}
void FrameTiming_EndNamedSpan(const char*,const char*,int){}
template<class... Args>void Environment_UpdateSkybox(Args...){}
template<class... Args>void SkyboxDraw_Draw(Args...){events.push_back("skybox");}
void Environment_DrawSunAndMoon(PlayState*){events.push_back("sun");}
void GameInteractor_ExecuteOnPlayDrawSkyGradient(void*){events.push_back("gradient");}
void GameInteractor_ExecuteOnPlayDrawSky(void*){events.push_back("stars");}
void GameInteractor_ExecuteOnPlayDrawSkyClouds(void*){events.push_back("clouds");}
void DrawNativeSkyPhase(PlayState* play){GraphicsContext context,*gfxCtx=&context;int drawPhase=0;/* NATIVE_SKY_PHASE */}
int main(){PlayState play;DrawNativeSkyPhase(&play);if(events!=std::vector<std::string>{"skybox","gradient","stars","clouds","sun"}){std::fputs("FAIL: native sky phase does not bracket WW layers between skybox and sun\n",stderr);return 1;}std::puts("cel native sky phase: PASS");}
