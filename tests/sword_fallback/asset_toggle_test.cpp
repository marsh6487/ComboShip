#include <cassert>
#include <cstring>
#include <iostream>
#include <set>
#include <string>
#include <unordered_set>
#include "combo/menu/ComboItemDrawABI.h"
#include "soh/soh/Enhancements/randomizer/NeiGiFrameFit.h"
using NeiGi::Kind;
#define COMBO_EXPORT
#define CVAR_ENHANCEMENT(x) x
#define CVAR_NEI_GI_EFFECTS "test.ItemEffects"
/* ITEM_IDS */
bool ootAlt=true,mmAlt=false,legacyMod=false,mmLegacyMod=false;
bool ResourceMgr_IsAltAssetsEnabled(){return mmAlt;}
int32_t OOT_NeiAltAssetsEnabled(){return ootAlt;}
int CVarGetInteger(const char*,int){return 0;}
const char* NeiResource_Route(const char* path){return path;}
int ResourceMgr_IsModAssetForGame(const char*,const char*){return mmLegacyMod;}
std::set<std::string> resources;
int32_t OOT_NeiEnsureGiBaseOwner(){return 1;}
int32_t OOT_NeiResourceExists(const char* path){
    if(!path)return 0;
    if(!strncmp(path,"__OTR__@oot-gi-base:",20))return resources.contains(std::string("__OTR__")+(path+20));
    return resources.contains(path);
}
struct Presentation {
    const char* opaque;const char* translucent;float scale;Kind effect;
    struct{float x,y,z;}effectCenter{};bool alwaysShimmer=true;void(*draw)()=nullptr;
};
const Presentation kMmKokiriPresentation={"__OTR__objects/nei_gi_redesign/mm_kokiri_sword/gi_dl",nullptr,1,Kind::KokiriSword};
const Presentation kPresentations[]={
    {"__OTR__objects/nei_gi_redesign/master_sword/gi_dl",nullptr,1,Kind::MasterSword},
    {"__OTR__objects/nei_gi_redesign/true_master_sword/gi_dl",nullptr,1,Kind::SwordAura},
    {"__OTR__objects/nei_gi_redesign/razor_sword/gi_dl",nullptr,1,Kind::RazorSword},
    {"__OTR__objects/nei_gi_redesign/gilded_sword/gi_dl",nullptr,1,Kind::GildedSword},
    {"__OTR__objects/nei_gi_redesign/biggoron_sword/gi_dl",nullptr,1,Kind::BiggoronSword},
    {"__OTR__objects/nei_gi_redesign/great_fairy_sword/gi_dl",nullptr,1,Kind::GreatFairySword},
    {"__OTR__objects/nei_gi_redesign/fire_rod/gi_dl",nullptr,1,Kind::Fire},
};
struct{const char* slug;}kSeasons[]={{"season_spring"}};
void Randomizer_DrawCaneSomariaUpgrade(){}
bool NeiGi_FillSeasonInfo(int,CwItemDrawInfo*){return false;}
bool HasLegacyGiMod(const Presentation&,bool){return legacyMod;}
bool HasRedesignGiMod(const Presentation&){return false;}
/* OWNER_SELECTION */
/* OWNER_ASSET_QUERY */
/* OWNER_QUERY */
enum RandomizerGet{RG_KOKIRI_SWORD,RG_RAZOR_SWORD,RG_GILDED_SWORD,RG_TRUE_MASTER_SWORD,RG_MASTER_SWORD,RG_BIGGORON_SWORD,RG_GREAT_FAIRY_SWORD};
/* OWNER_CUSTOM_RECIPE */
int OwnerItemDescribe(const char* name,CwItemDrawInfo* out){
    const auto id=!strcmp(name,"Master Sword")?RG_MASTER_SWORD:!strcmp(name,"True Master Sword")?RG_TRUE_MASTER_SWORD:
      !strcmp(name,"Biggoron's Sword")?RG_BIGGORON_SWORD:!strcmp(name,"Great Fairy's Sword")?RG_GREAT_FAIRY_SWORD:
      !strcmp(name,"Razor Sword")?RG_RAZOR_SWORD:!strcmp(name,"Gilded Sword")?RG_GILDED_SWORD:RG_KOKIRI_SWORD;
    return CwAltSwordGi(id,out);
}
void* Combo_ResolveSym(const char*,const char* name){
    if(!strcmp(name,"OOT_GetItemDrawInfo"))return reinterpret_cast<void*>(OwnerItemDescribe);
    /* ASSET_QUERY_SYMBOL */
    if(!strcmp(name,"OOT_GetNeiGiDrawInfo"))return reinterpret_cast<void*>(OOT_GetNeiGiDrawInfo);
    return nullptr;
}
/* MM_BINDINGS */
/* MM_SELECTION */
namespace ComboRando {
constexpr int GAME_OOT=0;
struct ForeignItem {int itemGame=0;std::string itemName="Master Sword",fakeItemName;bool HasDisguise()const{return false;}} foreignItem;
}
namespace Rando::MiscBehavior {const ComboRando::ForeignItem* MM_LookupForeign(int){return &ComboRando::foreignItem;}}
using RandoCheckId=int;
struct {uint32_t gameplayFrames=42;} fakePlay;
auto* gPlayState=&fakePlay;
int ComboNativeMmImport(const char*){return -1;}
enum class ComboForeignResolveOOT{Ok,Unknown,NotReady};
const char* ComboInternRoutedPathOOT(const std::string& s){static std::unordered_set<std::string> paths;return paths.insert(s).first->c_str();}
#include "combo/menu/ComboSwordGiAssetSelection.h"
/* FOREIGN_RESOLVER */
int main(){
    resources.insert(kMmKokiriPresentation.opaque);
    for(const auto& item:kPresentations)resources.insert(item.opaque);
    resources.insert("__OTR__alt/objects/object_custom_equip/gCustomKokiriSwordDL");
    resources.insert("__OTR__alt/objects/object_custom_equip/gCustomMasterSwordDL");
    resources.insert("__OTR__alt/objects/object_custom_equip/gCustomLongswordDL");
    struct Award{RandoItemId id;const char* slug;Kind identity;};
    const Award awards[]={{RI_SWORD_KOKIRI,"mm_kokiri_sword",Kind::KokiriSword},
      {RI_SWORD_RAZOR,"razor_sword",Kind::RazorSword},{RI_SWORD_GILDED,"gilded_sword",Kind::GildedSword},
      {RI_OOT_MASTER_SWORD,"master_sword",Kind::MasterSword},{RI_OOT_TRUE_MASTER_SWORD,"true_master_sword",Kind::SwordAura},
      {RI_OOT_BIGGORON_SWORD,"biggoron_sword",Kind::BiggoronSword},
      {RI_GREAT_FAIRY_SWORD,"great_fairy_sword",Kind::GreatFairySword}};
    // The active MM mode must win over an independently enabled OoT donor.
    // Repeated transitions also exercise the descriptor's static function cache.
    for(bool donor:{true,false})for(bool host:{false,true,false,true,false})for(const auto& award:awards){
        ootAlt=donor;mmAlt=host;legacyMod=mmLegacyMod=false;
        CwItemDrawInfo info{};assert(MM_DescribeNeiGi(award.id,&info));
        assert(info.itemShimmer&&info.neiShimmer==static_cast<int>(award.identity)+1&&info.stateDependent==2);
        if(!host||!donor){
            const std::string authored=std::string(!host?"__OTR__@oot-gi-base:":"__OTR__")+"objects/nei_gi_redesign/"+award.slug+"/gi_dl";
            assert(info.drawKind==CW_DRAW_KIND_NEI_GI&&!strcmp(info.dlists[0],authored.c_str())&&
                   "vanilla MM sword GI retained a custom OoT donor model");
            assert(NeiGi::FindFrameBounds(info.dlists[0])&&"base-owner route lost the approved sword framing");
        }else assert(info.drawKind==CW_DRAW_KIND_CUSTOM_GI&&strstr(info.dlists[0],"object_custom_equip"));
        assert(ootAlt==donor&&mmAlt==host&&"GI selection mutated either owner's global asset state");
    }
    // A foreign OoT award must honor MM's independent Alt state, including
    // repeated draws through the resolver's cached producer function.
    for(bool host:{true,false,true,false}) {
      ootAlt=true;mmAlt=host;legacyMod=mmLegacyMod=false;
      ComboForeignDrawInfoOOT foreign{};
      assert(ComboFillForeignDrawInfoOOT(1,foreign,"Master Sword")==ComboForeignResolveOOT::Ok);
      if(host)assert(foreign.drawKind==CW_DRAW_KIND_CUSTOM_GI && strstr(foreign.dls[0],"object_custom_equip"));
      else assert(foreign.drawKind==CW_DRAW_KIND_NEI_GI &&
                  !strcmp(foreign.dls[0],"__OTR__@oot-gi-base:objects/nei_gi_redesign/master_sword/gi_dl") &&
                  "foreign sword ignored the active MM vanilla asset setting");
      assert(foreign.appearanceDependent && foreign.stateDependent);
    }
    // Missing shipped data is retryable, so a later host toggle cannot remain
    // frozen in the foreign renderer's negative cache.
    ootAlt=true;mmAlt=false;legacyMod=mmLegacyMod=false;
    resources.erase("__OTR__objects/nei_gi_redesign/master_sword/gi_dl");
    ComboForeignDrawInfoOOT missing{};
    assert(ComboFillForeignDrawInfoOOT(1,missing,"Master Sword")==ComboForeignResolveOOT::NotReady &&
           "missing vanilla sword was permanently cached as an unknown foreign item");
    mmAlt=true;missing={};
    assert(ComboFillForeignDrawInfoOOT(1,missing,"Master Sword")==ComboForeignResolveOOT::Ok);
    resources.insert("__OTR__objects/nei_gi_redesign/master_sword/gi_dl");
    // A surviving/base-path mod must not suppress the authored vanilla sword.
    ootAlt=true;mmAlt=false;legacyMod=mmLegacyMod=true;
    CwItemDrawInfo info{};assert(MM_DescribeNeiGi(RI_SWORD_RAZOR,&info)&&info.drawKind==CW_DRAW_KIND_NEI_GI);
    mmAlt=true;assert(!MM_DescribeNeiGi(RI_SWORD_RAZOR,&info)&&info.itemShimmer);
    // Non-sword legacy priority keeps its existing behavior.
    mmAlt=false;mmLegacyMod=false;legacyMod=true;
    assert(!MM_DescribeNeiGi(RI_OOT_NEI_FIRE_ROD,&info));
    legacyMod=false;assert(MM_DescribeNeiGi(RI_OOT_NEI_FIRE_ROD,&info));
    std::cout<<"PASS seven sword identities, independent MM/OoT asset states, repeated vanilla/custom transitions, surviving legacy mods and unchanged non-sword priority\n";
}
