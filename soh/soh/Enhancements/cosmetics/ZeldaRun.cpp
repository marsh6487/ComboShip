#include "global.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/resource/type/Animation.h"
#include "soh/resource/type/PlayerAnimation.h"
#include "mods/transformation_masks/transformation_masks.h"
#include "mods/transformation_masks/wolf_link_form.h"
#include "expansions/sm64/sm64_mario.h"
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/archive/ArchiveManager.h>
#include <libultraship/bridge/consolevariablebridge.h>

static bool ZeldaRun_Adult() {
    return LINK_AGE_IN_YEARS == YEARS_ADULT;
}
static bool ZeldaRun_Eligible(Player* player) {
    return player && gPlayState && player == GET_PLAYER(gPlayState) && !TransformMasks_IsTransformedAny() &&
           !WolfLinkForm_IsSelected() && !Sm64Mario_ShouldHideLink();
}
static std::shared_ptr<Ship::IResource> ZeldaRun_Load(const std::string& path) {
    const auto context = Ship::Context::GetRawInstance();
    const auto manager = context ? context->GetResourceManager() : nullptr;
    const auto archive = manager ? manager->GetArchiveManager() : nullptr;
    if (!archive || (!archive->HasFile(path) && !(manager->IsAltAssetsEnabled() && archive->HasFile("alt/" + path))))
        return nullptr;
    return manager->LoadResource(path);
}
#include "../../../../combo/actors/ZeldaRunBridge.inc"
