#include "combo/BlackMarketGranny.h"
#include "resource/type/Array.h"
#include <cassert>
#include <cstdio>
#include <fast/lus_gbi.h>
#include <fast/resource/type/Vertex.h>
/* PRODUCTION ADAPTER */
int main() {
  auto nativeInfo = std::make_shared<Ship::ResourceInitData>();
  nativeInfo->Type = 0x4f415252;
#ifdef GRANNY_VAPOR_MM
  auto native = std::make_shared<SOH::Array>(nativeInfo);
  native->ArrayType = SOH::ArrayResourceType::Vertex;
  native->ArrayCount = 56;
  auto& nativeVertices = native->Vertices;
#else
  // The OoT Array factory now imports an OARR/Vertex as Fast::Vertex. The
  // archive's resource type remains OARR; scalar OARRs still use SOH::Array.
  auto native = std::make_shared<Fast::Vertex>(nativeInfo);
  auto& nativeVertices = native->VertexList;
#endif
  nativeVertices.resize(56);
  nativeVertices[0].v.ob[0] = 123;
  auto converted = BlackMarketGranny::NativeVaporVertices(native);
  assert(converted.data != nullptr &&
         std::memcmp(converted.data, nativeVertices.data(),
                     56 * sizeof(Vtx)) == 0 &&
         converted.count == 56 && converted.owner &&
         "original OARR vertex resource must reach Granny vapor validation");
#ifdef GRANNY_VAPOR_MM
  // ArrayCount is not the public vertex byte length: the native Array
  // implementation derives it from its live vector, as the renderer does.
  native->ArrayCount = 55;
  assert(BlackMarketGranny::NativeVaporVertices(native).count == 56);
#endif
  nativeVertices.resize(55);
  assert(!BlackMarketGranny::NativeVaporVertices(native).data);
  nativeVertices.resize(57);
  assert(!BlackMarketGranny::NativeVaporVertices(native).data);
  nativeVertices.resize(56);
  auto scalar = std::make_shared<SOH::Array>(nativeInfo);
  scalar->ArrayType = SOH::ArrayResourceType::Scalar;
  scalar->ArrayScalarType = SOH::ScalarType::ZSCALAR_U16;
  scalar->ArrayCount = 56;
  scalar->Scalars.resize(56);
  assert(!BlackMarketGranny::NativeVaporVertices(scalar).data);
  auto modernInfo = std::make_shared<Ship::ResourceInitData>();
  modernInfo->Type = 0x4f565458;
  auto modern = std::make_shared<Fast::Vertex>(modernInfo);
  modern->VertexList.resize(56);
  assert(BlackMarketGranny::NativeVaporVertices(modern).count == 56);
  modern->VertexList.resize(55);
  assert(!BlackMarketGranny::NativeVaporVertices(modern).data);
  modern->VertexList.resize(56);
  modernInfo->Type = 0xdeadbeef;
  assert(!BlackMarketGranny::NativeVaporVertices(modern).data);
  auto noInfo = std::make_shared<Fast::Vertex>(std::shared_ptr<Ship::ResourceInitData>{});
  noInfo->VertexList.resize(56);
  assert(!BlackMarketGranny::NativeVaporVertices(noInfo).data);
  assert(!BlackMarketGranny::NativeVaporVertices({}).data);
  // The validated copy owns its bytes independently of the imported source.
  std::weak_ptr<void> retained = converted.owner;
  std::weak_ptr<void> source = native;
  nativeVertices[0].v.ob[0] = 456;
  assert(converted.data[0].v.ob[0] == 123);
  native.reset();
  assert(source.expired());
  assert(converted.data[0].v.ob[0] == 123);
  assert(!retained.expired());
  converted = {};
  assert(retained.expired());
  std::puts(
      "PASS Granny native vapor resource boundary: real legacy OARR/Vertex and "
      "Fast::Vertex owners, public byte-count/type rejection and copied lifetime");
}
