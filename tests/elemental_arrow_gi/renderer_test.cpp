#define NEI_GI_FIXTURE_BOUNDARY_ONLY
#include "../nei_gi/presentation_test.cpp"
#if __has_include("ComboElementalArrowGi.h")
#include "ComboElementalArrowGi.h"
int main() {
    using namespace Fixture;
    // A lost restore, missing optional archive, duplicate shimmer, or graphics
    // arena overrun fails this real renderer/GBI boundary test.
    for (int profile = 1; profile <= 3; ++profile) for (bool texture : {false,true}) {
        for (int assets : {0,1}) for (int toggle : {0,1}) {
            Reset(); alt=assets; enabled=toggle;
            if(texture) files.insert(NeiArrowGi::Texture(profile));
            NeiGi_DrawElementalArrow(&play,profile);
            assert(arena.size() == 3 && !arena[0].empty() && !arena[1].empty() && !arena[2].empty());
            assert(stack.empty() && matrix==1 && matrixY==0);
            assert(gfx.polyOpa.p<=gfx.polyOpa.d && gfx.polyXlu.p<=gfx.polyXlu.d);
            const auto* p=xlu; int depth=0;
            for(;p<gfx.polyXlu.p;++p) {
                if((p->words.w0>>24)==G_COMBO_RM_PUSH) ++depth;
                if((p->words.w0>>24)==G_COMBO_RM_POP) --depth;
                assert(depth>=0);
            }
            assert(depth==0);
        }
    }
    for(int invalid : {-1,0,4,100}) {
        Reset(); NeiGi_DrawElementalArrow(&play,invalid);
        assert(arena.empty() && allocations==0 && stack.empty());
    }
    for(int space=0;space<32;++space) {
        Reset(); gfx.polyOpa.d=opa+space;
        NeiGi_DrawElementalArrow(&play,1);
        assert(allocations==0 && arena.empty() && gfx.polyXlu.p==xlu && stack.empty());
    }
    std::cout << "PASS real elemental-arrow renderer: all profiles, Alt/toggle parity, missing-texture fallback, arena and owner restore\n";
}
#else
int main() { assert(false && "Elemental arrow renderer has not been implemented"); }
#endif
