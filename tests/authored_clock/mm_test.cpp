#define main BaselineRendererMain
#define gSPDisplayList ClockLegacyDisplayList
#define gSPSegment ClockLegacySegment
#include "../mm_nei/renderer_runtime_test.cpp"
#undef gSPSegment
#undef gSPDisplayList
#undef main
#define COMBO_BUILD
#include "combo/NeiClockGI.h"
#include "clock_mm_tile_lerp.inc"
#include "ComboCollectibleShimmer.h"
#include "clock_mm.inc"
#include "clock_mm_export.inc"
static int halfDays=0;
static void DrawClock(int,Actor*) { ++halfDays; }
#include "clock_mm_dispatch.inc"
#include "ComboSwordGiFit.h"
#define COMBO_DIN_SWORD_GI_HOST_MM
#include "ComboDinSwordGi.h"
#undef COMBO_DIN_SWORD_GI_HOST_MM
#include <unordered_set>
extern "C" int ResourceMgr_OTRSigCheck(char* path) { return !std::strncmp(path,"__OTR__",7); }
#include "clock_mm_displaylist.inc"
extern "C" void gSPSegment(void* packet, int segment, uintptr_t target) {
    __gSPSegment(packet, segment, target);
}
#define gSPDisplayList ClockNativeDisplayList
#include "clock_mm_foreign.inc"
#undef gSPDisplayList
#include "glass_checks.h"
static ComboForeignDrawInfoOOT activeForeign;
static const ComboForeignDrawInfoOOT* ComboResolveForeignDrawInfoOOT(RandoCheckId) { return &activeForeign; }
static void DrawOotNeiUltrahand() { assert(false); }
static void DrawOotNeiCaneOfSomaria(RandoItemId) { assert(false); }
extern "C" void GetItem_Draw(PlayState*, s16) { assert(false); }
namespace Rando {
void DrawResolvedItem(RandoItemId, RandoCheckId, Actor*) { assert(false); }
}
#include "clock_mm_compositor.inc"
int main() {
    PlayState play{}; GraphicsContext gfx{};
    alignas(16) static Gfx opa[0x6700],xlu[0x1000],overlay[0x800];
    play.state.gfxCtx=&gfx; gPlayState=&play;
    play.billboardMtxF.xx=play.billboardMtxF.yy=play.billboardMtxF.zz=1;
    for(bool effects:{false,true}) for(bool alt:{false,true}) for(auto [frame,phase]:ClockFrames) {
        auto reset=[&] {
            gfx.polyOpa.p=opa;gfx.polyOpa.d=std::end(opa);
            gfx.polyXlu.p=xlu;gfx.polyXlu.d=std::end(xlu);
            gfx.overlay.p=overlay;gfx.overlay.d=std::end(overlay);
            Matrix_Translate(7,8,9,MTXMODE_NEW); nativeDisplayLists.clear();
            itemEffects=effects;mmAltEnabled=ownerAltEnabled=alt;play.gameplayFrames=frame;
        };
        auto check=[&](const char* expected) {
            assert(matrices.empty() && current.xx==1 && current.yy==1 && current.zz==1 && current.xw==7 && current.yw==8 && current.zw==9);
            int models=0,vertices=0;
            for(auto [begin,end]:{std::pair(opa,gfx.polyOpa.p),std::pair(xlu,gfx.polyXlu.p)}) {
                int depth=0;bool grayscale=false;
                for(auto* p=begin;p<end;++p) {
                    auto op=p->words.w0>>24;
                    if(op==G_SETTILESIZE_LERP) { p+=4; continue; }
                    assert(!(op==G_MOVEWORD && ((p->words.w0>>16)&255)==G_MW_SEGMENT));
                    if(op==G_DL_OTR_FILEPATH && !strcmp(reinterpret_cast<const char*>(p->words.w1),expected)) ++models;
                    if(op==G_COMBO_RM_PUSH)++depth;
                    if(op==G_COMBO_RM_POP)--depth;
                    if(op==G_SETGRAYSCALE)grayscale=p->words.w1;
                    if(op==G_VTX)++vertices;
                    assert(depth>=0);
                }
                assert(depth==0 && !grayscale);
            }
            assert(models==1);
            assert((vertices>0)==effects);
        };
        reset(); halfDays=0;ClockDispatch(RI_OOT_CLOCK);assert(halfDays==0);
        check(NeiClockGi::Opaque);
        CheckGlassCommands(xlu,gfx.polyXlu.p,NeiClockGi::GlassTexture,NeiClockGi::Glass,phase,"mm");
        for(int id:{RI_TIME_DAY_1,RI_TIME_NIGHT_1,RI_TIME_DAY_2,RI_TIME_NIGHT_2,RI_TIME_DAY_3,RI_TIME_NIGHT_3}) ClockDispatch(id);
        assert(halfDays==6);
        reset();CwItemDrawInfo recipe{};assert(ClockExport(RI_OOT_CLOCK,&recipe));
        assert(recipe.itemShimmer==effects && recipe.stateDependent==2 && recipe.drawKind==CW_DRAW_KIND_CUSTOM_GI);
        assert(recipe.dlistCount==2 && recipe.xluStartIndex==1 && recipe.scale==NeiClockGi::Scale);
        assert(!strcmp(recipe.dlists[0],NeiClockGi::OpaqueMM));
        assert(!strcmp(recipe.dlists[1],NeiClockGi::GlassMM));
        // The opposite host's exported Clock is consumed through the existing
        // native MM foreign custom drawer, not its old Stone of Agony recipe.
        CwItemDrawInfo imported{};
        NeiClockGi::Describe(&imported,NeiClockGi::Opaque,effects);
        activeForeign={};
        assert(ClockRoute(imported,activeForeign)==ComboForeignResolveOOT::Ok);
        assert(!strcmp(activeForeign.dls[0],"__OTR__@oot:objects/object_nei_clock/gNeiClockDL"));
        assert(!strcmp(activeForeign.dls[1],NeiClockGi::GlassOOT));
        MM_DrawComboForeign(static_cast<RandoCheckId>(1));
        check(activeForeign.dls[0]);
        CheckGlassCommands(xlu,gfx.polyXlu.p,NeiClockGi::GlassTextureOOT,NeiClockGi::GlassOOT,phase);

        // The new surface must not hide the accepted opaque body when only the
        // former draw budget fits. No texture/scroll may be left without a lens.
        if(!effects && !alt && frame==0) {
            reset();gfx.polyXlu.d=xlu+16;ClockDispatch(RI_OOT_CLOCK);
            check(NeiClockGi::Opaque);CheckNoGlassCommands(xlu,gfx.polyXlu.p);
            assert(gfx.polyXlu.p<=gfx.polyXlu.d && gfx.polyOpa.p<=gfx.polyOpa.d);
            reset();gfx.polyXlu.d=xlu+32;MM_DrawComboForeign(static_cast<RandoCheckId>(1));
            check(activeForeign.dls[0]);CheckNoGlassCommands(xlu,gfx.polyXlu.p);
            assert(gfx.polyXlu.p<=gfx.polyXlu.d && gfx.polyOpa.p<=gfx.polyOpa.d);
            reset();gfx.polyOpa.d=opa;ClockDispatch(RI_OOT_CLOCK);
            assert(gfx.polyOpa.p==opa && gfx.polyXlu.p==xlu);
            reset();ComboForeignDrawInfoOOT unrelated{};
            unrelated.count=2;unrelated.xluStart=1;
            unrelated.dls[0]="__OTR__@oot:objects/unrelated/BodyDL";
            unrelated.dls[1]="__OTR__@oot:objects/unrelated/SurfaceDL";
            MM_DrawForeignCustomGi(&unrelated);
            check(unrelated.dls[0]);CheckNoGlassCommands(xlu,gfx.polyXlu.p);
            int surfaces=0;
            for(auto* p=xlu;p<gfx.polyXlu.p;++p)
                if(p->words.w0>>24==G_DL_OTR_FILEPATH &&
                   !std::strcmp(reinterpret_cast<const char*>(p->words.w1),unrelated.dls[1])) ++surfaces;
            assert(surfaces==1);
        }
    }
    std::cout << "PASS MM actual alias dispatch/export/full foreign compositor: intrinsic glass/scroll/owners, six protected half-days, shimmer toggle, Alt, frame wrap, tight arena body fallback, state cleanup\n";
}
