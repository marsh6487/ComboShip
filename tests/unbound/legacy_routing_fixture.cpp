#include "soh/soh/unbound/LegacyScenePolicy.h"
#include <algorithm>
#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#define COMBO_BUILD 1
#define SPDLOG_WARN(...) ((void)0)
#define SPDLOG_INFO(...) ((void)0)

// Archive/loader boundaries only: the production traversal and selection body
// below consume linked scene, room, alternate, collision and path resources.
namespace Ship {
struct Init { std::string Path; };
struct Resource {
    virtual ~Resource() = default;
    std::shared_ptr<Init> init=std::make_shared<Init>();
    std::shared_ptr<Init> GetInitData() { return init; }
};
struct Archives {
    std::unordered_map<std::string,int> ranks;
    bool HasFile(const std::string& path) { return ranks.contains(path); }
    int GetFilePriority(const std::string& path) { return HasFile(path)?ranks.at(path):-1; }
    std::shared_ptr<std::vector<std::string>> ListFiles(const std::string& mask) {
        auto out=std::make_shared<std::vector<std::string>>();
        for(const auto& [path,rank]:ranks) if(path.starts_with(mask.substr(0,mask.size()-1))) out->push_back(path);
        return out;
    }
};
struct ResourceManager {
    bool alt=false;
    std::shared_ptr<Archives> archives=std::make_shared<Archives>();
    std::unordered_map<std::string,std::shared_ptr<Resource>> loaded;
    std::shared_ptr<Archives> GetArchiveManager() { return archives; }
    bool IsAltAssetsEnabled() { return alt; }
    std::shared_ptr<Resource> LoadResource(std::string path) {
        if(alt && loaded.contains("alt/"+path)) path="alt/"+path;
        return loaded.contains(path)?loaded.at(path):nullptr;
    }
};
std::shared_ptr<ResourceManager> oot, mm;
struct CrossRMRegistry {
    static std::shared_ptr<ResourceManager> Get(const std::string& game) { assert(game=="oot");return oot; }
};
struct ResourceManagerScope { explicit ResourceManagerScope(std::shared_ptr<ResourceManager> owner) { assert(owner==oot); } };
}
namespace SOH {
struct Scene:Ship::Resource { std::vector<std::shared_ptr<Ship::Resource>> commands; };
struct SetAlternateHeaders:Ship::Resource { std::vector<std::string> headerFileNames;std::vector<std::shared_ptr<Scene>> headers; };
struct SetRoomList:Ship::Resource { std::vector<std::string> fileNames; };
struct SetCollisionHeader:Ship::Resource { std::string fileName; };
struct SetPathways:Ship::Resource { std::vector<std::string> pathFileNames; };
}
namespace SOH::Unbound {
/* ACTUAL LEGACY GRAPH */
/* ACTUAL LEGACY ROUTING */
}

int main() {
    using SOH::Unbound::LegacySceneOverridesJson;
    const std::string native="scenes/shared/spot00_scene/spot00_scene";
    const std::string json="scenes/spot00/scene.json";
    const std::string room="scenes/shared/spot00_room_0/spot00_room_0";
    const std::string collision="scenes/shared/spot00_scene/collision";
    auto reset=[&]() {
        Ship::oot=std::make_shared<Ship::ResourceManager>();
        Ship::mm=std::make_shared<Ship::ResourceManager>();
        Ship::oot->archives->ranks={{native,1},{room,1},{collision,1},{json,2}};
        auto scene=std::make_shared<SOH::Scene>();scene->init->Path=native;
        auto rooms=std::make_shared<SOH::SetRoomList>();rooms->fileNames={room};
        auto col=std::make_shared<SOH::SetCollisionHeader>();col->fileName=collision;
        scene->commands={rooms,col};Ship::oot->loaded[native]=scene;
        auto header=std::make_shared<SOH::Scene>();header->init->Path=room;Ship::oot->loaded[room]=header;
        // A later MM resource must never affect an OOT path's provenance.
        Ship::mm->archives->ranks={{native,99},{json,0}};
    };
    reset();assert(!LegacySceneOverridesJson(native,json));
    Ship::oot->archives->ranks[room]=3;assert(LegacySceneOverridesJson(native,json));
    reset();Ship::oot->archives->ranks[collision]=3;assert(LegacySceneOverridesJson(native,json));
    reset();Ship::oot->archives->ranks[native]=3;assert(LegacySceneOverridesJson(native,json));
    reset();Ship::oot->archives->ranks[room+".meta"]=3;assert(LegacySceneOverridesJson(native,json));
    reset();Ship::oot->archives->ranks["alt/"+room]=3;
    assert(!LegacySceneOverridesJson(native,json));
    Ship::oot->alt=true;assert(LegacySceneOverridesJson(native,json));
    reset();Ship::oot->archives->ranks["scenes/shared/spot00_room_0/mesh"]=10;
    assert(!LegacySceneOverridesJson(native,json));
    reset();Ship::oot->archives->ranks[room]=3;
    Ship::oot->archives->ranks["scenes/spot00/rooms/0.json"]=4;
    assert(!LegacySceneOverridesJson(native,json));
    reset();auto alternate=std::make_shared<SOH::Scene>();alternate->init->Path="alternate";
    auto alternates=std::make_shared<SOH::SetAlternateHeaders>();alternates->headers={alternate};
    std::static_pointer_cast<SOH::Scene>(Ship::oot->loaded[room])->commands.push_back(alternates);
    Ship::oot->archives->ranks["alternate"]=3;assert(LegacySceneOverridesJson(native,json));
    reset();auto paths=std::make_shared<SOH::SetPathways>();paths->pathFileNames={"path"};
    std::static_pointer_cast<SOH::Scene>(Ship::oot->loaded[native])->commands.push_back(paths);
    Ship::oot->archives->ranks["path"]=3;assert(LegacySceneOverridesJson(native,json));
}
