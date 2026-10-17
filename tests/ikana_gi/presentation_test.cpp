// Execute production shield pose selectors and icon routes at graphics/loader seams.
#include "combo/menu/ComboItemDrawABI.h"
#include "combo/menu/ComboItemIconOwnership.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
using s16 = int16_t; using u8 = uint8_t;
constexpr int MTXMODE_APPLY=1, G_MTX_NOPUSH=0, G_MTX_LOAD=0, G_MTX_MODELVIEW=0;
constexpr int RI_SHIELD_MIRROR=1, RG_EXT_SHIELD_OF_IKANA=1;
struct Gfx {const char* route=nullptr;}; struct GraphicsContext {}; struct State { GraphicsContext* gfxCtx; };
struct PlayState { State state; int gameplayFrames=0; }; struct GetItemEntry {};
GraphicsContext ctx; PlayState play{{&ctx}}; PlayState* gPlayState=&play;
Gfx commands[64], model, wrongHostModel; Gfx* opa=commands;
float poseX=0, poseZ=0, spin=0, scale=1, height=0, depth=0, width=0, chinY=0;
bool flat=false, sideways=false, ootMod=false, canMeasure=true, useMod=true, donorAvailable=true;
const char* drawnRoute=nullptr;
std::string poseOwner;
int failures=0;
struct Pose {float x,z,scale;};
std::vector<Pose> stack;
std::vector<int> rotations;
void check(bool ok,const char* label) { if(!ok){std::cerr<<"FAIL "<<label<<'\n';++failures;} }
void Matrix_Push(){stack.push_back({poseX,poseZ,scale});}
void Matrix_Pop(){poseX=stack.back().x;poseZ=stack.back().z;scale=stack.back().scale;stack.pop_back();}
void Matrix_RotateY(float angle,u8){spin=angle;}
void Matrix_RotateZ(float angle,u8){poseZ+=angle;rotations.push_back(CW_OP_ROTATE_Z);}
void Matrix_RotateZF(float angle,u8 mode){Matrix_RotateZ(angle,mode);}
void Matrix_RotateX(float angle,u8){poseX+=angle;rotations.push_back(CW_OP_ROTATE_X);}
void Matrix_RotateXF(float angle,u8 mode){Matrix_RotateX(angle,mode);}
void Matrix_Scale(float x,float,float,u8){scale*=x;}
void* Matrix_NewMtx(GraphicsContext*,char*,int){return nullptr;}
void Gfx_SetupDL_25Opa(GraphicsContext*){}
void record(const Gfx* selected=nullptr) {
    drawnRoute=selected?selected->route:nullptr;
    const bool selectedFlat=selected==&wrongHostModel?!flat:flat;
    const float x=sideways?99.f:68.f;
    const float vertical=sideways?68.f:99.f;
    const float y=selectedFlat?2.f:vertical,z=selectedFlat?vertical:2.f;
    const float tiltedY=std::abs(std::cos(poseX))*y+std::abs(std::sin(poseX))*z;
    height=scale*(std::abs(std::sin(poseZ))*x+std::abs(std::cos(poseZ))*tiltedY);
    width=scale*(std::abs(std::cos(poseZ))*x+std::abs(std::sin(poseZ))*tiltedY);
    depth=scale*(std::abs(std::sin(poseX))*y+std::abs(std::cos(poseX))*z);
    const float chinX=sideways?1.f:0.f;
    const float chinAfterX=sideways?0.f:(selectedFlat?-std::sin(poseX):-std::cos(poseX));
    chinY=std::sin(poseZ)*chinX+std::cos(poseZ)*chinAfterX;
}
#define OPEN_DISPS(x) ((void)(x))
#define CLOSE_DISPS(x) ((void)(x))
#define POLY_OPA_DISP opa
#define gSPMatrix(p,...) ((void)(p))
#define gSPDisplayList(p,dl) ((void)(p),record(dl))
constexpr int G_DL_OTR_FILEPATH=1,G_DL_PUSH=0;
void gDma1p(Gfx* out,int,const char* path,int,int){out->route=path;}
void gSPEndDisplayList(Gfx*){}
Gfx* NeiGi_ModOverrideDL(const char*,bool){return useMod?&model:nullptr;}
void* TransformMasks_LoadMmDL(const char*){return &wrongHostModel;}
int ResourceMgr_IsGiModelAvailableForGame(const char* owner,const char*){
    check(!std::strcmp(owner,"mm"),"donor availability must follow measured MM owner");return donorAvailable;
}
int ResourceMgr_IsModAssetForGame(const char* game,const char*){return !std::strcmp(game,"oot")?ootMod:!ootMod;}
int ResourceMgr_GetIkanaShieldGiTiltXForGame(const char* owner,const char*,float* tilt){
    poseOwner=owner;
    if(!canMeasure)return 0;
    *tilt=flat?float(M_PI/2):0.f;return 1;
}
int ResourceMgr_GetIkanaShieldGiPoseForGame(const char* owner,const char* path,float* tilt,float* roll){
    if(!ResourceMgr_GetIkanaShieldGiTiltXForGame(owner,path,tilt))return 0;
    *roll=sideways?-float(M_PI/2):0.f;return 1;
}
void DescriptorPose(const CwItemDrawInfo& info){
    poseX=poseZ=0;scale=info.scale;rotations.clear();
    for(int i=0;i<info.opCount;++i){
        if(info.ops[i].op==CW_OP_ROTATE_X)Matrix_RotateX(info.ops[i].a*float(M_PI/32768),MTXMODE_APPLY);
        if(info.ops[i].op==CW_OP_ROTATE_Z)Matrix_RotateZ(info.ops[i].a*float(M_PI/32768),MTXMODE_APPLY);
    }
    record();
}
void GetItem_Draw(PlayState*,int){record();}
namespace Rando::StaticData { struct Item {int drawId=1;}; std::map<int,Item> Items; }
/* OOT_NATIVE_DRAW */
/* OOT_DESCRIPTOR */
/* MM_NATIVE_DRAW */

