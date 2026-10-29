#include "BgCheckStateSnapshot.h"

#include <libultraship/libultra.h>
#include "z64math.h"
#include "z64bgcheck.h"
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace {
using Bytes = std::vector<unsigned char>;

size_t ByteCount(uint64_t count, size_t width) {
    if (width == 0 || count > SIZE_MAX / width) {
        throw std::length_error("collision snapshot size");
    }
    return (size_t)count * width;
}

Bytes Copy(const void* pointer, size_t size) {
    if (size != 0 && pointer == nullptr) {
        throw std::invalid_argument("unallocated collision table");
    }
    Bytes bytes(size);
    if (size != 0) {
        std::memcpy(bytes.data(), pointer, size);
    }
    return bytes;
}

struct FreeBytes {
    void operator()(void* pointer) const noexcept { std::free(pointer); }
};
using Allocation = std::unique_ptr<void, FreeBytes>;

Allocation Clone(const Bytes& bytes) {
    Allocation pointer(bytes.empty() ? nullptr : std::malloc(bytes.size()));
    if (!bytes.empty() && !pointer) {
        throw std::bad_alloc();
    }
    if (pointer) {
        std::memcpy(pointer.get(), bytes.data(), bytes.size());
    }
    return pointer;
}

struct GeometryCopy {
    void* address;
    Bytes bytes;
};
} // namespace

struct BgCheckSnapshotData {
    CollisionContext* context;
    uint64_t allocationEpoch;
    uint64_t roomEpoch;
    CollisionContext metadata;
    Bytes lookup, staticNodes, polyChecks, actors, actorFlags, dynamicNodes;
    std::vector<GeometryCopy> geometry;
};

struct BgCheckRestoreData {
    std::shared_ptr<const BgCheckSnapshotData> snapshot;
    CollisionContext* context;
    CollisionContext previous;
    Allocation lookup, staticNodes, polyChecks, actors, actorFlags, dynamicNodes;
    bool committed = false;
};

BgCheckStateSnapshot::BgCheckStateSnapshot(std::shared_ptr<BgCheckSnapshotData> data) : data(std::move(data)) {}
BgCheckStateRestore::BgCheckStateRestore(std::unique_ptr<BgCheckRestoreData> data) : data(std::move(data)) {}
BgCheckStateRestore::~BgCheckStateRestore() = default;

std::unique_ptr<BgCheckStateSnapshot> BgCheckStateSnapshot::Capture(CollisionContext* context) noexcept {
    try {
        if (!context || BgCheck_GetAllocationEpoch(context) == 0 || context->colHeader == nullptr ||
            context->subdivAmount.x <= 0 || context->subdivAmount.y <= 0 || context->subdivAmount.z <= 0 ||
            context->polyNodes.count > context->polyNodes.max || context->dyna.bgActorMax < 0 ||
            context->dyna.polyNodes.count < 0 || context->dyna.polyNodes.count > context->dyna.polyNodes.max ||
            context->dyna.polyListMax < 0 || context->dyna.vtxListMax < 0) {
            return nullptr;
        }
        auto saved = std::make_shared<BgCheckSnapshotData>();
        saved->context = context;
        saved->allocationEpoch = BgCheck_GetAllocationEpoch(context);
        saved->roomEpoch = BgCheck_GetRoomEpoch(context);
        saved->metadata = *context;
        size_t cells = ByteCount(context->subdivAmount.x, context->subdivAmount.y);
        cells = ByteCount(cells, context->subdivAmount.z);
        saved->lookup = Copy(context->lookupTbl, ByteCount(cells, sizeof(StaticLookup)));
        saved->staticNodes = Copy(context->polyNodes.tbl, ByteCount(context->polyNodes.max, sizeof(SSNode)));
        saved->polyChecks = Copy(context->polyNodes.polyCheckTbl,
                                 context->colHeader->numPolygons != 0 ? context->colHeader->numPolygons : 1);
        saved->actors = Copy(context->dyna.bgActors, ByteCount(context->dyna.bgActorMax, sizeof(BgActor)));
        saved->actorFlags = Copy(context->dyna.bgActorFlags, ByteCount(context->dyna.bgActorMax, sizeof(u16)));
        saved->dynamicNodes = Copy(context->dyna.polyNodes.tbl, ByteCount(context->dyna.polyNodes.max, sizeof(SSNode)));
        std::vector<BgCheckGeometryBuffer> buffers(BgCheck_GetGeometryBuffers(context, nullptr, 0));
        if (BgCheck_GetGeometryBuffers(context, buffers.data(), buffers.size()) != buffers.size() ||
            !BgCheck_OwnsGeometryBuffer(context, context->dyna.polyList,
                                       ByteCount(context->dyna.polyListMax, sizeof(CollisionPoly))) ||
            !BgCheck_OwnsGeometryBuffer(context, context->dyna.vtxList,
                                       ByteCount(context->dyna.vtxListMax, sizeof(Vec3i)))) {
            return nullptr;
        }
        for (const auto& buffer : buffers) {
            saved->geometry.push_back({ buffer.address, Copy(buffer.address, buffer.size) });
        }
        return std::unique_ptr<BgCheckStateSnapshot>(new BgCheckStateSnapshot(std::move(saved)));
    } catch (const std::exception&) {
        return nullptr;
    }
}

