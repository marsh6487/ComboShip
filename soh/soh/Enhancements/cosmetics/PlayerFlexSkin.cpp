#include "global.h"
#include "soh/resource/type/PlayerAnimation.h"
#include "mods/transformation_masks/transformation_masks.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/resource/type/Array.h"
#include "soh/resource/type/Skeleton.h"
#include "mods/pak_loader/pak_loader.h"
#include <fast/resource/type/DisplayList.h>
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/archive/ArchiveManager.h>

static std::shared_ptr<Ship::IResource> PlayerFlexSkin_Load(const std::string& path) {
    auto context = Ship::Context::GetRawInstance();
    auto manager = context ? context->GetResourceManager() : nullptr;
    auto archive = manager ? manager->GetArchiveManager() : nullptr;
    if (!archive || (!archive->HasFile(path) && !archive->HasFile(path + ".meta") &&
                     !(manager->IsAltAssetsEnabled() && !path.starts_with("alt/") &&
                       (archive->HasFile("alt/" + path) || archive->HasFile("alt/" + path + ".meta")))))
        return nullptr;
    return manager->LoadResource(path);
}
#include "../../../../combo/actors/PlayerFlexSkinBridge.inc"

static bool PlayerFlexSkin_SeasonalIdleHostEligible(const Player& player) {
    return player.unk_6AD == 0 && !(player.stateFlags1 & PLAYER_STATE1_IN_WATER) &&
           !(player.stateFlags2 & PLAYER_STATE2_CRAWLING) && !TransformMasks_IsTransformedAny();
}
#include "../../../../combo/actors/SeasonalIdleBridge.inc"
