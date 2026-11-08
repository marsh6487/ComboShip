#pragma once

#include "SeasonFoliageTint.h"

namespace SeasonFoliage {
// Lift every material color toward white without discarding luminance. Black
// maps to 168/255; white stays white. Alpha and surface shading remain separate.
inline unsigned SnowBrightness(unsigned value, unsigned maximum) {
    return (168 * maximum + 87 * value) / 255;
}

inline std::shared_ptr<Fast::Texture> BuildWinterTerrainTexture(const std::shared_ptr<Fast::Texture>& source,
                                                                Region region = {}) {
    if (!source || !source->ImageData || !source->Width || !source->Height || region.left >= region.right ||
        region.top >= region.bottom || region.right > 16 || region.bottom > 16)
        return nullptr;
    const auto type = source->Type;
    const bool indexed = type == Fast::TextureType::Palette4bpp || type == Fast::TextureType::Palette8bpp;
    // Match ImportTexture: IMG is decoded RGBA and takes precedence. RAW CI
    // still routes through the CI importer/TLUT; only non-indexed RAW is RGBA.
    const bool rgba = (source->Flags & TEX_FLAG_LOAD_AS_IMG) != 0 ||
                      ((source->Flags & TEX_FLAG_LOAD_AS_RAW) != 0 && !indexed) || type == Fast::TextureType::RGBA32bpp;
    unsigned bits = rgba                                                                                     ? 32
                    : type == Fast::TextureType::RGBA16bpp || type == Fast::TextureType::GrayscaleAlpha16bpp ? 16
                    : type == Fast::TextureType::Grayscale8bpp || type == Fast::TextureType::GrayscaleAlpha8bpp ||
                            type == Fast::TextureType::Palette8bpp
                        ? 8
                    : type == Fast::TextureType::Grayscale4bpp || type == Fast::TextureType::GrayscaleAlpha4bpp ||
                            type == Fast::TextureType::Palette4bpp
                        ? 4
                        : 0;
    const size_t pixels = size_t(source->Width) * source->Height;
    if (!bits || (pixels * bits + 7) / 8 != source->ImageDataSize)
        return nullptr;
    auto copy = std::make_shared<Fast::Texture>(source->GetInitData());
    copy->Type = type;
    copy->Flags = source->Flags;
    copy->Width = source->Width;
    copy->Height = source->Height;
    copy->HByteScale = source->HByteScale;
    copy->VPixelScale = source->VPixelScale;
    copy->ImageDataSize = source->ImageDataSize;
    const auto* first = reinterpret_cast<const char*>(source->ImageData);
    copy->mImageBuffer = std::make_shared<std::vector<char>>(first, first + source->ImageDataSize);
    copy->ImageData = reinterpret_cast<uint8_t*>(copy->mImageBuffer->data());
    // A mixed atlas must not neutralize warm shading on its protected wood.
    copy->NeutralizeShading = region.left == 0 && region.top == 0 && region.right == 16 && region.bottom == 16;
    for (size_t i = 0; i < pixels; ++i) {
        const unsigned x = i % source->Width, y = i / source->Width;
        if (x * 16 < source->Width * region.left || x * 16 >= source->Width * region.right ||
            y * 16 < source->Height * region.top || y * 16 >= source->Height * region.bottom)
            continue;
        auto* p = copy->ImageData + i * bits / 8;
        if (rgba) {
            const unsigned snow = SnowBrightness((unsigned(p[0]) + p[1] + p[2]) / 3, 255);
            p[0] = p[1] = p[2] = snow;
        } else if (type == Fast::TextureType::RGBA16bpp) {
            const unsigned value = (unsigned(p[0]) << 8) | p[1];
            const unsigned light = ((value >> 11) + ((value >> 6) & 31) + ((value >> 1) & 31)) / 3;
            const unsigned snow = SnowBrightness(light, 31);
            const unsigned tinted = (snow << 11) | (snow << 6) | (snow << 1) | (value & 1);
            p[0] = tinted >> 8;
            p[1] = tinted;
        } else if (type == Fast::TextureType::Grayscale8bpp || type == Fast::TextureType::GrayscaleAlpha16bpp) {
            p[0] = SnowBrightness(p[0], 255);
        } else if (type == Fast::TextureType::GrayscaleAlpha8bpp) {
            p[0] = (SnowBrightness(p[0] >> 4, 15) << 4) | (p[0] & 15);
        } else if (type == Fast::TextureType::Grayscale4bpp || type == Fast::TextureType::GrayscaleAlpha4bpp) {
            const unsigned shift = (i & 1) ? 0 : 4;
            const unsigned nibble = (*p >> shift) & 15;
            const bool alpha = type == Fast::TextureType::GrayscaleAlpha4bpp;
            const unsigned snow =
                alpha ? (SnowBrightness(nibble >> 1, 7) << 1) | (nibble & 1) : SnowBrightness(nibble, 15);
            *p = (*p & ~(15 << shift)) | (snow << shift);
        }
        // CI indices stay intact. Their reviewed dedicated TLUT is tinted as a
        // second material; the CI copy carries the draw's shading marker. RGBA
        // replacements at either resource identity use the same formula above.
    }
    return copy;
}
} // namespace SeasonFoliage