namespace Ship {
struct Resource {std::string name;};
class ResourceManager {
  public:
    std::shared_ptr<Resource> LoadResourceProcess(const char* raw){
        std::string path=raw;if(path.starts_with("__OTR__"))path.erase(0,7);
        return files.contains(path)?files[path]:nullptr;
    }
    std::map<std::string,std::shared_ptr<Resource>> files;
};
struct CrossRMRegistry {
    static std::shared_ptr<ResourceManager> Get(const std::string& game){return owners.contains(game)?owners[game]:nullptr;}
    static inline std::map<std::string,std::shared_ptr<ResourceManager>> owners;
};
} // namespace Ship
namespace Fast {using Texture=Ship::Resource;}
std::shared_ptr<Ship::ResourceManager> active;
std::shared_ptr<Ship::ResourceManager> ActiveResMgr(){return active;}
/* TEXTURE_RESOLVER */
bool nativeIconPresent=true, donorQueried=false;
u8 ResourceMgr_FileExists(const char*){return nativeIconPresent;}
u8 ResourceMgr_FileAltExists(const char*){return false;}
bool ResourceMgr_IsAltAssetsEnabled(){return false;}
int NeiResource_Available(const char*){donorQueried=true;return 1;}
const char* NeiResource_Route(const char*){return "__OTR__@oot:icon_item_static_yar/gItemIconMirrorShieldTex";}
/* EQUIPMENT_SELECTOR */
/* EQUIPMENT_TABLES */
/* OOT_CATALOG_ICON */

