"""Execute the production hex geometry: real hollow hexagons, green, bounded, animated."""
from pathlib import Path
import os, subprocess, tempfile

root=Path(__file__).resolve().parents[2]
header=root/'mm/2s2h/Enhancements/Graphics/WoodsMysteryHexEffect.h'
assert header.exists(), 'The Woods of Mystery green hex effect is missing'
source=r'''
#include "mm/2s2h/Enhancements/Graphics/WoodsMysteryHexEffect.h"
#include <cassert>
#include <cstring>
#include <cstdio>
int main() {
    using namespace WoodsMysteryHexEffect;
    std::array<Vertex,kVertexCount> a{},b{};
    Build(0,a); Build(30,b);
    assert(std::memcmp(a.data(),b.data(),sizeof(a))!=0);
    Build(0,b); assert(std::memcmp(a.data(),b.data(),sizeof(a))==0);
    for(unsigned frame : {0u,1u,30u,1000u,100000u,0xffffffffu}) {
        Build(frame,a);
        for(const auto& v:a) {
            assert(std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z));
            assert(std::abs(v.x)<78 && v.y>=-4 && v.y<140 && v.z>-62 && v.z<15);
            assert(v.g>v.r && v.g>v.b);
            assert(v.a<=215);
        }
        for(int h=0;h<kHexes;++h) {
            float centerX=0,centerY=0;
            for(int i=0;i<6;++i) {centerX+=a[h*12+i].x;centerY+=a[h*12+i].y;}
            centerX/=6; centerY/=6;
            assert(std::abs(centerX)>42 || centerY>104); // The center stays open.
            // Accepted outer wood rim, in mouth-local coordinates. Particles
            // follow its curved depth rather than a broad plane above it.
            constexpr float rim[][3]={{-50.205f,1,-57.276f},{-61.518f,15,-57.276f},
                {-66.468f,48,-45.255f},{-60.811f,80,-32.527f},{-46.669f,102,-16.971f},
                {-28.284f,120,-4.243f},{0,128,0},{29.698f,120,-4.243f},{50.912f,104,-16.971f},
                {60.811f,80,-32.527f},{66.468f,48,-45.255f},{61.518f,15,-57.276f},{50.205f,1,-57.276f}};
            float nearest=10000;
            for(int segment=0;segment<12;++segment) {
                const auto& p=rim[segment];const auto& q=rim[segment+1];
                float dx=q[0]-p[0],dy=q[1]-p[1],dz=q[2]-p[2];
                float x=centerX-p[0],y=centerY-p[1],z=a[h*12].z-p[2];
                float t=(x*dx+y*dy+z*dz)/(dx*dx+dy*dy+dz*dz);
                t=std::fmax(0.0f,std::fmin(1.0f,t));
                nearest=std::fmin(nearest,std::sqrt((x-t*dx)*(x-t*dx)+(y-t*dy)*(y-t*dy)+(z-t*dz)*(z-t*dz)));
            }
            assert(nearest<13);
            float radius=std::hypot(a[h*12].x-centerX,a[h*12].y-centerY);
            for(int i=0;i<6;++i) {
                const auto& o=a[h*12+i];const auto& in=a[h*12+6+i];
                assert(std::abs(std::hypot(o.x-centerX,o.y-centerY)-radius)<.001);
                assert(std::hypot(in.x-centerX,in.y-centerY)<radius*.8);
            }
        }
    }
    assert(kHexes<=36);
    std::puts("PASS: curated green hollow hexes track the accepted 3D wood rim, leave its center open, animate within bounds and freeze with gameplay time");
}
'''
with tempfile.TemporaryDirectory() as td:
    path=Path(td);(path/'test.cpp').write_text(source)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',
      '-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie','-I'+str(root),str(path/'test.cpp'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
