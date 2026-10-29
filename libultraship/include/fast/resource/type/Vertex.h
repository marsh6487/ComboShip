#pragma once

#include "ship/resource/Resource.h"
#include <vector>

union Vtx;

namespace Fast {
// Render resources can expose vertices without sharing game-specific classes.
// The archive stride is independent of the padded/widened runtime vertex ABI.
class IVertexSource {
  public:
    virtual ~IVertexSource() = default;
    virtual uint32_t GetVertexRecordSize() const = 0;
};

class Vertex final : public Ship::Resource<Vtx>, public IVertexSource {
  public:
    using Resource::Resource;

    Vertex();

    Vtx* GetPointer() override;
    size_t GetPointerSize() override;
    uint32_t GetVertexRecordSize() const override { return RecordSize; }

    std::vector<Vtx> VertexList;

    // Bytes per vertex as stored in the archive. NOT sizeof(Vtx): the runtime struct is padded and its
    // positions may be wider. Exported display lists address vertices by byte offset, so this is the divisor
    // that turns such an offset into an element index. v0 (vanilla s16) is 16; v1 (s32 positions) is 22.
    uint32_t RecordSize = 16;
};
} // namespace Fast
