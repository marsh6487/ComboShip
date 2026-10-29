#include "global.h"
#include "overlays/actors/ovl_En_Light/z_en_light.h"
#include "overlays/actors/ovl_En_M_Thunder/z_en_m_thunder.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include "light_source_types.inc"

static struct { LightInfo barrierLightInfo; } gFormState;
static LightInfo sUhLightInfo, sLanternLightInfo, domRodLightInfo;
static Vec3f domRodOrbPos, sRitoWindOrigin;
static LightInfo* submitted;
static LightInfo captured;
static int submissions;
extern "C" void Lights_PointNoGlowSetInfo(LightInfo* info, f32 x, f32 y, f32 z, u8 r, u8 g, u8 b, s16 radius) {
    submitted = info;
    ++submissions;
    info->type = LIGHT_POINT_NOGLOW;
    info->params.point.x = x;
    info->params.point.y = y;
    info->params.point.z = z;
    info->params.point.color[0] = r;
    info->params.point.color[1] = g;
    info->params.point.color[2] = b;
    info->params.point.radius = radius;
    captured = *info;
}
extern "C" f32 Math_SinS(s16 yaw) { assert(yaw == 0); return 0; }
extern "C" f32 Math_CosS(s16 yaw) { assert(yaw == 0); return 1; }

static void ExecuteCall(size_t index, const Vec3f& position) {
    Player playerStorage{};
    Player* player = &playerStorage;
    player->actor.world.pos = position;
    EnMThunder thunder{};
    EnMThunder* enMThunder = &thunder;
    enMThunder->actor.world.pos = position;
    MagicLight spell{};
    MagicLight* self = &spell; // Bind the native C identifier `this` for C++.
    self->actor.world.pos = position;
    EnLight flameStorage{};
    Actor* flame = &flameStorage.actor;
    Vec3f lightPos = position;
    Vec3f point = position;
    Vec3f* pos = &point;
    domRodOrbPos = sRitoWindOrigin = position;
    const f32 cosA = 7.25f, sinB = -11.5f, cosB = 23.75f;
    const f32 pulse = .75f, blueRadius = .5f, temp = .5f;
    const s32 redGreen = 127;
    const u8 r = 93, g = 147, b = 201;
    const s16 radius = 401;
#include "light_calls.inc"
}

int main() {
    struct Expected {
        Vec3f offset;
        u8 r, g, b;
        s16 radius;
    };
    // Independent authored expectations: orbit inputs are explicit fixture
    // values; SW97's two lights retain their 55/20-unit placements at yaw zero.
    constexpr Expected expected[] = {
        { { 0, 0, 0 }, 100, 200, 255, 600 },
        { { 7.25f, -11.5f, 23.75f }, 100, 200, 255, 600 },
        { { 0, 0, 0 }, 41, 191, 120, 340 },
        { { 0, 0, 0 }, 93, 147, 201, 401 },
        { { 0, 0, 0 }, 255, 215, 50, 200 },
        { { 0, 0, 0 }, 255, 215, 50, 200 },
        { { 0, 0, 0 }, 0, 0, 0, 0 },
        { { 0, 0, 0 }, 127, 127, 50, 400 },
        { { 0, 55, 0 }, 127, 127, 100, 50 },
        { { 0, 20, 20 }, 127, 127, 100, 50 },
    };
    static_assert(std::size(expected) == std::size(kLightCallNames));
    int failures = 0;
    for (size_t call = 0; call < std::size(expected); ++call) {
        for (const Vec3f& point : { Vec3f{ 70000.25f, -70000.5f, 12345.75f },
                                   Vec3f{ -70000.75f, 70000.5f, -12345.25f },
                                   Vec3f{ 123.25f, -456.5f, 789.75f } }) {
            submissions = 0;
            submitted = nullptr;
            ExecuteCall(call, point);
            assert(submissions == 1 && submitted);
            const auto& actual = captured.params.point;
            const auto& control = expected[call];
            const Vec3f wanted{ point.x + control.offset.x, point.y + control.offset.y, point.z + control.offset.z };
            const bool positionMatches = actual.x == wanted.x && actual.y == wanted.y && actual.z == wanted.z;
            if (!positionMatches) {
                ++failures;
                std::fprintf(stderr, "FAIL %s expected %.4f %.4f %.4f; received %.4f %.4f %.4f\n", kLightCallNames[call],
                             wanted.x, wanted.y, wanted.z, actual.x, actual.y, actual.z);
            }
            assert(actual.color[0] == control.r && actual.color[1] == control.g && actual.color[2] == control.b &&
                   actual.radius == control.radius);
        }
    }
    if (failures)
        return 1;
    std::puts("PASS all ten OoT light calls: native float API, fractional +/-70000/vanilla positions, authored offsets/colors/radii");
}
