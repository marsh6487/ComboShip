#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <utility>
#include <vector>
#include "libultraship/libultra/types.h"
#include "libultraship/libultra/gbi.h"
#include "fast/lus_gbi.h"
#include "production_shader.inc"

static unsigned checks;
#define CHECK(test) do { ++checks; if (!(test)) { \
    std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #test); std::abort(); \
} } while (0)

namespace Fast {

// Submission boundaries observe state at the point production handlers call
// them. They do not decode commands or implement toon/shadow policy.
struct FixtureRsp {
    bool toon_key_valid = true;
    float toon_key_dir[3] = {0.0f, 1.0f, 0.0f};
    float toon_key_color[3] = {0.25f, 0.5f, 0.75f};
    bool lights_changed = false;
    float toon_shadow_size = 0.25f;
    float toon_shadow_dir[3] = {1.0f, 0.0f, 0.0f};
    bool toon_shadow_clamp_feet = true;
    float toon_shadow_feet_clamp_y = 42.0f;
};
struct FixtureRdp {
    bool toon = true;
    bool toon_shadow = true;
};
enum class Boundary { GeometryFlush, ShadowFlush, ShadowRender, Stencil };
struct Event {
    Boundary kind;
    FixtureRsp rsp;
    FixtureRdp rdp;
    int pendingTriangles;
    int stencilMode;
    int argument;
};
class Interpreter;
struct FixtureBackend {
    Interpreter* owner;
    int stencilMode = 2;
    void SetStencilMode(int mode);
};
class Interpreter {
  public:
    FixtureRsp rsp;
    FixtureRdp rdp;
    FixtureBackend backend{this};
    FixtureRsp* mRsp = &rsp;
    FixtureRdp* mRdp = &rdp;
    FixtureBackend* mRapi = &backend;
    int pendingTriangles = 7;
    std::vector<Event> events;
    void Record(Boundary kind, int argument = 0) {
        events.push_back({kind, rsp, rdp, pendingTriangles, backend.stencilMode, argument});
    }
    void Flush() {
        Record(Boundary::GeometryFlush);
        pendingTriangles = 0;
    }
    void FlushToonShadow() { Record(Boundary::ShadowFlush); }
    void RenderShadowVolumes() { Record(Boundary::ShadowRender); }
};
void FixtureBackend::SetStencilMode(int mode) {
    owner->Record(Boundary::Stencil, mode);
    stencilMode = mode;
}

// A real, different global target catches handlers that accidentally go through
// the old singleton instead of operating on the explicit Interpreter* argument.
std::weak_ptr<Interpreter> mInstance;

#include "production_handlers.inc"

using Handler = bool (*)(Interpreter*, F3DGfx**);
struct Registration { const char* name; Handler handler; };
static const std::pair<int8_t, Registration> registrations[] = {
#include "production_registrations.inc"
};

static void Execute(Interpreter& target, const Gfx& native) {
    F3DGfx command{};
    command.words.w0 = native.words.w0;
    command.words.w1 = native.words.w1;
    F3DGfx* cursor = &command;
    unsigned found = 0;
    for (const auto& entry : registrations) {
        if (static_cast<uint8_t>(entry.first) == (command.words.w0 >> 24)) {
            CHECK(!entry.second.handler(&target, &cursor));
            found++;
        }
    }
    CHECK(found == 1);
    CHECK(cursor == &command);
}
} // namespace Fast

static void CheckVector(const float* actual, const std::array<float, 3>& expected) {
    for (size_t i = 0; i < expected.size(); i++) {
        CHECK(std::fabs(actual[i] - expected[i]) < 0.000001f);
    }
}

static void TestKeyBoundary() {
    Fast::Interpreter target;
    Gfx command{};
    gSPToonKey(&command, -127, 64, 0, 17, 128, 255);
    Fast::Execute(target, command);
    CHECK(target.events.size() == 1);
    const auto& flush = target.events[0];
    CHECK(flush.kind == Fast::Boundary::GeometryFlush && flush.pendingTriangles == 7);
    CheckVector(flush.rsp.toon_key_dir, {0, 1, 0});
    CheckVector(flush.rsp.toon_key_color, {0.25f, 0.5f, 0.75f});
    CheckVector(target.rsp.toon_key_dir, {-1, 64.0f / 127.0f, 0});
    CheckVector(target.rsp.toon_key_color, {17.0f / 255.0f, 128.0f / 255.0f, 1});
    CHECK(target.pendingTriangles == 0);
    CHECK(target.rsp.toon_key_valid && target.rsp.lights_changed);
    CHECK(target.rdp.toon && target.rdp.toon_shadow);
}

