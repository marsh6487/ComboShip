// SOH [Unbound] collision.json + collision.bin -> SOH::CollisionHeader. See unbound-docs/SPEC.md §4.4.
#include "UnboundFactories.h"
#include "UnboundJson.h"
#include "UnboundSchema.h"

#include <spdlog/spdlog.h>
#include <ship/utils/binarytools/BinaryReader.h>
#include <cstdint>

#include "soh/resource/type/CollisionHeader.h"

using SOH::Unbound::Json;
using SOH::Unbound::PositionalKeys;
using SOH::Unbound::ToInt;
namespace K = SOH::Unbound::Schema;

namespace SOH {
namespace {

// collision.bin layout (SPEC.md §4.4.1): little-endian, vertices then polys, no header.
//   vertex { s32 x, y, z } (12 B)
//   poly   { u16 type, u16 pad, u32 vA, vB, vC, s16 nx, ny, nz, s16 pad, s32 dist } (28 B)
constexpr size_t kVertexBytes = 12;
constexpr size_t kPolyBytes = 28;

bool BulkSize(int64_t numVertices, int64_t numPolys, size_t* byteCount) {
    // Vertex words have 29 index bits; native polygon ids and iteration bounds are signed 32-bit.
    // Validate before narrowing JSON integers or doing size_t arithmetic (which may be 32-bit).
    if (numVertices < 0 || numVertices > 0x20000000ll || numPolys < 0 || numPolys > INT32_MAX) {
        return false;
    }
    uint64_t bytes = (uint64_t)numVertices * kVertexBytes + (uint64_t)numPolys * kPolyBytes;
    if (bytes > SIZE_MAX || (uint64_t)numVertices > SIZE_MAX / sizeof(Vec3i) ||
        (uint64_t)numPolys > SIZE_MAX / sizeof(CollisionPoly)) {
        return false;
    }
    *byteCount = (size_t)bytes;
    return true;
}

Vec3i ReadVertex(Ship::BinaryReader& r) {
    Vec3i v;
    v.x = r.ReadInt32();
    v.y = r.ReadInt32();
    v.z = r.ReadInt32();
    return v;
}

CollisionPoly ReadPoly(Ship::BinaryReader& r) {
    CollisionPoly p{};
    p.type = r.ReadUInt16();
    r.ReadUInt16(); // pad
    p.flags_vIA = r.ReadUInt32();
    p.flags_vIB = r.ReadUInt32();
    p.vIC = r.ReadUInt32();
    p.normal.x = r.ReadInt16();
    p.normal.y = r.ReadInt16();
    p.normal.z = r.ReadInt16();
    r.ReadInt16(); // pad
    p.dist = r.ReadInt32();
    return p;
}

bool ReadBulk(CollisionHeader& col, const Json& bulk, const std::string& docPath) {
    int64_t vertexCount = Unbound::Field(bulk, K::kVertices);
    int64_t polyCount = Unbound::Field(bulk, K::kPolys);
    size_t byteCount;
    if (!BulkSize(vertexCount, polyCount, &byteCount) || (uint64_t)vertexCount > col.vertices.max_size() ||
        (uint64_t)polyCount > col.polygons.max_size()) {
        SPDLOG_ERROR("[Unbound] {}: bulk counts exceed the native index or allocation range", docPath);
        return false;
    }
    uint32_t numVertices = (uint32_t)vertexCount;
    uint32_t numPolys = (uint32_t)polyCount;
    std::string binPath = Unbound::PathField(bulk, K::kFile);
    std::vector<char> bytes = Unbound::LoadBulk(binPath);
    if (bytes.empty()) {
        SPDLOG_ERROR("[Unbound] {}: bulk file {} missing", docPath, binPath);
        return false;
    }
    if (bytes.size() < byteCount) {
        SPDLOG_ERROR("[Unbound] {}: bulk file {} is shorter than its declared counts", docPath, binPath);
        return false;
    }

    Ship::BinaryReader r(bytes.data(), bytes.size());
    r.SetEndianness(Ship::Endianness::Little);
    col.vertices.reserve(numVertices);
    for (uint32_t i = 0; i < numVertices; i++) {
        col.vertices.push_back(ReadVertex(r));
    }
    col.polygons.reserve(numPolys);
    for (uint32_t i = 0; i < numPolys; i++) {
        col.polygons.push_back(ReadPoly(r));
    }
    col.collisionHeaderData.numVertices = (u32)col.vertices.size();
    col.collisionHeaderData.vtxList = col.vertices.data();
    col.collisionHeaderData.numPolygons = (u32)col.polygons.size();
    col.collisionHeaderData.polyList = col.polygons.data();
    return true;
}

SurfaceType ReadSurfaceType(const Json& e) {
    SurfaceType s{};
    s.camera = (s32)Unbound::Field(e, K::kCamera);
    s.exit = (s32)Unbound::Field(e, K::kExit);
    s.lightSetting = (s32)Unbound::Field(e, K::kLightSetting);
    s.floorType = (u8)Unbound::Field(e, K::kFloorType);
    s.wallFlags = (u8)Unbound::Field(e, K::kWallFlags);
    s.wallType = (u8)Unbound::Field(e, K::kWallType);
    s.floorProperty = (u8)Unbound::Field(e, K::kFloorProperty);
    s.isSoft = (u8)Unbound::Field(e, K::kIsSoft);
    s.isHorseBlocked = (u8)Unbound::Field(e, K::kIsHorseBlocked);
    s.material = (u8)Unbound::Field(e, K::kMaterial);
    s.floorEffect = (u8)Unbound::Field(e, K::kFloorEffect);
    s.echo = (u8)Unbound::Field(e, K::kEcho);
    s.canHookshot = (u8)Unbound::Field(e, K::kCanHookshot);
    s.conveyorSpeed = (u8)Unbound::Field(e, K::kConveyorSpeed);
    s.conveyorDirection = (u8)Unbound::Field(e, K::kConveyorDirection);
    s.isWallDamage = (u8)Unbound::Field(e, K::kIsWallDamage);
    return s;
}

void ReadSurfaceTypes(CollisionHeader& col, const Json& list, const std::string& docPath) {
    auto keys = PositionalKeys(list, docPath + " " + K::kSurfaceTypes);
    if (keys.size() > UINT32_MAX) {
        throw Unbound::DocumentError(docPath + ": too many surface types for the native count");
    }
    for (const auto& k : keys) {
        col.surfaceTypes.push_back(ReadSurfaceType(list[k]));
    }
    col.surfaceTypesCount = (uint32_t)col.surfaceTypes.size();
    col.collisionHeaderData.surfaceTypeList = col.surfaceTypes.data();
}

void ReadCameras(CollisionHeader& col, const Json& cameras, const Json& positions, const std::string& docPath) {
    // Camera positions stay s16: CamData packs them into Vec3s (SPEC.md §9).
    auto positionKeys = PositionalKeys(positions, docPath + " " + K::kCameraPositions);
    auto cameraKeys = PositionalKeys(cameras, docPath + " " + K::kCameras);
    if (positionKeys.size() > INT32_MAX || cameraKeys.size() > UINT32_MAX) {
        throw Unbound::DocumentError(docPath + ": camera list exceeds its native count");
    }
    for (const auto& k : positionKeys) {
        col.camPosData.push_back(Unbound::ReadVec3s(positions[k]));
    }
    col.camPosCount = (int32_t)col.camPosData.size();
    col.camPosDataZero = Vec3s{ 0, 0, 0 };

    for (const auto& k : cameraKeys) {
        const Json& c = cameras[k];
        CamData cam{};
        cam.cameraSType = (u16)Unbound::Field(c, K::kSType);
        int64_t count = Unbound::Field(c, K::kCount);
        if (count < INT16_MIN || count > INT16_MAX) {
            throw Unbound::DocumentError(docPath + ": camera count exceeds its native field");
        }
        cam.numCameras = (s16)count;
        int64_t positionIndex = c.contains(K::kPositionIndex) ? ToInt(c[K::kPositionIndex], -1) : -1;
        int32_t idx = positionIndex >= 0 && positionIndex < col.camPosCount ? (int32_t)positionIndex : -1;
        col.camPosDataIndices.push_back(idx);
        col.camData.push_back(cam);
    }
    col.camDataCount = (uint32_t)col.camData.size();
    for (size_t i = 0; i < col.camData.size(); i++) {
        int32_t idx = col.camPosDataIndices[i];
        col.camData[i].camPosData =
            (idx >= 0 && idx < col.camPosCount) ? &col.camPosData[(size_t)idx] : &col.camPosDataZero;
    }
    col.collisionHeaderData.cameraDataList = col.camData.data();
    col.collisionHeaderData.cameraDataListLen = col.camDataCount;
}

void ReadWaterBoxes(CollisionHeader& col, const Json& list, const std::string& docPath) {
    auto keys = PositionalKeys(list, docPath + " " + K::kWaterBoxes);
    if (keys.size() > UINT16_MAX) {
        throw Unbound::DocumentError(docPath + ": too many water boxes for the native count");
    }
    for (const auto& k : keys) {
        const Json& w = list[k];
        WaterBox box{};
        box.xMin = Unbound::IntegralField(w, K::kXMin);
        box.ySurface = Unbound::IntegralField(w, K::kYSurface);
        box.zMin = Unbound::IntegralField(w, K::kZMin);
        box.xLength = Unbound::IntegralField(w, K::kXLength);
        box.zLength = Unbound::IntegralField(w, K::kZLength);
        box.camera = (s32)Unbound::Field(w, K::kCamera);
        box.lightSetting = (s32)Unbound::Field(w, K::kLightSetting);
        box.room = w.contains(K::kRoom) ? (s32)ToInt(w[K::kRoom]) : -1;
        box.notSwimmable = (u8)Unbound::Field(w, K::kNotSwimmable);
        col.waterBoxes.push_back(box);
    }
    if (col.waterBoxes.size() > UINT16_MAX) { // CollisionHeader.numWaterBoxes is a u16 (SPEC.md §9)
        throw Unbound::DocumentError(docPath + ": " + std::to_string(col.waterBoxes.size()) + " water boxes; at most " +
                                     std::to_string(UINT16_MAX));
    }
    col.collisionHeaderData.numWaterBoxes = (u16)col.waterBoxes.size();
    col.collisionHeaderData.waterBoxes = col.waterBoxes.data();
}

// The only collision document this build reads is SPEC.md §4.4's; a missing $schema is not accepted.
bool CollisionSchemaIsCurrent(const Json& doc, const std::string& docPath) {
    std::string schema = Unbound::SchemaOf(doc);
    std::string type;
    int version = 0;
    bool known =
        Unbound::ParseSchema(schema, type, version) && type == K::kCollisionType && version == K::kCollisionVersion;
    if (!known) {
        SPDLOG_ERROR("[Unbound] {}: unsupported $schema '{}' (this build reads {})", docPath, schema, K::kCollisionV3);
    }
    return known;
}

std::shared_ptr<CollisionHeader> ReadCollisionDocument(const Json& doc,
                                                       std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!CollisionSchemaIsCurrent(doc, initData->Path)) {
        return nullptr;
    }
    auto col = std::make_shared<CollisionHeader>(initData);
    const Json& bounds = Unbound::Sub(doc, K::kBounds);
    col->collisionHeaderData.minBounds = Unbound::ReadVec3i(Unbound::SubArray(bounds, K::kMin));
    col->collisionHeaderData.maxBounds = Unbound::ReadVec3i(Unbound::SubArray(bounds, K::kMax));
    if (!ReadBulk(*col, Unbound::Sub(doc, K::kBulk), initData->Path)) {
        return nullptr;
    }
    ReadSurfaceTypes(*col, Unbound::Sub(doc, K::kSurfaceTypes), initData->Path);
    ReadCameras(*col, Unbound::Sub(doc, K::kCameras), Unbound::Sub(doc, K::kCameraPositions), initData->Path);
    ReadWaterBoxes(*col, Unbound::Sub(doc, K::kWaterBoxes), initData->Path);
    return col;
}

} // namespace

std::shared_ptr<Ship::IResource>
ResourceFactoryJsonCollisionHeaderV3::ReadResource(std::shared_ptr<Ship::File> file,
                                                   std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }
    Json doc = Unbound::LoadMergedJson(initData->Path);
    if (!doc.is_object()) {
        SPDLOG_ERROR("[Unbound] {}: no usable document", initData->Path);
        return nullptr;
    }

    try {
        return ReadCollisionDocument(doc, initData);
    } catch (const Unbound::DocumentError& e) { // a hole in an indexed list, a bad count (SPEC.md §3)
        SPDLOG_ERROR("[Unbound] {}", e.what());
    } catch (const std::exception& e) { SPDLOG_ERROR("[Unbound] {}: {}", initData->Path, e.what()); }
    return nullptr;
}

} // namespace SOH
