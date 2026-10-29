#include "fast/lus_gbi.h"
#include "mm/2s2h/resource/type/Array.h"
#include "combo/ComboExport.h"
#include <typeinfo>

static_assert(sizeof(Fast::F3DVtx) == 24);

extern "C" COMBO_EXPORT Ship::IResource* VertexMmDso_CreateArray(bool vertex) {
    auto* resource = new SOH::Array;
    resource->ArrayType = vertex ? SOH::ArrayResourceType::Vertex : SOH::ArrayResourceType::Scalar;
    resource->ArrayScalarType = SOH::ScalarType::ZSCALAR_U16;
    resource->ArrayCount = 1000; // Runtime vertex storage remains authoritative.
    resource->Vertices.resize(3);
    resource->Vertices[0].v.ob[0] = 12345;
    resource->Vertices[2].v.ob[1] = -23456;
    resource->Scalars.resize(3);
    return resource;
}

extern "C" COMBO_EXPORT const void* VertexMmDso_InterfaceTypeInfo() {
    return &typeid(Fast::IVertexSource);
}