static void TestToonBoundary() {
    for (bool previous : {false, true}) {
        for (bool requested : {false, true}) {
            Fast::Interpreter target;
            target.rdp.toon = previous;
            Gfx command{};
            gSPToon(&command, requested);
            Fast::Execute(target, command);
            CHECK(target.events.size() == 2);
            CHECK(target.events[0].kind == Fast::Boundary::GeometryFlush);
            CHECK(target.events[0].pendingTriangles == 7);
            CHECK(target.events[0].rdp.toon == previous);
            CHECK(target.events[1].kind == Fast::Boundary::ShadowFlush);
            CHECK(target.events[1].pendingTriangles == 0);
            CHECK(target.events[1].rdp.toon == previous);
            CHECK(target.events[1].rdp.toon_shadow && target.events[1].rsp.toon_key_valid);
            CHECK(target.rdp.toon == requested);
            CHECK(!target.rdp.toon_shadow && !target.rsp.toon_key_valid);
        }
    }
}

static void TestShadowBoundary() {
    Fast::Interpreter target;
    Gfx command{};
    gSPToonShadowArm(&command, -1234, 0.75f);
    Fast::Execute(target, command);
    CHECK(target.events.size() == 1 && target.events[0].kind == Fast::Boundary::ShadowFlush);
    CHECK(target.events[0].rsp.toon_shadow_size == 0.25f);
    CHECK(target.events[0].rsp.toon_shadow_feet_clamp_y == 42.0f);
    CHECK(target.rdp.toon_shadow && target.rsp.toon_shadow_size == 0.75f);
    CHECK(target.rsp.toon_shadow_clamp_feet && target.rsp.toon_shadow_feet_clamp_y == -1234.0f);
    CheckVector(target.rsp.toon_shadow_dir, {0, 1, 0});

    // A later live key cannot change the prior object's deferred shadow key.
    gSPToonKey(&command, 127, 0, 0, 255, 255, 255);
    Fast::Execute(target, command);
    CheckVector(target.rsp.toon_shadow_dir, {0, 1, 0});
    target.events.clear();
    gSPToonShadow(&command, 0, 0, 0, 0.0f);
    Fast::Execute(target, command);
    CHECK(target.events.size() == 1 && target.events[0].kind == Fast::Boundary::ShadowFlush);
    CHECK(target.events[0].rdp.toon_shadow);
    CheckVector(target.events[0].rsp.toon_shadow_dir, {0, 1, 0});
    CHECK(!target.rdp.toon_shadow);

    target.events.clear();
    gSPToonShadowArm(&command, TOON_SHADOW_NO_CLAMP, 1.0f);
    Fast::Execute(target, command);
    CHECK(target.rdp.toon_shadow && !target.rsp.toon_shadow_clamp_feet);
    CHECK(target.rsp.toon_shadow_size == 1.0f);
    CheckVector(target.rsp.toon_shadow_dir, {1, 0, 0});

    // The flush sentinel renders accumulated volumes, with no new object arm or
    // overwrite of its saved parameters. Volume geometry itself is tested by root.
    target.events.clear();
    gSPToonShadowFlush(&command);
    Fast::Execute(target, command);
    CHECK(target.events.size() == 1 && target.events[0].kind == Fast::Boundary::ShadowRender);
    CHECK(target.rdp.toon_shadow && !target.rsp.toon_shadow_clamp_feet);
    CHECK(target.rsp.toon_shadow_size == 1.0f);
    CHECK(target.rsp.toon_shadow_feet_clamp_y == -32768.0f);
    CheckVector(target.rsp.toon_shadow_dir, {1, 0, 0});
}

static void TestStencilBoundary() {
    Fast::Interpreter target;
    Gfx command{};
    gSPStencil(&command, 5);
    Fast::Execute(target, command);
    CHECK(target.events.size() == 2);
    CHECK(target.events[0].kind == Fast::Boundary::GeometryFlush);
    CHECK(target.events[0].stencilMode == 2 && target.events[0].pendingTriangles == 7);
    CHECK(target.events[1].kind == Fast::Boundary::Stencil);
    CHECK(target.events[1].pendingTriangles == 0 && target.events[1].argument == 5);
    CHECK(target.backend.stencilMode == 5);
    CHECK(target.rdp.toon && target.rdp.toon_shadow && target.rsp.toon_key_valid);
}

