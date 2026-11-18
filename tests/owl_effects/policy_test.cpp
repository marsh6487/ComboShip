#include "combo/actors/ComboOwlEffectPolicy.h"
#include <cassert>
#include <cmath>
#include <limits>
int main() {
    using namespace ComboOwlEffects;
    for (unsigned tick=0;tick<240;++tick) {
        assert(TextureFrame(tick)<60 && TextureFrame(tick)==TextureFrame(tick+120));
        auto page=Pages(tick); auto loop=Pages(tick+120);
        assert(page.count==1152 && page.count==loop.count);
        for(size_t i=0;i<page.count;++i) {
            const auto a=page.vertices[i], b=loop.vertices[i];
            assert(a.rgb==b.rgb && a.alpha==b.alpha);
            assert(std::abs(a.p.x*16)<32768 && std::abs(a.p.y*16)<32768 && std::abs(a.p.z*16)<32768);
            const auto source=ComboOwlPageGlow::kVertices[i];
            assert(std::abs(a.p.x*100-source.x)<.001f && a.alpha==source.alpha);
        }
    }
    auto left=Flame(false),right=Flame(true);
    assert(left.count==6 && right.count==6);
    assert(left.vertices[0].v==1 && left.vertices[2].v==0);
    for(size_t i=0;i<left.count;++i) {
        assert(std::abs(right.vertices[i].p.x-left.vertices[i].p.x-10.3f)<.001f);
        assert(left.vertices[i].alpha==255); // texture owns 65%, never squared
        assert(left.vertices[i].p.y>=40.5f && left.vertices[i].p.y<=51.5f);
    }
}
