#pragma once

#include <fast/Fast3dGui.h>
#include <fast/interpreter.h>
#include <ship/window/gui/resource/GuiTexture.h>
#include <memory>
#include <string_view>
#include <vector>

namespace ComboRando::SongEvents::Atlas {

struct Region {
    int x, y, width, height;

    ImVec2 Uv(float across, float down) const;
};

#include "SongEventGlyphAtlasData.inc"

inline ImVec2 Region::Uv(float across, float down) const {
    return ImVec2((x + width * across) / kAtlasWidth, (y + height * down) / kAtlasHeight);
}

inline ImTextureID GetTexture(Fast::Fast3dGui& gui, Fast::Interpreter& interpreter) {
    constexpr char name[] = "combo/ui/song-event-glyphs";
    if (!gui.HasTextureByName(name)) {
        constexpr std::string_view alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::vector<stbi_uc> png;
        png.reserve(std::size(kEncodedPng) * 72);
        uint32_t accumulated = 0;
        unsigned bits = 0;
        for (const auto line : kEncodedPng) {
            for (char c : line) {
                const auto index = alphabet.find(c);
                if (index == std::string_view::npos)
                    continue;
                accumulated = (accumulated << 6) | static_cast<uint32_t>(index);
                bits += 6;
                if (bits >= 8) {
                    bits -= 8;
                    png.push_back(static_cast<stbi_uc>(accumulated >> bits));
                }
            }
        }

        auto texture = std::make_shared<Ship::GuiTexture>();
        texture->Data = stbi_load_from_memory(png.data(), static_cast<int>(png.size()), &texture->Metadata.Width,
                                              &texture->Metadata.Height, nullptr, STBI_rgb_alpha);
        if (!texture->Data || texture->Metadata.Width != kAtlasWidth || texture->Metadata.Height != kAtlasHeight)
            return {};
        texture->DataSize = kAtlasWidth * kAtlasHeight * 4;
        gui.LoadTextureFromResource(name, texture);
        // LoadTextureFromResource leaves the atlas selected. Linear sampling
        // retains smooth edges as the glyph moves between screen pixels.
        interpreter.GetCurrentRenderingAPI()->SetSamplerParameters(0, true, 0, 0);
    }
    return gui.GetTextureByName(name);
}

} // namespace ComboRando::SongEvents::Atlas