static void TestShaderIds() {
    CHECK(SHADER_OPT(TOON) == (uint64_t{1} << 33));
    for (unsigned bit = 0; bit < 33; bit++) {
        CCFeatures features{};
        gfx_cc_get_features(0, uint64_t{1} << bit, &features);
        CHECK(!features.opt_toon);
    }
    CCFeatures combined{};
    gfx_cc_get_features(0, (uint64_t{1} << 33) - 1, &combined);
    CHECK(!combined.opt_toon && combined.shader_id == -1);
    for (uint32_t bits = 0; bits <= 0xFFFF; bits++) {
        const auto id = static_cast<int16_t>(bits);
        const uint64_t packed = Fast::ShaderIdMask(id);
        CHECK(packed == (uint64_t{bits} << 17));
        CHECK((packed & (uint64_t{1} << 33)) == 0);
        CHECK(Fast::ShaderIdUnmask(packed) == id);
        CCFeatures features{};
        gfx_cc_get_features(0, packed, &features);
        CHECK(features.shader_id == id && !features.opt_toon);
        gfx_cc_get_features(0, packed | SHADER_OPT(TOON), &features);
        CHECK(features.shader_id == id && features.opt_toon);
    }
}

static void TestOpcodes() {
    struct Opcode { const char* name; uint8_t value; };
    const Opcode all[] = {
#include "production_opcodes.inc"
    };
    for (size_t i = 0; i < std::size(all); i++) {
        for (size_t j = i + 1; j < std::size(all); j++) {
            if (all[i].value == all[j].value) {
                std::fprintf(stderr, "opcode collision: %s / %s = 0x%02x\n", all[i].name, all[j].name, all[i].value);
            }
            CHECK(all[i].value != all[j].value);
        }
    }
    CHECK(G_FOGF == 0x41 && static_cast<uint8_t>(Fast::OTR_G_FOGF) == G_FOGF);
    CHECK(G_SETTOON == 0x4c && static_cast<uint8_t>(Fast::OTR_G_SETTOON) == G_SETTOON);
    CHECK(G_SETTOONKEY == 0x4d && static_cast<uint8_t>(Fast::OTR_G_SETTOONKEY) == G_SETTOONKEY);
    CHECK(G_SETTOONSHADOW == 0x4e && static_cast<uint8_t>(Fast::OTR_G_SETTOONSHADOW) == G_SETTOONSHADOW);
    CHECK(G_SETSTENCIL == 0x4f && static_cast<uint8_t>(Fast::OTR_G_SETSTENCIL) == G_SETSTENCIL);
    Gfx command{};
    float factors[2] = {1.25f, -100000.0f};
    gSPFogFactorF(&command, factors);
    CHECK((command.words.w0 >> 24) == G_FOGF && command.words.w1 == reinterpret_cast<uintptr_t>(factors));
    gSPToon(&command, true);
    CHECK((command.words.w0 >> 24) == G_SETTOON);
    gSPToonKey(&command, 0, 127, 0, 255, 128, 0);
    CHECK((command.words.w0 >> 24) == G_SETTOONKEY);
    gSPToonShadowArm(&command, 0, 0.5f);
    CHECK((command.words.w0 >> 24) == G_SETTOONSHADOW);
    gSPStencil(&command, 0);
    CHECK((command.words.w0 >> 24) == G_SETSTENCIL);
}

int main() {
    auto global = std::make_shared<Fast::Interpreter>();
    global->rsp.toon_key_dir[0] = 0.875f;
    global->backend.stencilMode = 3;
    Fast::mInstance = global;
    TestKeyBoundary();
    TestToonBoundary();
    TestShadowBoundary();
    TestStencilBoundary();
    CHECK(global->events.empty());
    CHECK(global->rsp.toon_key_dir[0] == 0.875f && global->backend.stencilMode == 3);
    CHECK(global->pendingTriangles == 7);
    TestShaderIds();
    TestOpcodes();
    std::printf("PASS cel interpreter: %u checks; explicit instance handlers, flush boundaries, all 65536 shader IDs, opcode ownership\n", checks);
}
