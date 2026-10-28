// Exercises the real transport policy. A clamped wide-world Y would put a
// signpost shadow at an unrelated floor; unrepresentable values must disarm
// the optional clamp while preserving the actor's captured geometry.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
using s16 = int16_t;
using f32 = float;
constexpr s16 TOON_SHADOW_NO_CLAMP = -32768;
/* PRODUCTION_SHADOW_FLOOR */
int main() {
    const struct { float floor; bool deepRooted; s16 want; } cases[] = {
        {123.75f, true, 123}, {-123.75f, true, -123},
        {32767, true, 32767}, {-32767, true, -32767},
        {50000, true, TOON_SHADOW_NO_CLAMP}, {-50000, true, TOON_SHADOW_NO_CLAMP},
        {-32768, true, TOON_SHADOW_NO_CLAMP}, {123, false, TOON_SHADOW_NO_CLAMP},
        {std::numeric_limits<float>::infinity(), true, TOON_SHADOW_NO_CLAMP},
        {std::numeric_limits<float>::quiet_NaN(), true, TOON_SHADOW_NO_CLAMP},
    };
    for (const auto& test : cases) {
        auto got = ToonShadowFeetClamp(test.floor, test.deepRooted);
        if (got != test.want) { std::fprintf(stderr, "FAIL: floor %g deep=%d got %d want %d\n", test.floor, test.deepRooted, got, test.want); return 1; }
    }
    std::puts("cel shadow floor transport: PASS");
}
