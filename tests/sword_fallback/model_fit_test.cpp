#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "combo/NeiGiModelBounds.h"
#include "combo/menu/ComboItemDrawABI.h"
// Resource loading is the test boundary. Actual Fast resource payloads and the
// production graph traversal/FrameFit execute below, on supplied mod archives.
namespace Ship {
IResource::IResource(std::shared_ptr<ResourceInitData> init) : mInitData(init) {}
IResource::~IResource() = default;
std::shared_ptr<ResourceInitData> IResource::GetInitData() { return mInitData; }
}
namespace Fast {
DisplayList::DisplayList() : Resource(nullptr) {}
DisplayList::~DisplayList() = default;
Gfx* DisplayList::GetPointer() { return Instructions.data(); }
size_t DisplayList::GetPointerSize() { return Instructions.size()*sizeof(Gfx); }
Vertex::Vertex() : Resource(nullptr) {}
Vtx* Vertex::GetPointer() { return VertexList.data(); }
size_t Vertex::GetPointerSize() { return VertexList.size()*sizeof(Vtx); }
}
struct Loader {
    std::map<std::string,std::shared_ptr<Ship::IResource>> files;
    std::map<uint64_t,std::string> names;
    std::shared_ptr<Ship::IResource> operator()(const char* name) const {
        if (!name) return nullptr;
        if (!std::strncmp(name,"__OTR__",7)) name+=7;
        auto it=files.find(name);return it==files.end()?nullptr:it->second;
    }
    std::shared_ptr<Ship::IResource> operator()(uint64_t hash) const {
        auto it=names.find(hash);return it==names.end()?nullptr:(*this)(it->second.c_str());
    }
} loader;
void List(const char* name,uint64_t hash,std::initializer_list<std::pair<uintptr_t,uintptr_t>> words) {
    auto dl=std::make_shared<Fast::DisplayList>();dl->UCode=ucode_f3dex2;
    for(auto [w0,w1]:words){Gfx command{};command.words.w0=w0;command.words.w1=w1;dl->Instructions.push_back(command);}
    loader.files[name]=dl;loader.names[hash]=name;
}
void Vertex(const char* name,uint64_t hash,std::initializer_list<std::array<int16_t,3>> points) {
    auto vertices=std::make_shared<Fast::Vertex>();
    for(auto p:points){Vtx vertex{};std::copy(p.begin(),p.end(),vertex.v.ob);vertices->VertexList.push_back(vertex);}
    loader.files[name]=vertices;loader.names[hash]=name;
}
using f32=float;using s16=int16_t;
constexpr int MTXMODE_APPLY=1;
struct PlayState {struct{void* gfxCtx;} state;uint32_t gameplayFrames=42;} play;
struct Pose {float scale=1,lift=0,ry=0,rz=0;} pose;
void Matrix_Translate(float,float y,float,int) {pose.lift+=y;}
void Matrix_Scale(float x,float y,float z,int) {assert(x==y&&y==z);pose.scale*=x;}
void Matrix_RotateY(float a,int) {pose.ry+=a;}
void Matrix_RotateZ(float a,int) {pose.rz+=a;}
#include "combo/menu/ComboSwordGiFit.h"
extern "C" int ResourceMgr_GetGiModelFitForGame(const char*,const char* path,float scale,float tilt,int shop,float out[2]) {
    NeiGi::FrameBounds bounds{};NeiGi::ModelBoundsReader<Loader> reader(loader,tilt);
    if(!reader.Read(path,bounds))return 0;
    auto fit=NeiGi::FrameFit(bounds,scale,shop);out[0]=fit.scale;out[1]=fit.lift;return 1;
}
#define OPEN_DISPS(...) ((void)0)
#define CLOSE_DISPS(...) ((void)0)
#define Gfx_SetupDL_25Opa(...) ((void)0)
#undef gDPSetPrimColor
#undef gDPSetEnvColor
#undef gSPGrayscale
#undef gSPMatrix
#undef gDma1p
#define gDPSetPrimColor(...) ((void)0)
#define gDPSetEnvColor(...) ((void)0)
#define gSPGrayscale(...) ((void)0)
#define gSPMatrix(...) ((void)0)
#define gDma1p(...) ((void)0)
#define CVAR_ENHANCEMENT(x) x
int CVarGetInteger(const char*,int){return 0;}
void ComboDinSwordGi_DrawLayers(PlayState*,const char*,const char*) {}
enum RandomizerGet{RG_KOKIRI_SWORD,RG_RAZOR_SWORD,RG_GILDED_SWORD,RG_TRUE_MASTER_SWORD,RG_MASTER_SWORD,RG_BIGGORON_SWORD,RG_GREAT_FAIRY_SWORD};
int32_t OOT_NeiAltAssetsEnabled(){return 1;}
int32_t OOT_NeiResourceExists(const char* path){return bool(loader(path));}
/* PRODUCTION */
struct Expected {const char* path;float low,high,width;};
std::vector<Expected> expected;
/* ARCHIVES */
int main(){
    InitArchive();
    for(const auto& e:expected){
        NeiGi::FrameBounds bounds{};NeiGi::ModelBoundsReader<Loader> reader(loader,1.8f);
        assert(reader.Read(e.path,bounds));
        assert(std::abs(bounds.minimum.y-e.low)<.01f&&std::abs(bounds.maximum.y-e.high)<.01f);
        assert(std::abs(bounds.spinningWidth-e.width)<.01f);
        for(bool shop:{false,true})for(uint32_t frame=0;frame<360;++frame){
            pose={};play.gameplayFrames=frame;NeiGi_DrawSelectedSword(&play,e.path,shop);
            assert(std::abs(pose.rz-1.8f)<.0001f);
            assert(bounds.minimum.y*pose.scale+pose.lift >= (shop?-22.f:-52.f)-.001f);
            assert(bounds.maximum.y*pose.scale+pose.lift <= (shop?52.f:48.f)+.001f);
            assert(bounds.spinningWidth*pose.scale <= (shop?76.f:104.f)+.001f);
        }
    }
    for(auto id:{RG_KOKIRI_SWORD,RG_RAZOR_SWORD,RG_GILDED_SWORD,RG_MASTER_SWORD,RG_TRUE_MASTER_SWORD,RG_BIGGORON_SWORD,RG_GREAT_FAIRY_SWORD}){
        CwItemDrawInfo info{};
        if(CwAltSwordGi(id,&info)){
            assert(loader(info.dlists[0])&&info.itemShimmer&&info.neiShimmer>0);
            pose={};NeiGi_DrawSelectedSword(&play,info.dlists[0]);
            NeiGi::FrameBounds bounds{};NeiGi::ModelBoundsReader<Loader> reader(loader,1.8f);assert(reader.Read(info.dlists[0],bounds));
            assert(bounds.maximum.y*pose.scale+pose.lift<=48.001f);
        }
    }
    std::cout<<"PASS selected mod graph + native/producer GI route: "<<expected.size()<<" meshes, upright full-spin pickup/shop bounds\n";
}
