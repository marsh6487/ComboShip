// Real resource-graph reader and owner query; registry/archive loading are seams.
#include "combo/NeiGiModelBounds.h"
#include <fast/resource/type/Texture.h>
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <string>

namespace Ship {
class ResourceManager;
class Context {
  public:
    static Context* GetRawInstance();
    std::shared_ptr<ResourceManager> GetResourceManager(){return active;}
    void SetResourceManager(std::shared_ptr<ResourceManager> value){active=value;}
    std::shared_ptr<ResourceManager> active;
};
static Context context;
Context* Context::GetRawInstance(){return &context;}
class ResourceManager {
  public:
    std::shared_ptr<IResource> LoadResource(std::string path){
        assert(context.active.get()==this);
        if(path.starts_with("__OTR__"))path.erase(0,7);
        if(alt&&files.contains("alt/"+path))path="alt/"+path;
        return files.contains(path)?files[path]:nullptr;
    }
    std::shared_ptr<IResource> LoadResource(uint64_t hash){return hashes.contains(hash)?LoadResource(hashes[hash]):nullptr;}
    std::map<std::string,std::shared_ptr<IResource>> files;
    std::map<uint64_t,std::string> hashes;
    bool alt=false;
};
struct CrossRMRegistry {
    static std::shared_ptr<ResourceManager> Get(const std::string& game){return owners.contains(game)?owners[game]:nullptr;}
    static inline std::map<std::string,std::shared_ptr<ResourceManager>> owners;
};
} // namespace Ship
/* RESOURCE_SCOPE */
namespace NeiAssetPriority {
/* POSE_QUERY */
}
/* OWNER_QUERY */

