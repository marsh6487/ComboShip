#include <cassert>
#include <cstring>
#include <vector>
#include "global.h"
enum { PAK_SOURCE_O2R = 2 };
struct PakModel {
    int source = 0;
    FlexSkeletonHeader* o2rAdultSkel = nullptr;
    FlexSkeletonHeader* o2rChildSkel = nullptr;
    char o2rAdultSkelOtr[128]{};
    char o2rChildSkelOtr[128]{};
};
static std::vector<PakModel> sModels;
static s32 active = -1;
static s32 sGetActiveIndex() { return active; }
#include "pak_query.inc"
int main() {
    void* adult[2]{}; void* child[2]{}; void* unrelated[2]{};
    FlexSkeletonHeader a{}, c{}; a.sh.segment = adult; c.sh.segment = child;
    assert(!PakLoader_GetSkinSkeletonPath(adult));
    sModels.resize(1); active = 0;
    auto& model = sModels[0];
    model.o2rAdultSkel = &a; model.o2rChildSkel = &c;
    std::strcpy(model.o2rAdultSkelOtr, "__OTR__objects/din/adult/Skel");
    std::strcpy(model.o2rChildSkelOtr, "__OTR__objects/din/child/Skel");
    assert(!PakLoader_GetSkinSkeletonPath(adult)); // raw PAK never inherits a sidecar
    model.source = PAK_SOURCE_O2R;
    assert(PakLoader_GetSkinSkeletonPath(adult) == model.o2rAdultSkelOtr);
    assert(PakLoader_GetSkinSkeletonPath(child) == model.o2rChildSkelOtr);
    assert(!PakLoader_GetSkinSkeletonPath(unrelated));
    assert(!PakLoader_GetSkinSkeletonPath(nullptr));
    active = 1; assert(!PakLoader_GetSkinSkeletonPath(adult));
}
