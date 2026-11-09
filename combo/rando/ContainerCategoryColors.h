#pragma once
#include <cstdint>
namespace ComboRando::ContainerCategoryColors {
// The first seven identities and all three palettes preserve OoT's existing
// particle CSMC colors. MM has two additional container-art categories.
enum class Category : uint8_t { Junk, Lesser, Health, BossKey, SmallKey, Token, Major, Mask, StrayFairy };
struct Color {
    uint8_t r, g, b, a;
};
inline constexpr Color Primary[] = {
    { 255, 255, 255, 255 }, { 170, 50, 0, 255 },    { 250, 0, 0, 255 },
    { 255, 255, 0, 255 },   { 180, 180, 180, 255 }, { 0, 0, 0, 255 },
    { 250, 185, 40, 255 },  { 160, 80, 255, 255 },  { 255, 120, 220, 255 },
};
inline constexpr Color Secondary[] = {
    { 255, 255, 255, 255 }, { 250, 75, 0, 255 },    { 0, 0, 255, 255 },
    { 0, 200, 255, 255 },   { 130, 130, 130, 255 }, { 255, 250, 190, 255 },
    { 255, 220, 135, 255 }, { 230, 200, 255, 255 }, { 255, 210, 245, 255 },
};
inline constexpr Color Flare[] = {
    { 135, 135, 135, 255 }, { 255, 160, 100, 255 }, { 255, 125, 125, 255 },
    { 0, 200, 255, 255 },   { 100, 100, 100, 255 }, { 255, 255, 255, 255 },
    { 250, 220, 180, 255 }, { 200, 160, 255, 255 }, { 255, 170, 230, 255 },
};
inline constexpr Color Get(Category category, int component = 0) {
    const auto index = static_cast<unsigned>(category);
    const auto safe = index < 9 ? index : static_cast<unsigned>(Category::Major);
    return component == 1 ? Secondary[safe] : component == 2 ? Flare[safe] : Primary[safe];
}
} // namespace ComboRando::ContainerCategoryColors
