#pragma once
#include "BlackMarketRoomMagic.h"
#include <fast/resource/type/DisplayList.h>
#include <fast/resource/type/Vertex.h>
#include <fast/resource/type/Texture.h>
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManager.h>
#include <libultraship/bridge/consolevariablebridge.h>

namespace BlackMarketRoomMagic {
inline Palette ReadPalette(bool oot) {
    Palette palette = kDefaultPalette;
    for (size_t i = 0; i < palette.size(); ++i) {
        const auto key = std::string(oot ? "gCosmetics." : "gCosmetic.") + kCosmeticKeys[i];
        if (!CVarGetInteger((key + ".Changed").c_str(), 0))
            continue;
        const auto& fallback = kDefaultPalette[i];
        const auto color = CVarGetColor((key + (oot ? ".Value" : ".Color")).c_str(),
                                        Color_RGBA8{ fallback[0], fallback[1], fallback[2], 255 });
        palette[i] = { color.r, color.g, color.b, 255 };
    }
    return palette;
}

class Resources {
  public:
    struct PreparationTrace {
        const char* reason = "skipped";
        const void* owner = nullptr;
        const void* root = nullptr;
        bool alt = false;
    };
    const PreparationTrace& LastPreparation() const {
        return mPreparation;
    }
    bool Prepare(const char* host) {
        mPreparation = {};
        try {
            // Copied textures use @host routes, which require a registered owner.
            const auto owner = Ship::CrossRMRegistry::Get(host);
            mPreparation.owner = owner.get();
            if (!owner)
                return Unavailable("no-owner");
            const bool alt = owner->IsAltAssetsEnabled();
            mPreparation.alt = alt;
            const auto archive = owner->GetArchiveManager();
            // An unrelated Alt room needs its matching variant. Never put a
            // base-room material graph over a custom Alt room's geometry.
            if (alt && archive->HasFile(std::string("alt/") + SourceRoom(std::strcmp(host, "oot") == 0)) &&
                !archive->HasFile(std::string("alt/") + kRoom))
                return Unavailable("source-alt-unpaired");
            if (!archive->HasFile(kRoom) && !(alt && archive->HasFile(std::string("alt/") + kRoom)))
                return Unavailable("graph-absent");
            auto root = std::dynamic_pointer_cast<Fast::DisplayList>(owner->LoadResource(kRoom));
            mPreparation.root = root.get();
            if (!root)
                return Unavailable("root-not-display-list");
            mPreparation.reason = "ready";
            if (mRoom && mOwner == owner && mRoot == root && mAlt == alt && mHost == host)
                return true;
            Clear();
            Provider provider;
            provider.game = host;
            provider.display = [owner](const std::string& path) -> Display {
                auto resource = std::dynamic_pointer_cast<Fast::DisplayList>(owner->LoadResource(path));
                return resource ? Display{ resource->Instructions.data(), resource->Instructions.size(), resource }
                                : Display{};
            };
            provider.vertices = [owner](const std::string& path) -> Vertices {
                auto resource = std::dynamic_pointer_cast<Fast::Vertex>(owner->LoadResource(path));
                return resource ? Vertices{ resource->VertexList.data(), resource->VertexList.size(), resource }
                                : Vertices{};
            };
            provider.texture = [owner](const std::string& path) -> std::shared_ptr<void> {
                return std::dynamic_pointer_cast<Fast::Texture>(owner->LoadResource(path));
            };
            auto room = std::make_unique<Room>();
            if (!room->Load(provider))
                return Unavailable("graph-invalid");
            mRoom = std::move(room);
            mOwner = owner;
            mRoot = std::move(root);
            mAlt = alt;
            mHost = host;
            return true;
        } catch (const std::exception&) { return Unavailable("exception"); }
    }
    Gfx* Opaque() const {
        return mRoom ? mRoom->Opaque() : nullptr;
    }
    void SetPalette(const Palette& palette) {
        if (mRoom)
            mRoom->SetPalette(palette);
    }
    void Clear() {
        mRoom.reset();
        mRoot.reset();
        mOwner.reset();
        mHost.clear();
        mAlt = false;
    }

  private:
    PreparationTrace mPreparation;
    bool Unavailable(const char* reason) {
        mPreparation.reason = reason;
        Clear();
        return false;
    }
    std::unique_ptr<Room> mRoom;
    std::shared_ptr<Ship::ResourceManager> mOwner;
    std::shared_ptr<Fast::DisplayList> mRoot;
    std::string mHost;
    bool mAlt = false;
};
} // namespace BlackMarketRoomMagic
