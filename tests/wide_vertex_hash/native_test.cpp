#include "fast/resource/type/Vertex.h"
#include "fast/lus_gbi.h"
#undef GIMMCMD
#include "libultraship/libultra/gbi.h"
#include "mm/2s2h/resource/type/Array.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>

using F3DGfx = Gfx;
using F3DVtx = Fast::F3DVtx;
constexpr unsigned MAX_VERTICES = 64;
static const F3DVtx* expected;
static int32_t expectedX = 50000, expectedY = 70000;
struct Interpreter {
    unsigned submissions = 0;
    void GfxSpVertex(unsigned count, unsigned destination, F3DVtx* pointer) {
        assert(pointer == expected && "hash command retained an old owner/resource pointer");
        assert(count == 3 && destination == 0);
        assert(pointer[0].v.ob[0] == expectedX && pointer[2].v.ob[1] == expectedY);
        ++submissions;
    }
};
struct Manager {
    std::shared_ptr<Ship::IResource> resource;
    unsigned lookups = 0;
    auto LoadResource(uint64_t hash) {
        assert(hash == 7);
        ++lookups;
        return resource;
    }
    void* GetResourceRawPointer(const std::shared_ptr<Ship::IResource>& value) {
        return value ? value->GetRawPointer() : nullptr;
    }
};
static std::shared_ptr<Manager> currentManager;
static auto ActiveResMgr() { return currentManager; }
class OtherResource final : public Ship::Resource<uint32_t> {
  public:
    OtherResource() : Resource(nullptr) {}
    uint32_t* GetPointer() override { return values.data(); }
    size_t GetPointerSize() override { return sizeof(values); }
    std::array<uint32_t, 24> values{};
};
class MalformedVertexSource final : public Ship::Resource<F3DVtx>, public Fast::IVertexSource {
  public:
    MalformedVertexSource() : Resource(nullptr) {}
    F3DVtx* GetPointer() override { return nullPayload ? nullptr : values.data(); }
    size_t GetPointerSize() override { return bytes; }
    uint32_t GetVertexRecordSize() const override { return 16; }
    std::array<F3DVtx, 3> values{};
    size_t bytes = sizeof(values);
    bool nullPayload = false;
};
#define C0(shift, width) ((cmd->words.w0 >> (shift)) & ((1U << (width)) - 1))
/* PRODUCTION_HANDLER */
#undef C0

static std::array<Gfx, 3> Packet(uintptr_t offset, unsigned count = 3, unsigned end = 3) {
    std::array<Gfx, 3> commands{};
    commands[0].words.w0 = (uintptr_t(G_VTX_OTR_HASH) << 24) | (count << 12) | (end << 1);
    commands[0].words.w1 = offset;
    commands[1].words.w1 = 7;
    commands[2].words.w0 = uintptr_t(G_ENDDL) << 24;
    return commands;
}

static void Execute(Interpreter& renderer, std::array<Gfx, 3>& commands, unsigned expectedSubmissions) {
    const auto before = commands;
    auto* command = commands.data();
    assert(!gfx_vtx_hash_handler_custom(&renderer, &command));
    assert(renderer.submissions == expectedSubmissions && command == commands.data() + 1);
    assert(std::memcmp(commands.data(), before.data(), sizeof(commands)) == 0 &&
           "serialized hash and byte-offset packets must remain immutable");
}

static std::shared_ptr<Fast::Vertex> Vertex(unsigned recordSize, size_t index, int32_t x = 50000, int32_t y = 70000) {
    auto resource = std::make_shared<Fast::Vertex>();
    resource->RecordSize = recordSize;
    resource->VertexList.resize(index + 3);
    resource->VertexList[index].v.ob[0] = x;
    resource->VertexList[index + 2].v.ob[1] = y;
    return resource;
}

static std::shared_ptr<SOH::Array> LegacyArray(size_t index, size_t declaredCount) {
    auto resource = std::make_shared<SOH::Array>();
    resource->ArrayType = SOH::ArrayResourceType::Vertex;
    resource->ArrayCount = declaredCount;
    resource->Vertices.resize(index + 3);
    resource->Vertices[index].v.ob[0] = 12000;
    resource->Vertices[index + 2].v.ob[1] = 19000;
    return resource;
}

static void Select(const std::shared_ptr<Ship::IResource>& resource, size_t index, int32_t x, int32_t y) {
    currentManager = std::make_shared<Manager>();
    currentManager->resource = resource;
    expected = reinterpret_cast<F3DVtx*>(resource->GetRawPointer()) + index;
    expectedX = x;
    expectedY = y;
}

static void Valid(unsigned recordSize, size_t index) {
    const auto resource = Vertex(recordSize, index);
    Select(resource, index, 50000, 70000);
    expected = reinterpret_cast<F3DVtx*>(resource->VertexList.data()) + index;
    auto commands = Packet(index * recordSize);
    Interpreter renderer;
    Execute(renderer, commands, 1);
    Execute(renderer, commands, 2);
    assert(currentManager->lookups == 2);
}

