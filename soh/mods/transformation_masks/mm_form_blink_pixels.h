#ifndef MM_FORM_BLINK_PIXELS_H
#define MM_FORM_BLINK_PIXELS_H

#include <cstddef>
#include <cstdint>
#include <cstring>

// Native eye resources are big-endian RGBA5551, 64 x 32. Keep the derivation
// independent of renderer/resource ownership so its preservation rules are tested.
namespace MmFormBlink {
constexpr size_t Width = 64, Height = 32, Bytes = Width * Height * 2;
inline uint16_t Read(const uint8_t* p) {
    return (uint16_t(p[0]) << 8) | p[1];
}
inline void Write(uint8_t* p, uint16_t c) {
    p[0] = c >> 8;
    p[1] = c & 255;
}

inline bool Derive(const uint8_t* source, size_t size, bool deku, unsigned state, uint8_t* target) {
    if (!source || !target || size != Bytes || state < 1 || state > 2)
        return false;
    std::memcpy(target, source, Bytes);
    if (deku) {
        size_t opaque = 0;
        for (size_t i = 0; i < Bytes; i += 2)
            opaque += Read(source + i) & 1;
        if (opaque == 0 || opaque == Width * Height)
            return false;
        // The native Deku eye is an alpha-cutout material. Lower its visible
        // opening within each column; never add opacity outside the source.
        for (size_t x = 0; x < Width; ++x) {
            int first = -1, last = -1;
            for (size_t y = 0; y < Height; ++y) {
                if (Read(source + (y * Width + x) * 2) & 1) {
                    if (first < 0)
                        first = int(y);
                    last = int(y);
                }
            }
            if (first < 0)
                continue;
            const int start = state == 1 ? first + (last - first + 1) / 2 : last;
            for (int y = first; y < start; ++y) {
                auto* p = target + (y * Width + x) * 2;
                Write(p, Read(p) & ~uint16_t(1));
            }
        }
    } else {
        // Bounds measured from the actual native 64x32 FD atlas. Keep brows
        // above the sloping upper lash and cheek/forehead markings outside it.
        // A flat cheek pixel supplies skin; the existing inner lash supplies
        // the closed seam color. No donor-model UV assumptions enter this mask.
        constexpr int topByColumn[22] = { 15, 15, 16, 16, 16, 16, 16, 16, 17, 17, 17,
                                          18, 18, 19, 19, 20, 20, 21, 22, 23, 24, 26 };
        const uint16_t skin = Read(source + (30 * Width + 20) * 2);
        const uint16_t seam = Read(source + (26 * Width + 25) * 2);
        for (size_t x = 5; x <= 26; ++x) {
            const int top = topByColumn[x - 5];
            for (size_t mirror = 0; mirror < 2; ++mirror) {
                const size_t px = mirror ? Width - 1 - x : x;
                constexpr int outerBottom[5] = { 19, 21, 22, 23, 24 };
                const int bottom = x < 10 ? outerBottom[x - 5] : 25;
                const int halfway = (top + 26) / 2;
                const int end = state == 1 && halfway < bottom ? halfway : bottom;
                for (int y = top; y <= end; ++y) {
                    const size_t offset = (y * Width + px) * 2;
                    const uint16_t c = Read(source + offset);
                    Write(target + offset, (skin & ~uint16_t(1)) | (c & 1));
                }
                if (state == 2 && x >= 10 && x <= 25) {
                    const size_t offset = (25 * Width + px) * 2;
                    Write(target + offset, (seam & ~uint16_t(1)) | (Read(source + offset) & 1));
                }
            }
        }
    }
    return true;
}
} // namespace MmFormBlink
#endif
