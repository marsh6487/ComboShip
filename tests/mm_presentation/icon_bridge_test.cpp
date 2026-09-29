#include <cassert>
#include <cstring>
#include <memory>
#include <set>
#include <string>
#include <iostream>
#include "combo/menu/ComboItemDrawABI.h"
using s16=int16_t; using u8=uint8_t; using TexturePtr=void*;
enum RandomizerGet { RG_NONE, RG_CUSTOM, RG_PROGRESSIVE, RG_TIER, RG_NATIVE, RG_BAD };
constexpr int ICON_SIZE_24=24, ICON_SIZE_32=32, ITEM_MEDALLION_FOREST=102, ITEM_HEART_PIECE_2=122,
              ITEM_ROCS_FEATHER_SKIJER=158;
void* gItemIcons[158]={};
const char* customPath="__OTR__textures/icon_item_custom/fire";
int customSize=32, nativeId=0, grants=0;
struct GetItemEntry { int itemId; };
namespace Rando::StaticData {
struct Item {
 RandomizerGet id;
 std::shared_ptr<GetItemEntry> GetGIEntry(RandomizerGet* actual) {
   if(id==RG_PROGRESSIVE) *actual=RG_TIER;
   return std::make_shared<GetItemEntry>(GetItemEntry{nativeId});
 }
 bool HasCustomIcon() { return id==RG_CUSTOM || id==RG_TIER; }
 const char* GetCustomIcon() { return id==RG_TIER ? (grants ? "__OTR__next-tier" : "__OTR__awarded-tier") : customPath; }
 int GetCustomIconSize() { return customSize; }
};
Item RetrieveItem(RandomizerGet id) { return {id}; }
}
/* OWNER_ICON */
static TexturePtr sMsgCustomIconTex=nullptr;
static s16 sMsgCustomIconWidth=32, sMsgCustomIconHeight=32;
static u8 sMsgCustomIconIA8=0;
/* STAGE_ICON */
namespace ComboRando { constexpr int GAME_OOT=0; struct ForeignItem { int itemGame=0;bool trap=false;std::string itemName="progressive"; }; }
using RandoCheckId=int;
ComboRando::ForeignItem foreign;
bool found=true, provider=true;
namespace Rando {
uint8_t ComboForeignMessageIcon(RandoCheckId);
namespace MiscBehavior { const ComboRando::ForeignItem* MM_LookupForeign(RandoCheckId) {return found?&foreign:nullptr;} }
}
int32_t DescribeIcon(const char* name,CwItemIconInfo* out) {
 return OOT_FillItemIconInfo(std::string(name)=="progressive" ? RG_PROGRESSIVE : RG_CUSTOM,out);
}
void* Combo_ResolveSym(const char*,const char*) { return provider ? (void*)DescribeIcon : nullptr; }
const char* ComboInternRoutedPathOOT(const std::string& path) {static std::set<std::string> paths;return paths.insert(path).first->c_str();}
/* CONSUMER_ICON */
int main() {
 CwItemIconInfo info{};
 assert(OOT_FillItemIconInfo(RG_CUSTOM,&info)==1 && info.width==32 && !info.isIA8);
 customSize=24;info={};assert(OOT_FillItemIconInfo(RG_CUSTOM,&info)==1 && info.width==24);
 customPath="__OTR__textures/icon_item_static/gSongNoteTex";info={};
 assert(OOT_FillItemIconInfo(RG_CUSTOM,&info)==1 && info.width==16 && info.height==24 && info.isIA8);
 customPath="__OTR__textures/parameter_static/gOcarinaBtnIconATex";info={};
 assert(OOT_FillItemIconInfo(RG_CUSTOM,&info)==1 && info.width==16 && info.height==16 && info.isIA8);
 customPath="__OTR__textures/icon_item_static/gHeartPieceIcon2Tex";info={};
 assert(OOT_FillItemIconInfo(RG_CUSTOM,&info)==1 && info.width==48 && info.height==48 && info.isIA8);
 gItemIcons[102]=(void*)"__OTR__textures/icon_item_24_static/medallion";nativeId=102;info={};
 assert(OOT_FillItemIconInfo(RG_NATIVE,&info)==1 && info.width==24 && !info.isIA8);
 for(int bad:{-1,158,50000}) {nativeId=bad;info={};assert(OOT_FillItemIconInfo(RG_BAD,&info)==0);}
 assert(Rando::ComboForeignMessageIcon(1)==0xF5);
 assert(std::string((char*)sMsgCustomIconTex)=="__OTR__@oot:awarded-tier");
 ++grants; // grant changes owner's inventory after staging
 assert(std::string((char*)sMsgCustomIconTex)=="__OTR__@oot:awarded-tier");
 const char* frozen=(char*)sMsgCustomIconTex;
 for(int i=0;i<1000;i++) ComboInternRoutedPathOOT("__OTR__@oot:"+std::to_string(i));
 assert(std::string(frozen)=="__OTR__@oot:awarded-tier");
 foreign.trap=true;assert(Rando::ComboForeignMessageIcon(1)==0xFE);foreign.trap=false;
 found=false;assert(Rando::ComboForeignMessageIcon(1)==0xFE);found=true;
 customPath="__OTR__textures/icon_item_static/gSongNoteTex";foreign.itemName="custom";
 assert(Rando::ComboForeignMessageIcon(1)==0xF5 && sMsgCustomIconWidth==16 && sMsgCustomIconHeight==24 && sMsgCustomIconIA8);
 Message_StageCustomItemIcon((void*)"native",32);
 assert(sMsgCustomIconWidth==32 && sMsgCustomIconHeight==32 && !sMsgCustomIconIA8);
 Message_StageCustomItemIconEx((void*)"bad",0,24,0);assert(!sMsgCustomIconTex);
 Message_StageCustomItemIconEx((void*)"bad",24,65,0);assert(!sMsgCustomIconTex);
 Message_StageCustomItemIconEx((void*)"bad",24,24,2);assert(!sMsgCustomIconTex);
 std::cout<<"PASS actual icon export, archive routing, pre-grant freeze, pointer lifetime, formats, dimensions and staging bounds\n";
}