bool BgCheckStateSnapshot::Matches(const CollisionContext* context) const noexcept {
    return data && data->context == context && data->allocationEpoch != 0 &&
           BgCheck_GetAllocationEpoch(context) == data->allocationEpoch &&
           BgCheck_GetRoomEpoch(context) == data->roomEpoch;
}

std::unique_ptr<BgCheckStateRestore> BgCheckStateSnapshot::Prepare(CollisionContext* context) const noexcept {
    try {
        if (!Matches(context)) {
            return nullptr;
        }
        auto restore = std::make_unique<BgCheckRestoreData>();
        restore->snapshot = data;
        restore->context = context;
        restore->previous = *context;
        restore->lookup = Clone(data->lookup);
        restore->staticNodes = Clone(data->staticNodes);
        restore->polyChecks = Clone(data->polyChecks);
        restore->actors = Clone(data->actors);
        restore->actorFlags = Clone(data->actorFlags);
        restore->dynamicNodes = Clone(data->dynamicNodes);
        auto prepared = std::unique_ptr<BgCheckStateRestore>(new BgCheckStateRestore(std::move(restore)));
        return prepared->Validate() ? std::move(prepared) : nullptr;
    } catch (const std::exception&) {
        return nullptr;
    }
}

bool BgCheckStateRestore::Validate() const noexcept {
    if (!data || data->committed) {
        return false;
    }
    const auto& saved = *data->snapshot;
    const auto* live = data->context;
    const auto& old = data->previous;
    if (BgCheck_GetAllocationEpoch(live) != saved.allocationEpoch ||
        BgCheck_GetRoomEpoch(live) != saved.roomEpoch || live->lookupTbl != old.lookupTbl ||
        live->polyNodes.tbl != old.polyNodes.tbl || live->polyNodes.polyCheckTbl != old.polyNodes.polyCheckTbl ||
        live->dyna.bgActors != old.dyna.bgActors || live->dyna.bgActorFlags != old.dyna.bgActorFlags ||
        live->dyna.polyNodes.tbl != old.dyna.polyNodes.tbl || live->dyna.polyList != old.dyna.polyList ||
        live->dyna.vtxList != old.dyna.vtxList) {
        return false;
    }
    for (const auto& buffer : saved.geometry) {
        if (!BgCheck_OwnsGeometryBuffer(live, buffer.address, buffer.bytes.size())) {
            return false;
        }
    }
    return true;
}

void BgCheckStateRestore::Commit() noexcept {
    if (!data || data->committed) {
        return;
    }
    auto& saved = *data->snapshot;
    auto* context = data->context;
    // Actor fields restored by the system heap copy still target these exact retained allocations.
    for (const auto& buffer : saved.geometry) {
        if (!buffer.bytes.empty()) {
            std::memcpy(buffer.address, buffer.bytes.data(), buffer.bytes.size());
        }
    }
    *context = saved.metadata;
    context->lookupTbl = static_cast<StaticLookup*>(data->lookup.release());
    context->polyNodes.tbl = static_cast<SSNode*>(data->staticNodes.release());
    context->polyNodes.polyCheckTbl = static_cast<u8*>(data->polyChecks.release());
    context->dyna.bgActors = static_cast<BgActor*>(data->actors.release());
    context->dyna.bgActorFlags = static_cast<u16*>(data->actorFlags.release());
    context->dyna.polyNodes.tbl = static_cast<SSNode*>(data->dynamicNodes.release());
    std::free(data->previous.lookupTbl);
    std::free(data->previous.polyNodes.tbl);
    std::free(data->previous.polyNodes.polyCheckTbl);
    std::free(data->previous.dyna.bgActors);
    std::free(data->previous.dyna.bgActorFlags);
    std::free(data->previous.dyna.polyNodes.tbl);
    data->committed = true;
}
