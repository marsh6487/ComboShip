#include "../../soh/mods/transformation_masks/mm_form_blink_pixels.h"
#include <array>
#include <cassert>
#include <algorithm>
using namespace MmFormBlink;
int main() {
    std::array<uint8_t, Bytes> source{}, half{}, closed{};
    // An asymmetric cutout verifies that closure follows each actual column,
    // rather than adding an assumed rectangular lid over the head geometry.
    for (size_t x = 4; x < 60; ++x)
        for (size_t y = 4 + x % 5; y < 25 - x % 3; ++y)
            Write(source.data() + (y * Width + x) * 2, 0xFFC1);
    assert(Derive(source.data(), Bytes, true, 1, half.data()));
    assert(Derive(source.data(), Bytes, true, 2, closed.data()));
    size_t a = 0, b = 0, c = 0;
    for (size_t i = 0; i < Bytes; i += 2) {
        uint16_t s = Read(source.data() + i), h = Read(half.data() + i), z = Read(closed.data() + i);
        assert((h & ~1) == (s & ~1) && (z & ~1) == (s & ~1));
        assert(!(h & 1) || (s & 1)); assert(!(z & 1) || (s & 1));
        a += s & 1; b += h & 1; c += z & 1;
    }
    assert(a > b && b > c && c > 0);
    std::fill(source.begin(), source.end(), 255);
    // Color and alpha invariants on the FD socket/reference mask.
    Write(source.data() + (22 * Width + 6) * 2, 0xF801); // red cheek marking
    Write(source.data() + (15 * Width + 30) * 2, 0x003F); // blue forehead marking
    assert(Derive(source.data(), Bytes, false, 2, closed.data()));
    for (size_t y = 0; y < Height; ++y) for (size_t x = 0; x < Width; ++x) {
        size_t i = (y * Width + x) * 2;
        assert((Read(source.data() + i) & 1) == (Read(closed.data() + i) & 1));
        if (x < 5 || x > 58 || (x > 26 && x < 37) || y < 15 || y > 25)
            assert(Read(source.data() + i) == Read(closed.data() + i));
    }
    assert(Read(closed.data() + (22 * Width + 6) * 2) == 0xF801);
    assert(Read(closed.data() + (15 * Width + 30) * 2) == 0x003F);
    assert(!Derive(source.data(), Bytes - 1, true, 1, half.data()));
    assert(!Derive(nullptr, Bytes, true, 1, half.data()));
    assert(!Derive(source.data(), Bytes, true, 0, half.data()));
}
