#include "fast/resource/type/Vertex.h"
#include "fast/lus_gbi.h"
#undef GIMMCMD
#include "libultraship/libultra/gbi.h"
#include <cstdint>
#include <typeinfo>

static_assert(sizeof(Vtx) == sizeof(Fast::F3DVtx) && sizeof(Vtx) == 24);

extern "C" Ship::IResource* VertexDso_CreateFast(uint32_t stride) {
    auto* resource = new Fast::Vertex;
    resource->RecordSize = stride;
    resource->VertexList.resize(3);
    resource->VertexList[0].v.ob[0] = 80000;
    resource->VertexList[2].v.ob[1] = -90000;
    return resource;
}

extern "C" uint32_t VertexDso_RecordSize(Ship::IResource* resource) {
    auto* source = dynamic_cast<Fast::IVertexSource*>(resource);
    return source ? source->GetVertexRecordSize() : UINT32_MAX;
}

extern "C" bool VertexDso_CheckData(Ship::IResource* resource, int32_t x, int32_t y) {
    if (!resource || resource->GetPointerSize() != 3 * sizeof(Fast::F3DVtx))
        return false;
    const auto* vertices = static_cast<Fast::F3DVtx*>(resource->GetRawPointer());
    return vertices && vertices[0].v.ob[0] == x && vertices[2].v.ob[1] == y;
}

extern "C" const void* VertexDso_InterfaceTypeInfo() {
    return &typeid(Fast::IVertexSource);
}

extern "C" void VertexDso_Destroy(Ship::IResource* resource) {
    delete resource;
}
