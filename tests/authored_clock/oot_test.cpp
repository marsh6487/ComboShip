#define COMBO_BUILD
#include "soh/Enhancements/item-tables/ItemTableTypes.h"
#undef COMBO_BUILD
#define NEI_GI_FIXTURE_BOUNDARY_ONLY
#include "../nei_gi/presentation_test.cpp"
// The fixture above builds the original renderer; these ABI-only production
// helpers are also compiled below with the combined header surface enabled.
#define COMBO_BUILD
#include "combo/NeiClockGI.h"
#include "ComboNeiGiResourceOwner.h"
#include "clock_oot_tile_lerp.inc"
#include "ComboCollectibleShimmer.h"
extern "C" void Randomizer_DrawTrueMasterSwordFlame(PlayState*) { assert(false); }
extern "C" void Randomizer_DrawCaneSomariaUpgradeFlame(PlayState*) { assert(false); }
#include "clock_oot.inc"
#include "clock_oot_export.inc"
#include "clock_simple.inc"
#include "objects/object_gi_map/object_gi_map.h"
// Exact foreign export arm from e22bcb222dcd06d18567cbbb47087d3d5979be96.
static int BaselineClockExport(int rg,CwItemDrawInfo* out) {
    switch(rg) {
        case RG_CLOCK:
            return CwSimple(out, gGiStoneOfAgonyDL, false, 0.035f);
        default: return 0;
    }
}
static void OOT_DrawForeignWeaponFlame(PlayState*,const uint8_t*) {}
#include <unordered_set>
#include "clock_oot_foreign.inc"
#include "glass_checks.h"
static ComboForeignDrawInfo activeForeign;
static const ComboForeignDrawInfo* ComboResolveForeignDrawInfo(RandomizerCheck) { return &activeForeign; }
static RandomizerCheck OOT_GetQueuedDrawCheck() { assert(false); return RC_UNKNOWN_CHECK; }
extern "C" void gSPSegment(void* packet, int segment, uintptr_t target) {
    __gSPSegment(packet, segment, target);
}
#include "clock_oot_compositor.inc"
static void CheckState() {
    using namespace Fixture;
    assert(stack.empty() && matrix==1 && matrixY==0 && matrixYaw==0);
    for (auto [begin,end] : {std::pair(opa,gfx.polyOpa.p),std::pair(xlu,gfx.polyXlu.p)}) {
        int depth=0; bool grayscale=false;
        for (auto* p=begin;p<end;++p) {
            unsigned op=p->words.w0>>24;
            if(op==G_SETTILESIZE_LERP) { p+=4; continue; }
            assert(!(op==G_MOVEWORD && ((p->words.w0>>16)&255)==G_MW_SEGMENT));
            if(op==G_COMBO_RM_PUSH) ++depth;
            if(op==G_COMBO_RM_POP) --depth;
            if(op==G_SETGRAYSCALE) grayscale=p->words.w1;
            assert(depth>=0);
        }
        assert(depth==0 && !grayscale);
    }
}
int main() {
    using namespace Fixture;
    CwItemDrawInfo baseline{};
    assert(BaselineClockExport(RG_CLOCK,&baseline));
    assert(baseline.drawKind==CW_DRAW_KIND_SIMPLE && baseline.scale==.035f);
    assert(!strcmp(baseline.dlists[0],gGiStoneOfAgonyDL));
    assert(strcmp(baseline.dlists[0],NeiClockGi::Opaque));
    std::cout << "PASS baseline reproduction: foreign Clock exported Stone of Agony at .035\n";
    for(int effects:{0,1}) for(int assets:{0,1}) for(auto [frame, phase]:ClockFrames) {
        Reset(); enabled=effects; alt=assets; play.gameplayFrames=frame;
        GetItemEntry entry{}; entry.getItemId=RG_CLOCK;
        Randomizer_DrawClock(&play,&entry);
        const std::vector<std::string> expectedNative = {
            NeiClockGi::Opaque,
            "__OTR__objects/object_nei_clock/gNeiClockGlassTexDL",
            "__OTR__objects/object_nei_clock/gNeiClockGlassDL",
        };
        assert(Drawn()==expectedNative && "Clock glass must draw even with ambient ItemEffects disabled");
        assert(arena.size()==size_t(effects));
        assert(std::abs(submitted.front().first-NeiClockGi::Scale)<1e-6f);
        CheckGlassCommands(xlu,gfx.polyXlu.p,NeiClockGi::GlassTexture,NeiClockGi::Glass,phase,"oot");
        CheckState();
        Reset(); enabled=effects; alt=assets; play.gameplayFrames=frame;
        CwItemDrawInfo recipe{};
        assert(ClockExport(RG_CLOCK,&recipe));
        assert(recipe.drawKind==CW_DRAW_KIND_CUSTOM_GI && recipe.dlistCount==2 && recipe.xluStartIndex==1);
        assert(recipe.itemShimmer==effects && recipe.stateDependent==2);
        assert(recipe.neiShimmer==int(NeiGi::Kind::Hourglass)+1);
        assert(!strcmp(recipe.dlists[0],NeiClockGi::Opaque));
        assert(!strcmp(recipe.dlists[1],NeiClockGi::Glass));
        // Consume a native-import descriptor in OoT as the actual foreign helper
        // does. The MM alias uses its own identical model under an explicit owner.
        CwItemDrawInfo imported{};
        NeiClockGi::Describe(&imported,NeiClockGi::OpaqueMM,effects);
        activeForeign={};
        assert(ClockRoute(imported,activeForeign)==ComboForeignResolve::Ok);
        assert(!strcmp(activeForeign.dls[0],NeiClockGi::OpaqueMM));
        assert(!strcmp(activeForeign.dls[1],NeiClockGi::GlassMM));
        entry.comboForeignCheck=1;
        OOT_DrawComboForeign(&play,&entry);
        const std::vector<std::string> expectedForeign={NeiClockGi::OpaqueMM,NeiClockGi::GlassTextureMM,NeiClockGi::GlassMM};
        assert(Drawn()==expectedForeign);
        assert(arena.size()==size_t(effects));
        CheckGlassCommands(xlu,gfx.polyXlu.p,NeiClockGi::GlassTextureMM,NeiClockGi::GlassMM,phase);
        CheckState();
    }
    for(int slots:{16,24}) {
        Reset();gfx.polyXlu.d=xlu+slots;
        GetItemEntry entry{}; Randomizer_DrawClock(&play,&entry);
        assert(Drawn()==std::vector<std::string>{NeiClockGi::Opaque});
        CheckNoGlassCommands(xlu,gfx.polyXlu.p);CheckState();
        assert(gfx.polyXlu.p<=gfx.polyXlu.d && gfx.polyOpa.p<=gfx.polyOpa.d);
    }
    Reset();gfx.polyXlu.d=xlu+32;activeForeign.itemShimmer=0;
    GetItemEntry entry{};entry.comboForeignCheck=1;OOT_DrawComboForeign(&play,&entry);
    assert(Drawn()==std::vector<std::string>{NeiClockGi::OpaqueMM});
    CheckNoGlassCommands(xlu,gfx.polyXlu.p);CheckState();
    assert(gfx.polyXlu.p<=gfx.polyXlu.d && gfx.polyOpa.p<=gfx.polyOpa.d);
    Reset();gfx.polyOpa.d=opa;Randomizer_DrawClock(&play,&entry);
    assert(Drawn().empty() && gfx.polyOpa.p==opa && gfx.polyXlu.p==xlu);
    Reset();auto* before=gfx.polyXlu.p;
    NeiClockGi::PrepareGlass(gfx.polyXlu.p,42,"__OTR__@bad:objects/object_nei_clock/gNeiClockGlassDL");
    assert(gfx.polyXlu.p==before);
    Reset();ComboForeignDrawInfo unrelated{};
    unrelated.count=2;unrelated.xluStart=1;
    unrelated.dls[0]="__OTR__@mm:objects/unrelated/BodyDL";
    unrelated.dls[1]="__OTR__@mm:objects/unrelated/SurfaceDL";
    OOT_DrawForeignCustomGi(&play,&unrelated);
    const std::vector<std::string> unrelatedPaths={unrelated.dls[0],unrelated.dls[1]};
    assert(Drawn()==unrelatedPaths);
    CheckNoGlassCommands(xlu,gfx.polyXlu.p);CheckState();
    std::cout << "PASS OoT actual Clock callback and full foreign compositor: intrinsic glass/scroll/owners, gold shimmer toggle, Alt, frame wrap, tight arena body fallback, state cleanup\n";
}
