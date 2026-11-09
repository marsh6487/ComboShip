// Engine allocation/interpolation/resource services are the boundary. Matrix
// math, display-list bridge, setup presets and Graph open/close are production.
#include "mods/nei_save.h"
#include "mods/extended_inventory.h"
#include "mods/combo_rpg.h"
#include "Rando/MiscBehavior/ClockShuffle.h"
#include "Rando/MiscBehavior/MiscBehavior.h"
#include "2s2h/BenPort.h"
static MtxF csmcMatrixStack[20];
MtxF* sCurrentMatrix=csmcMatrixStack;
static int csmcMatrixDepth,csmcFrameDepth,csmcResources,csmcCategoryQueries;
static std::vector<MtxF> csmcMatrices;
static Mtx csmcMatrixStorage[1024];
static Gfx csmcCircleResource[8];
static std::set<RandoItemId> csmcUnobtainable;
#ifdef COMBO_BUILD
static std::map<RandoCheckId,ComboRando::ForeignItem> csmcForeign;
const ComboRando::ForeignItem* Rando::MiscBehavior::MM_LookupForeign(RandoCheckId check){
 ++csmcCategoryQueries;auto it=csmcForeign.find(check);return it==csmcForeign.end()?nullptr:&it->second;
}
#endif
extern "C" {
void FrameInterpolation_RecordOpenChild(const void*,int){++csmcFrameDepth;}
void FrameInterpolation_RecordCloseChild(){REQUIRE(csmcFrameDepth>0);--csmcFrameDepth;}
void FrameInterpolation_RecordMatrixPush(){REQUIRE(++csmcMatrixDepth<20);}
void FrameInterpolation_RecordMatrixPop(){REQUIRE(csmcMatrixDepth>0);--csmcMatrixDepth;}
void FrameInterpolation_RecordMatrixTranslate(f32,f32,f32,u8){}
void FrameInterpolation_RecordMatrixScale(f32,f32,f32,u8){}
void FrameInterpolation_RecordMatrixReplaceRotation(MtxF*){}
Mtx* Matrix_Finalize(GraphicsContext*){
 REQUIRE(csmcMatrices.size()<1024);csmcMatrices.push_back(*sCurrentMatrix);return &csmcMatrixStorage[csmcMatrices.size()-1];
}
int ResourceMgr_OTRSigCheck(char* value){return value && !memcmp(value,"__OTR__",7);}
Gfx* ResourceMgr_LoadGfxByName(const char* path){
 REQUIRE(!strcmp(path,"__OTR__objects/gameplay_keep/gLensFlareCircleDL"));++csmcResources;return csmcCircleResource;
}
NeiSaveData* Nei_Save(){return &gSaveContext.save.shipSaveInfo.nei;}
u16 Nei_GetOwnedItem(u8 slot){return gSaveContext.save.shipSaveInfo.nei.ownedItems[slot-24];}
u8 Nei_StrengthLevel(){std::abort();}
s32 Inventory_HasEmptyBottle(){std::abort();}
}
// Other progressive chains are outside the marker fixture; trap if accidentally
// used rather than silently providing a fabricated tier.
static RandoItemId ResolveCanePresentation(){std::abort();}
bool Rando::IsItemObtainable(RandoItemId item,RandoCheckId){return !csmcUnobtainable.contains(item);}
#include "environment_csmc_native_graphics.inc"
#include "environment_csmc_conversion.inc"
