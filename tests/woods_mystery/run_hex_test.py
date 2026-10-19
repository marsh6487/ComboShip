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
            assert(std::abs(v.x)<112 && v.y>=-12 && v.y<200 && std::abs(v.z)<30);
            assert(v.g>v.r && v.g>v.b);
        }
        for(int h=0;h<kHexes;++h) {
            float centerX=0,centerY=0;
            for(int i=0;i<6;++i) {centerX+=a[h*12+i].x;centerY+=a[h*12+i].y;}
            centerX/=6; centerY/=6;
            float radius=std::hypot(a[h*12].x-centerX,a[h*12].y-centerY);
            for(int i=0;i<6;++i) {
                const auto& o=a[h*12+i];const auto& in=a[h*12+6+i];
                assert(std::abs(std::hypot(o.x-centerX,o.y-centerY)-radius)<.001);
                assert(std::hypot(in.x-centerX,in.y-centerY)<radius*.8);
            }
        }
    }
    std::puts("PASS: animated green six-sided hollow particles; deterministic paused frame; bounded finite vertices");
}
'''
with tempfile.TemporaryDirectory() as td:
    path=Path(td);(path/'test.cpp').write_text(source)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',
      '-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie','-I'+str(root),str(path/'test.cpp'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
