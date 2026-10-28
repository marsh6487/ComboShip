#include "LegacySceneRouting.h"
#include "LegacyScenePolicy.h"

#include <algorithm>
#include <unordered_set>
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/ResourceManagerScope.h>
#include <ship/resource/CrossRMRegistry.h>
#include <spdlog/spdlog.h>
#include "soh/resource/type/Scene.h"
#include "soh/resource/type/scenecommand/SetAlternateHeaders.h"
#include "soh/resource/type/scenecommand/SetCollisionHeader.h"
#include "soh/resource/type/scenecommand/SetPathways.h"
#include "soh/resource/type/scenecommand/SetRoomList.h"

namespace SOH::Unbound {
namespace {
class LegacyGraph {
  public:
    explicit LegacyGraph(std::shared_ptr<Ship::ResourceManager> owner) : resources(std::move(owner)) {}

    int Rank(std::string path) const {
        if (path.starts_with("__OTR__")) {
            path.erase(0, 7);
        }
        auto archives = resources->GetArchiveManager();
        if (resources->IsAltAssetsEnabled() && !path.starts_with("alt/") &&
            (archives->HasFile("alt/" + path) || archives->HasFile("alt/" + path + ".meta"))) {
            path = "alt/" + path;
        }
        return std::max(archives->GetFilePriority(path), archives->GetFilePriority(path + ".meta"));
    }

    void VisitPath(const std::string& path) {
        if (path.empty() || !paths.insert(path).second) {
            return;
        }
        priority = std::max(priority, Rank(path));
        auto scene = std::dynamic_pointer_cast<SOH::Scene>(resources->LoadResource(path));
        Visit(scene);
    }

    void Visit(const std::shared_ptr<SOH::Scene>& scene) {
        if (!scene || !scenes.insert(scene.get()).second) {
            return;
        }
        priority = std::max(priority, Rank(scene->GetInitData()->Path));
        for (const auto& command : scene->commands) {
            if (auto alternates = std::dynamic_pointer_cast<SOH::SetAlternateHeaders>(command)) {
                for (const auto& path : alternates->headerFileNames) {
                    priority = std::max(priority, Rank(path));
                }
                for (const auto& header : alternates->headers) {
                    Visit(header);
                }
            } else if (auto rooms = std::dynamic_pointer_cast<SOH::SetRoomList>(command)) {
                for (const auto& path : rooms->fileNames) {
                    VisitPath(path);
                }
            } else if (auto collision = std::dynamic_pointer_cast<SOH::SetCollisionHeader>(command)) {
                priority = std::max(priority, Rank(collision->fileName));
            } else if (auto paths = std::dynamic_pointer_cast<SOH::SetPathways>(command)) {
                for (const auto& path : paths->pathFileNames) {
                    priority = std::max(priority, Rank(path));
                }
            }
        }
    }

    int priority = -1;

  private:
    std::shared_ptr<Ship::ResourceManager> resources;
    std::unordered_set<std::string> paths;
    std::unordered_set<const SOH::Scene*> scenes;
};
} // namespace

bool LegacySceneOverridesJson(const std::string& legacyScene, const std::string& jsonScene) {
    auto owner = Ship::CrossRMRegistry::Get("oot");
#ifndef COMBO_BUILD
    if (!owner) {
        owner = Ship::Context::GetRawInstance()->GetResourceManager();
    }
#endif
    if (!owner) {
        return false;
    }
    Ship::ResourceManagerScope scope(owner);
    LegacyGraph graph(owner);
    int jsonPriority = graph.Rank(jsonScene);
    const auto slash = jsonScene.find_last_of('/');
    const auto directory = jsonScene.substr(0, slash + 1);
    auto documents = owner->GetArchiveManager()->ListFiles(directory + "*");
    if (documents) {
        for (const auto& path : *documents) {
            if (path.ends_with(".json")) {
                jsonPriority = std::max(jsonPriority, graph.Rank(path));
            }
        }
    }
    try {
        graph.VisitPath(legacyScene);
    } catch (const std::exception& e) {
        // An invalid overridden legacy header remains the winning resource;
        // the normal scene loader reports its own load failure/fallback.
        SPDLOG_WARN("[Unbound] legacy graph {} could not be inspected: {}", legacyScene, e.what());
    }
    const bool legacy = UseLegacyScene(true, graph.priority, jsonPriority);
    if (legacy) {
        SPDLOG_INFO("[Unbound] legacy structured override keeps {} on its native scene path", legacyScene);
    }
    return legacy;
}
} // namespace SOH::Unbound