static void LegacyMM() {
    for (const size_t index : { size_t(1), size_t(65536) }) {
        auto resource = LegacyArray(index, index + 3);
        Select(resource, index, 12000, 19000);
        auto commands = Packet(index * 16);
        Interpreter renderer;
        Execute(renderer, commands, 1);
        Execute(renderer, commands, 2);
        // The runtime vector, rather than a stale declaration, owns its bounds.
        resource->ArrayCount = 0;
        Execute(renderer, commands, 3);
        assert(resource->GetPointerSize() == resource->Vertices.size() * sizeof(F3DVtx));
    }
}

static void Lifetime() {
    auto first = Vertex(16, 1);
    auto replacement = LegacyArray(1, 4); // Allocate before retiring the old vector.
    Select(first, 1, 50000, 70000);
    auto commands = Packet(16);
    Interpreter renderer;
    Execute(renderer, commands, 1);
    std::weak_ptr<Ship::IResource> retiredResource = first;
    first.reset();
    currentManager->resource = replacement;
    assert(retiredResource.expired());
    expected = replacement->Vertices.data() + 1;
    expectedX = 12000;
    expectedY = 19000;
    Execute(renderer, commands, 2); // Alt/resource replacement within the same owner.
    assert(currentManager->lookups == 2);

    auto second = Vertex(16, 1, -90000, 123456);
    std::weak_ptr<Manager> retiredOwner = currentManager;
    retiredResource = replacement;
    replacement.reset();
    Select(second, 1, -90000, 123456);
    assert(retiredOwner.expired() && retiredResource.expired());
    Execute(renderer, commands, 3); // A new route owns the same serialized hash.
    assert(currentManager->lookups == 1);
}

static void Refused(const std::shared_ptr<Ship::IResource>& resource, uintptr_t offset,
                    unsigned count = 3, unsigned end = 3) {
    currentManager = std::make_shared<Manager>();
    currentManager->resource = resource;
    auto commands = Packet(offset, count, end);
    Interpreter renderer;
    Execute(renderer, commands, 0);
}

static void Bounds() {
    Refused(Vertex(22, 0), 1);
    Refused(Vertex(22, 0), 22); // Request extends beyond the real vector.
    Refused(Vertex(16, 0), std::numeric_limits<uint32_t>::max());
    Refused(Vertex(16, 0), uintptr_t(std::numeric_limits<uint32_t>::max()) + 1);
    Refused(Vertex(0, 0), 0);
    Refused(Vertex(24, 0), 0); // Runtime stride is never a serialized record size.
    Refused(Vertex(16, 0), 0, 0, 0);
    Refused(Vertex(16, 0), 0, 3, 2);
    Refused(Vertex(16, 0), 0, 3, MAX_VERTICES + 1);
    Refused(nullptr, 0);
    Refused(std::make_shared<OtherResource>(), 0);
    auto malformed = std::make_shared<MalformedVertexSource>();
    ++malformed->bytes;
    Refused(malformed, 0); // Partial runtime record size is malformed.
    --malformed->bytes;
    malformed->nullPayload = true;
    Refused(malformed, 0);
    Refused(LegacyArray(0, 100000), 16); // Declared count cannot admit an OOB read.
    auto empty = LegacyArray(0, 100000);
    empty->Vertices.clear();
    Refused(empty, 0);
    auto scalar = LegacyArray(0, 3);
    scalar->ArrayType = SOH::ArrayResourceType::Scalar;
    scalar->ArrayScalarType = SOH::ScalarType::ZSCALAR_U16;
    scalar->Scalars.resize(64);
    Refused(scalar, 0);
    scalar->ArrayType = SOH::ArrayResourceType::Vector;
    Refused(scalar, 0);
    Refused(std::make_shared<SOH::Array>(), 0); // Default/untyped arrays expose no vertex provenance.
    currentManager.reset();
    auto commands = Packet(0);
    Interpreter renderer;
    Execute(renderer, commands, 0);
}

int main(int argc, char** argv) {
    static_assert(sizeof(Vtx) == sizeof(F3DVtx) && sizeof(Vtx) == 24);
    const auto run = [&](const char* name) { return argc == 1 || (argc == 2 && std::strcmp(argv[1], name) == 0); };
    if (run("offsets")) {
        Valid(22, 47663); // First v1 record above the old 0xfffff cutoff.
        Valid(16, 65536); // Legacy resources may also exceed one MiB.
    }
    if (run("packet"))
        Valid(16, 1);
    if (run("mm"))
        LegacyMM();
    if (run("lifetime"))
        Lifetime();
    if (run("bounds"))
        Bounds();
    std::puts("PASS production hash vertices: archive offsets, actual MM arrays, immutable packets, owner/Alt swaps and bounds");
}
