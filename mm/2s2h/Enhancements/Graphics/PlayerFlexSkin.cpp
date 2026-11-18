#include "global.h"
#include "BenPort.h"
#include "2s2h/resource/type/Array.h"
#include "2s2h/resource/type/Skeleton.h"
#include "mods/pak_loader/pak_loader.h"
#include <fast/resource/type/DisplayList.h>
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/archive/ArchiveManager.h>
extern "C" bool ResourceMgr_IsAltAssetsEnabled();
extern "C" uint8_t ResourceMgr_FileExists(const char* path);
extern "C" uint8_t ResourceMgr_FileAltExists(const char* path);

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