void List(Ship::ResourceManager& owner,const char* path,std::initializer_list<std::pair<uintptr_t,uintptr_t>> words){
    auto list=std::make_shared<Fast::DisplayList>();list->UCode=ucode_f3dex2;
    for(auto [w0,w1]:words){Gfx cmd{};cmd.words.w0=w0;cmd.words.w1=w1;list->Instructions.push_back(cmd);}
    owner.files[path]=list;
}
void Vertices(Ship::ResourceManager& owner,const char* path,bool flat,bool binary){
    const int16_t y=flat?0:1000,z=flat?1000:20;
    std::array<std::array<int16_t,3>,4> points={{{-600,int16_t(-y),int16_t(-z)},{600,y,z},{-600,y,z},{600,int16_t(-y),int16_t(-z)}}};
    if(binary){
        auto vertices=std::make_shared<SOH::Array>();vertices->ArrayType=SOH::ArrayResourceType::Vertex;
        vertices->ArrayCount=points.size();
        for(auto point:points){Fast::F3DVtx v{};std::copy(point.begin(),point.end(),v.v.ob);vertices->Vertices.push_back(v);}
        owner.files[path]=vertices;
    }else{
        auto vertices=std::make_shared<Fast::Vertex>();
        for(auto point:points){Vtx v{};std::copy(point.begin(),point.end(),v.v.ob);vertices->VertexList.push_back(v);}
        owner.files[path]=vertices;
    }
}
int main(){
    auto oot=std::make_shared<Ship::ResourceManager>(),mm=std::make_shared<Ship::ResourceManager>();
    Ship::CrossRMRegistry::owners={{"oot",oot},{"mm",mm}};Ship::context.active=mm;
    constexpr char worn[]="objects/object_link_child/gLinkHumanMirrorShieldDL";
    constexpr char gi[]="objects/object_gi_shield_3/gGiMirrorShieldDL";
    for(auto owner:{oot,mm}){
        List(*owner,worn,{{uintptr_t(G_DL_OTR_FILEPATH)<<24,uintptr_t("body")},{uintptr_t(G_ENDDL)<<24,0}});
        List(*owner,gi,{{uintptr_t(G_DL_OTR_FILEPATH)<<24,uintptr_t("body")},{uintptr_t(G_ENDDL)<<24,0}});
        List(*owner,"body",{{uintptr_t(G_VTX_OTR_FILEPATH)<<24,uintptr_t("vertices")},{4,0},{uintptr_t(G_ENDDL)<<24,0}});
        owner->hashes[7]="vertices";
        List(*owner,"alt/body",{{(uintptr_t(G_VTX_OTR_HASH)<<24)|(4u<<12),0},{0,7},{uintptr_t(G_ENDDL)<<24,0}});
    }
    // Donor uses planar XZ geometry; MM is upright. Each Alt reverses its pose.
    Vertices(*oot,"vertices",true,false);Vertices(*oot,"alt/vertices",false,true);
    Vertices(*mm,"vertices",false,false);Vertices(*mm,"alt/vertices",true,true);
    for(bool hostAlt:{false,true})for(bool donorAlt:{false,true}){
        mm->alt=hostAlt;oot->alt=donorAlt;
        float tilt=-99;
        assert(ResourceMgr_GetIkanaShieldGiTiltXForGame("oot",worn,&tilt));
        assert(std::abs(tilt-(donorAlt?0.f:float(M_PI/2)))<.0001f);
        assert(ResourceMgr_GetIkanaShieldGiTiltXForGame("mm",gi,&tilt));
        assert(std::abs(tilt-(hostAlt?float(M_PI/2):0.f))<.0001f);
        assert(Ship::context.active==mm&&mm->alt==hostAlt&&oot->alt==donorAlt);
    }
    // Resource matrices contribute before the caller chooses an outer pose.
    auto matrix=std::make_shared<Fast::Matrix>();
    const float m[4][4]={{1,0,0,0},{0,0,1,0},{0,-1,0,0},{0,0,0,1}};
    auto* words=reinterpret_cast<uint32_t*>(&matrix->Matrx);
    for(int row=0;row<4;++row)for(int col=0;col<4;col+=2){
        const uint32_t a=int32_t(m[row][col]*65536),b=int32_t(m[row][col+1]*65536);
        words[row*2+col/2]=(a&0xffff0000u)|(b>>16);words[8+row*2+col/2]=(a<<16)|(b&0xffffu);
    }
    oot->alt=false;oot->files["pose"]=matrix;
    List(*oot,"transformed",{{(uintptr_t(G_MTX_OTR_FILEPATH)<<24)|G_MTX_PUSH,uintptr_t("pose")},
                            {uintptr_t(G_DL_OTR_FILEPATH)<<24,uintptr_t("body")},{uintptr_t(G_ENDDL)<<24,0}});
    float tilt=-99;
    assert(ResourceMgr_GetIkanaShieldGiTiltXForGame("oot","transformed",&tilt)&&tilt==0);
    // Missing/invalid descendants decline the correction and restore Context.
    oot->files["vertices"]=std::make_shared<Fast::Texture>();tilt=77;
    assert(!ResourceMgr_GetIkanaShieldGiTiltXForGame("oot",worn,&tilt)&&tilt==77);
    assert(!ResourceMgr_GetIkanaShieldGiTiltXForGame("unknown",gi,&tilt));
    assert(!ResourceMgr_GetIkanaShieldGiTiltXForGame(nullptr,gi,&tilt));
    assert(!ResourceMgr_GetIkanaShieldGiTiltXForGame("mm",nullptr,&tilt));
    assert(!ResourceMgr_GetIkanaShieldGiTiltXForGame("mm",gi,nullptr));
    assert(Ship::context.active==mm);
    auto native=
#ifdef TEST_HOST_MM
        mm;constexpr char nativeGame[]="mm";
#else
        oot;constexpr char nativeGame[]="oot";
#endif
    Ship::CrossRMRegistry::owners.erase(nativeGame);
#ifdef COMBO_BUILD
    assert(!ResourceMgr_GetIkanaShieldGiTiltXForGame(nativeGame,gi,&tilt));
#else
    native->alt=false;Vertices(*native,"vertices",false,false);Ship::context.active=native;
    assert(ResourceMgr_GetIkanaShieldGiTiltXForGame(nativeGame,gi,&tilt)&&tilt==0);
#endif
    std::cout<<"PASS actual Ikana geometry reader/query: XML/hash/native vertices, planar/vertical meshes, "
                 "resource matrix, independent Alt owners, missing resources and scope restoration\n";
}