int main(){
    play.gameplayFrames=17;
    for(bool selectedSideways:{false,true})for(bool selectedFlat:{false,true}) {
        sideways=selectedSideways;flat=selectedFlat;poseX=poseZ=0;scale=1;opa=commands;rotations.clear();
        Randomizer_DrawExtShieldOfIkana(&play,nullptr);
        check(height>depth*10&&height>width&&chinY<-.99f,"OoT selected worn shield must face upright, chin down");
        check(rotations==std::vector<int>({CW_OP_ROTATE_Z,CW_OP_ROTATE_X}),"OoT roll precedes X tilt");
        check(std::abs(scale-.035f)<.00001f&&std::abs(spin-.34f)<.00001f,"OoT pickup scale and spin preserved");
        poseX=poseZ=0;scale=1;rotations.clear();
        DrawNativeIkanaGI();
        check(height>depth*10&&height>width&&chinY<-.99f,"MM selected GI shield must face upright, chin down");
        check(rotations==std::vector<int>({CW_OP_ROTATE_Z,CW_OP_ROTATE_X}),"MM roll precedes X tilt");
        CwItemDrawInfo info{};DescribeIkana(&info);
        DescriptorPose(info);
        check(height>depth*10&&height>width&&chinY<-.99f,"foreign selected worn shield must face upright, chin down");
        check(rotations==std::vector<int>({CW_OP_ROTATE_Z,CW_OP_ROTATE_X}),"foreign roll precedes X tilt");
        check(info.stateDependent==2,"foreign worn shield pose must refresh across Alt changes");
    }
    check(stack.empty(),"shield matrix stack balanced");
    sideways=false;ootMod=true;flat=false;poseX=poseZ=0;scale=1;
    Randomizer_DrawExtShieldOfIkana(&play,nullptr);
    check(poseOwner=="oot"&&height>depth*10,"OoT local replacement retains priority and owner");
    CwItemDrawInfo local{};DescribeIkana(&local);
    check(poseOwner=="oot"&&!std::strcmp(local.dlists[0],"__OTR__objects/object_link_child/gLinkHumanMirrorShieldDL"),
          "foreign descriptor preserves the selected OoT local replacement");
    canMeasure=false;poseX=poseZ=0;scale=1;flat=true;
    Randomizer_DrawExtShieldOfIkana(&play,nullptr);
    check(height>depth*10,"unreadable worn graph preserves its old 90-degree pose");
    local={};DescribeIkana(&local);
    DescriptorPose(local);
    check(poseX==float(M_PI/2)&&poseZ==0,"unreadable foreign graph preserves its old 90-degree pose");
    poseX=poseZ=0;scale=1;DrawNativeIkanaGI();
    check(height<depth,"unreadable native graph preserves its original unrotated pose");
    check(stack.empty(),"unreadable shield matrix stack balanced");
    // An unscoped companion can load a different host shield at the same path.
    // Queue the measured MM owner's graph and refresh its pose on every draw.
    useMod=false;ootMod=false;canMeasure=true;
    for(bool donorFlat:{true,false,true,false}) {
        flat=donorFlat;poseX=poseZ=0;scale=1;opa=commands;
        Randomizer_DrawExtShieldOfIkana(&play,nullptr);
        check(height>depth*10,"native no-mod shield must draw the same upright MM geometry it measured");
        check(drawnRoute&&!std::strcmp(drawnRoute,"__OTR__@mm:objects/object_link_child/gLinkHumanMirrorShieldDL"),
              "native no-mod shield keeps MM root/dependency ownership live across donor toggles");
    }
    donorAvailable=false;opa=commands;
    Randomizer_DrawExtShieldOfIkana(&play,nullptr);
    check(opa==commands,"missing MM shield must skip without borrowing host geometry");
    useMod=true;donorAvailable=true;
    auto oot=std::make_shared<Ship::ResourceManager>(),mm=std::make_shared<Ship::ResourceManager>();
    Ship::CrossRMRegistry::owners={{"oot",oot},{"mm",mm}};
    constexpr char path[]="icon_item_static_yar/gItemIconMirrorShieldTex";
    auto own=std::make_shared<Ship::Resource>();own->name="MM Mirror Shield";
    auto wrong=std::make_shared<Ship::Resource>();wrong->name="OoT Mirror Shield mod";
    mm->files[path]=own;oot->files[path]=wrong;
    active=oot;
    check(ComboLoadTextureResource(COMBO_IKANA_SHIELD_ICON)==own,"Ikana icon must select MM while OoT is active");
    check(ComboLoadTextureResource(OotCatalogIcon)==own,"OoT native dialogue catalog selects MM Ikana artwork");
    check(active==oot,"routed icon must not change the active manager");
    for(const auto* icon:{MmEquip::ExtEquip_GetIcon(1,3),OotEquip::ExtEquip_GetIcon(1,3)}) {
        check(ComboLoadTextureResource(static_cast<const char*>(icon))==own,"both equipment grids must select MM Ikana artwork");
    }
    const auto* grid=static_cast<const char*>(KaleidoEquip_OotTex(COMBO_IKANA_SHIELD_ICON));
    check(grid&&ComboLoadTextureResource(grid)==own,"MM grid must preserve the MM owner marker");
    nativeIconPresent=false;donorQueried=false;
    grid=static_cast<const char*>(KaleidoEquip_OotTex(COMBO_IKANA_SHIELD_ICON));
    check(!donorQueried&&grid&&ComboLoadTextureResource(grid)==own,"cold MM icon index must never borrow OoT icon");
    std::cout<<"Ikana production pose and icon routes: "<<failures<<" failures\n";
    return failures?1:0;
}
