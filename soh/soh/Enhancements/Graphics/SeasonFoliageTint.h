#pragma once

#include <fast/resource/type/Texture.h>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

namespace SeasonFoliage {
enum class Surface { Ground, Canopy, Understory };
enum class PixelMask { Green, GrassTrim };
struct Tint {
    uint32_t color = 0;
    unsigned lightScale = 256;
};
// Atlas bounds in sixteenths, independent of native/HD texture dimensions.
// Defaults select the whole texture; authored exceptions protect nearby bark/stone.
struct Region {
    unsigned left = 0, top = 0, right = 16, bottom = 16;
};

inline Tint Palette(int season, Surface surface, uint32_t autumnColor = 0) {
    switch (season) {
        case 0:
            return { surface == Surface::Canopy ? 0xF6C1D9E0u : 0xF6C1D9FFu, 461 };
        case 2:
            return { autumnColor ? autumnColor : surface == Surface::Canopy ? 0xF4B783FFu : 0xFFD078FFu, 256 };
        case 3:
            return { surface == Surface::Canopy ? 0xCCDCEEFFu : 0xE5EDF3FFu, 461 };
        default:
            return {};
    }
}

// Tint copies are made from the original texture, never the preceding season.
// Djipi's RGBA16 resources with LOAD_AS_RAW carry four RGBA bytes per pixel.
inline std::shared_ptr<Fast::Texture> BuildTexture(const std::shared_ptr<Fast::Texture>& source, Tint tint,
                                                   Region region = {}, PixelMask pixelMask = PixelMask::Green) {
    if (!source || !tint.color || !source->ImageData || !source->Width || !source->Height)
        return nullptr;
    if (region.left >= region.right || region.top >= region.bottom || region.right > 16 || region.bottom > 16)
        return nullptr;
    const bool packed = source->Type == Fast::TextureType::RGBA16bpp && source->Flags == 0;
    const bool raw = (source->Flags & TEX_FLAG_LOAD_AS_RAW) != 0;
    const bool rgba32 = source->Type == Fast::TextureType::RGBA32bpp && source->Flags == 0;
    if (!packed && !raw && !rgba32)
        return nullptr;
    const size_t stride = packed ? 2 : 4;
    if (size_t(source->Width) * source->Height * stride != source->ImageDataSize)
        return nullptr;
    const unsigned red = tint.color >> 24;
    const unsigned green = (tint.color >> 16) & 255;
    const unsigned blue = (tint.color >> 8) & 255;
    const unsigned blend = tint.color & 255;
    const unsigned targetSum = red + green + blue;
    if (!targetSum || !blend)
        return nullptr;
    auto copy = std::make_shared<Fast::Texture>(source->GetInitData());
    copy->Type = source->Type;
    copy->Flags = source->Flags;
    copy->Width = source->Width;
    copy->Height = source->Height;
    copy->HByteScale = source->HByteScale;
    copy->VPixelScale = source->VPixelScale;
    copy->ImageDataSize = source->ImageDataSize;
    const auto* first = reinterpret_cast<const char*>(source->ImageData);
    copy->mImageBuffer = std::make_shared<std::vector<char>>(first, first + source->ImageDataSize);
    copy->ImageData = reinterpret_cast<uint8_t*>(copy->mImageBuffer->data());
    bool changed = false;
    const bool wholeTexture = region.left == 0 && region.top == 0 && region.right == 16 && region.bottom == 16;
    for (size_t i = 0; i < copy->ImageDataSize; i += stride) {
        if (!wholeTexture) {
            const size_t pixelIndex = i / stride;
            const unsigned x = pixelIndex % source->Width, y = pixelIndex / source->Width;
            if (x * 16 < source->Width * region.left || x * 16 >= source->Width * region.right ||
                y * 16 < source->Height * region.top || y * 16 >= source->Height * region.bottom)
                continue;
        }
        auto* pixel = copy->ImageData + i;
        const unsigned native = packed ? (unsigned(pixel[0]) << 8) | pixel[1] : 0;
        const unsigned r = packed ? native >> 11 : pixel[0];
        const unsigned g = packed ? (native >> 6) & 31 : pixel[1];
        const unsigned b = packed ? (native >> 1) & 31 : pixel[2];
        if (g <= r || g <= b)
            continue; // Rocks, bark, flowers and alpha retain their exact source pixels.
        // The Field paving's gray stone is slightly green too. Select its
        // saturated seam grass without recoloring those stone faces. Match the
        // same separation in native 5-bit and raw/HD 8-bit channels.
        if (pixelMask == PixelMask::GrassTrim &&
            (g * 100 <= r * 120 || g * 100 <= b * 120 || g - r < (packed ? 2u : 10u)))
            continue;
        const auto channel = [&](unsigned original, unsigned target) {
            const unsigned normalized =
                std::min(packed ? 31u : 255u, (r + g + b) * target * tint.lightScale / (targetSum * 256));
            return (original * (255 - blend) + normalized * blend) / 255;
        };
        const unsigned tr = channel(r, red), tg = channel(g, green), tb = channel(b, blue);
        if (packed) {
            const unsigned tinted = (tr << 11) | (tg << 6) | (tb << 1) | (native & 1);
            pixel[0] = tinted >> 8;
            pixel[1] = tinted & 255;
        } else {
            pixel[0] = tr;
            pixel[1] = tg;
            pixel[2] = tb;
        }
        changed |= tr != r || tg != g || tb != b;
    }
    return changed ? copy : nullptr;
}
} // namespace SeasonFoliage
