#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "combo/menu/ComboItemDrawABI.h"
#include "soh/mods/mm_sources/objects/object_gi_masks_all.h"
#include "soh/mods/mm_sources/objects/object_mm_rando_items.h"
#include "z64item.h"
#define RANDO_ENUM_BEGIN(x) enum x {
#define RANDO_ENUM_ITEM(x) x,
#define RANDO_ENUM_END(x) };
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
using s32=int32_t;using f32=float;
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
/* MASK_TABLE */
constexpr int TABLE_RANDOMIZER=1;
void Randomizer_DrawMmMask() {}
void Randomizer_DrawMmRemains() {}
void Randomizer_DrawCaneOfSomaria() {}
void Randomizer_DrawCanePacci() {}
void Randomizer_DrawCaneSomariaUpgrade() {}
void Randomizer_DrawCanePacciUpgrade() {}
void Randomizer_DrawCanePacciUltrahand() {}
struct GetItemEntry {int tableId,drawItemId,gid,itemId;void (*drawFunc)();};
namespace Rando::StaticData {
struct Name {std::string english="mask";};
struct Item {
 RandomizerGet rg;
 std::shared_ptr<GetItemEntry> GetGIEntry(RandomizerGet*) {
  if(rg>=RG_MM_REMAINS_ODOLWA && rg<=RG_MM_REMAINS_TWINMOLD)
   return std::make_shared<GetItemEntry>(GetItemEntry{TABLE_RANDOMIZER,rg,0,rg,Randomizer_DrawMmRemains});
  bool native=rg==RG_MM_MASK_GORON || rg==RG_MM_MASK_ZORA || rg==RG_MM_MASK_KEATON || rg==RG_MM_MASK_BUNNY || rg==RG_MM_MASK_TRUTH;
  return std::make_shared<GetItemEntry>(GetItemEntry{TABLE_RANDOMIZER,rg,native?40:0,ITEM_MM_MASK_POSTMAN+(int)rg-(int)RG_MM_MASK_POSTMAN,native?nullptr:Randomizer_DrawMmMask});
 }
 const Name& GetName() {static Name name;return name;}
};
Item RetrieveItem(RandomizerGet rg) {return {rg};}
}
int nativeRows=0;
int NeiGi_DescribeEntry(const GetItemEntry*,CwItemDrawInfo*) {return 0;}
int GetItem_GetDrawTableEntry(int,void**,int,int*,float*,int*,uint8_t*) {++nativeRows;return 0;}
void GetItem_GetDrawSetupDLs(int,void**,void**) {}
/* OWNER_DESCRIPTOR */
/* HOST_INFO */
RandomizerGet selected=RG_MM_MASK_POSTMAN;
int producerResult=1;
int32_t Describe(const char*,CwItemDrawInfo* out) {
 if(producerResult!=1) return producerResult;
 return OOT_FillItemDrawInfo(selected,out);
}
namespace ComboRando {
constexpr int GAME_OOT=0;
struct ForeignItem {int itemGame=GAME_OOT;std::string itemName="mask",fakeItemName;bool HasDisguise()const{return !fakeItemName.empty();}};
}
using RandoCheckId=int;
ComboRando::ForeignItem foreign;
namespace Rando::MiscBehavior {const ComboRando::ForeignItem* MM_LookupForeign(int) {return &foreign;}}
void* Combo_ResolveSym(const char*,const char* name) {return !strcmp(name,"OOT_GetItemDrawInfo")?(void*)Describe:nullptr;}
const char* ComboInternRoutedPathOOT(const std::string& path) {static std::set<std::string> paths;return paths.insert(path).first->c_str();}
enum class ComboForeignResolveOOT {Ok,Unknown,NotReady};
/* HOST_RESOLVER */
struct Gfx {int stream;};
Gfx opa[64],xlu[64];
struct GraphicsContext {Gfx* o=opa;Gfx* x=xlu;};
GraphicsContext gfx;
struct PlayState {struct {GraphicsContext* gfxCtx=&gfx;} state;};
PlayState play;PlayState* gPlayState=&play;
std::vector<std::pair<int,std::string>> submitted;
std::vector<int> luts;
#define OPEN_DISPS(g) ((void)(g))
#define CLOSE_DISPS(g) ((void)(g))
#define POLY_OPA_DISP gfx.o
#define POLY_XLU_DISP gfx.x
#define gSPDisplayList(p,dl) submitted.emplace_back((p)->stream,(const char*)(dl))
#define gDPSetTextureLUT(p,lut) luts.push_back((p)->stream)
#define G_TT_NONE 0
#define MM_FOREIGN_PIN_OPA() ((void)0)
#define MM_FOREIGN_PIN_XLU() ((void)0)
#define MATRIX_FINALIZE_AND_LOAD(p,g) ((void)(p))
#define MTXMODE_APPLY 1
float scale=0;
void Matrix_Scale(float x,float y,float z,int) {assert(x==y && y==z);scale=x;}
void Gfx_SetupDL25_Opa(GraphicsContext*) {}
void Gfx_SetupDL25_Xlu(GraphicsContext*) {}
#define gDPSetEnvColor(p,...) ((void)(p))
/* HOST_SIMPLE_DRAW */
int sentinels=0;
constexpr int RC_UNKNOWN=-1;
ComboForeignDrawInfoOOT current;
const ComboForeignDrawInfoOOT* ComboResolveForeignDrawInfoOOT(int rc) {return ComboFillForeignDrawInfoOOT(rc,current)==ComboForeignResolveOOT::Ok?&current:nullptr;}
void GetItem_Draw(PlayState*,int gid) {assert(gid==GID_RUPEE_BLUE);++sentinels;}
bool ComboForeignAnim_Draw(const CwItemAnimDrawInfo*,const char*,PlayState*) {assert(0);return false;}
using RandoItemId=int;
enum {RI_NONE,RI_OOT_NEI_CANE_OF_SOMARIA,RI_OOT_NEI_CANE_PACCI_FLIP,RI_OOT_NEI_CANE_SOMARIA_BLOCK,RI_OOT_NEI_CANE_PACCI_STONE,RI_OOT_NEI_CANE_SOMARIA_PLATFORM,RI_OOT_NEI_CANE_PACCI_ULTRAHAND};
void DrawOotNeiUltrahand() {assert(0);}
void DrawOotNeiCaneOfSomaria(int) {assert(0);}
void MM_DrawNeiGi(const CwItemDrawInfo&) {assert(0);}
/* OTHER_DRAW_HANDLERS */
/* HOST_DISPATCH */
int main() {
 for(auto& cmd:opa)cmd.stream=0;
 for(auto& cmd:xlu)cmd.stream=1;
 assert(CwMinDlistsForKind(CW_DRAW_KIND_MM_MASK)==2);
 CwItemDrawInfo bad{};
 assert(!OOT_DescribeMmMaskDraw(ITEM_MM_MASK_POSTMAN-1,&bad));
 assert(!OOT_DescribeMmMaskDraw(ITEM_MM_MASK_FIERCE_DEITY+1,&bad));
 assert(!OOT_DescribeMmMaskDraw(ITEM_MM_MASK_ROMANI,nullptr));
 for(int i=0;i<24;++i) {
  selected=(RandomizerGet)(RG_MM_MASK_POSTMAN+i);CwItemDrawInfo out{};
  assert(OOT_FillItemDrawInfo(selected,&out)==1 && out.drawKind==CW_DRAW_KIND_MM_MASK);
  assert(nativeRows==0 && out.dlistCount==2);
  const auto& expected=sMmMaskDrawTable[i];
  assert(!strcmp(out.dlists[0],expected.dl1) && !strcmp(out.dlists[1],expected.dl2));
  assert(out.xluStartIndex==(expected.mode==MM_MASK_DRAW_OPA0_XLU1?1:-1));
  gfx.o=opa;gfx.x=xlu;submitted.clear();luts.clear();current={};
  MM_DrawComboForeign(1);
  assert(sentinels==0 && current.ok && submitted.size()==2);
  for(int layer=0;layer<2;++layer) {
   assert(submitted[layer].second==std::string("__OTR__@mm:")+(out.dlists[layer]+7));
   assert(submitted[layer].first==(layer==1 && out.xluStartIndex==1?1:0));
  }
  assert(luts==(out.xluStartIndex==1?std::vector<int>{0,1}:std::vector<int>{0}));
 }
 const char* remains[]={gMmRemainsOdolwaDL,gMmRemainsGohtDL,gMmRemainsGyorgDL,gMmRemainsTwinmoldDL};
 assert(!OOT_DescribeMmRemainsDraw((int)RG_MM_REMAINS_ODOLWA-1,&bad));
 assert(!OOT_DescribeMmRemainsDraw(RG_MM_REMAINS_GOHT,nullptr));
 for(int i=0;i<4;++i) {
  selected=(RandomizerGet)(RG_MM_REMAINS_ODOLWA+i);CwItemDrawInfo out{};
  assert(OOT_FillItemDrawInfo(selected,&out)==1 && out.drawKind==CW_DRAW_KIND_MM_REMAINS);
  assert(out.dlistCount==1 && out.xluStartIndex==-1 && out.scale==0.02f && nativeRows==0);
  assert(!strcmp(out.dlists[0],remains[i]));
  gfx.o=opa;gfx.x=xlu;submitted.clear();luts.clear();current={};scale=0;
  MM_DrawComboForeign(1);
  assert(sentinels==0 && current.ok && submitted.size()==1 && submitted[0].first==0);
  assert(submitted[0].second==std::string("__OTR__@mm:")+(remains[i]+7));
  assert(luts==std::vector<int>{0} && scale==0.02f);
 }
 selected=RG_MM_MASK_ROMANI;CwItemDrawInfo romani{};assert(OOT_FillItemDrawInfo(selected,&romani));
 assert(!strcmp(romani.dlists[0],gGiRomaniMaskCapDL) && !strcmp(romani.dlists[1],gGiRomaniMaskNoseEyeDL));
 const char* retained=current.dls[0];for(int i=0;i<1000;++i)ComboInternRoutedPathOOT(std::to_string(i));
 assert(!strncmp(retained,"__OTR__@mm:",11));
 producerResult=CW_DRAW_NOT_READY;current={};assert(ComboFillForeignDrawInfoOOT(1,current)==ComboForeignResolveOOT::NotReady);
 MM_DrawComboForeign(RC_UNKNOWN);assert(sentinels==1);
 std::cout<<"PASS all 24 mask and four boss-remains production recipes and MM draw dispatch: no blue sentinel, correct native MM routing, opaque/translucent passes, LUT reset, Romani layers and retained paths\n";
}
