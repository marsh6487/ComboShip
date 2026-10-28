// The native light setter must apply the actual policy seam before publishing
// source RGB, preserving its opt-out and independent radius/position fields.
#include <cstdint>
#include <cstdio>
using u8=uint8_t;using s16=int16_t;
#define CVAR_ENHANCEMENT(s) s
struct LightInfo{struct{struct{float x=50000,y=40000,z=60000;u8 color[3]{};s16 radius=0;}point;}params;};
bool enabled=true;
int CVarGetInteger(const char*,int){return enabled;}
void WorldLighting_ApplyFlameFlicker(LightInfo*,u8* r,u8* g,u8* b){*r=42;*g=43;*b=44;}
/* NATIVE_LIGHT_SETTER */
int main(){LightInfo info;Lights_PointSetColorAndRadius(&info,10,11,12,200);if(info.params.point.color[0]!=42||info.params.point.color[2]!=44||info.params.point.radius!=200||info.params.point.y!=40000){std::fputs("FAIL: native source flicker seam missing or corrupts light state\n",stderr);return 1;}enabled=false;Lights_PointSetColorAndRadius(&info,10,11,12,300);if(info.params.point.color[0]!=10||info.params.point.color[2]!=12||info.params.point.radius!=300){std::fputs("FAIL: native source flicker opt-out lost\n",stderr);return 1;}std::puts("cel native light setter: PASS");}
