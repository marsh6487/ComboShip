#pragma once
#include "../NeiAssetPriority.h"
#include <ship/Context.h>
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/archive/Archive.h>
#include <ship/resource/archive/ArchiveManager.h>

// The dedicated GI wrappers retain MM's original shield, including every
// nested vertex and texture, without entering the generic Mirror Shield mod
// namespace. Dedicated Ikana root replacements still use MM's normal owner.
inline bool ComboEnsureIkanaShieldBaseOwner(const std::shared_ptr<Ship::ResourceManager>& owner) {
    if (!owner || !owner->GetArchiveManager())
        return false;
    if (Ship::CrossRMRegistry::Get("mm-ikana-base"))
        return true;
    const std::vector<std::string> shipped{ Ship::Context::LocateFileAcrossAppDirs("mm.o2r", "2ship"),
                                            Ship::Context::LocateFileAcrossAppDirs("mm.o2r", "soh"),
                                            Ship::Context::LocateFileAcrossAppDirs("mm.o2r") };
    const auto archives = owner->GetArchiveManager()->GetArchives();
    if (!archives)
        return false;
    for (const auto& archive : *archives) {
        if (!archive)
            continue;
        const auto path = NeiAssetPriority::NormalizeArchive(archive->GetPath());
        for (const auto& stock : shipped) {
            if (stock.empty() || path != NeiAssetPriority::NormalizeArchive(stock))
                continue;
            const auto view = owner->CreateResourceView(archive);
            if (!view)
                return false;
            Ship::CrossRMRegistry::Register("mm-ikana-base", view);
            return true;
        }
    }
    return false;
}
