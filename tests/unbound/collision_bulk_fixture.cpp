#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include "soh/soh/unbound/UnboundJson.h"
#include "soh/soh/unbound/UnboundSchema.h"

#define SPDLOG_ERROR(...) ((void)0)
using SOH::Unbound::Json;
namespace K = SOH::Unbound::Schema;
namespace Unbound = SOH::Unbound;
static std::vector<char> bulkBytes;
static unsigned bulkLoads;
namespace SOH::Unbound {
/* ACTUAL NUMERIC READERS */
std::vector<char> LoadBulk(const std::string&) { ++bulkLoads; return bulkBytes; }
}
namespace Ship {
enum class Endianness { Little };
struct BinaryReader {
    size_t offset = 0, length;
    BinaryReader(char*, size_t length) : length(length) {}
    void SetEndianness(Endianness) {}
    int32_t ReadInt32() { offset += 4; assert(offset <= length); return 0; }
    uint32_t ReadUInt32() { return ReadInt32(); }
    int16_t ReadInt16() { offset += 2; assert(offset <= length); return 0; }
    uint16_t ReadUInt16() { return ReadInt16(); }
};
}
struct CollisionPoly { u16 type; u32 flags_vIA, flags_vIB, vIC; Vec3s normal; s32 dist; };
struct CollisionHeader {
    std::vector<Vec3i> vertices;
    std::vector<CollisionPoly> polygons;
    struct { u32 numVertices = 77; Vec3i* vtxList = nullptr; u32 numPolygons = 88; CollisionPoly* polyList = nullptr; } collisionHeaderData;
};
/* ACTUAL COLLISION BULK */

int main() {
    auto invalid = [](int64_t vertices, int64_t polygons) {
        CollisionHeader col;
        bulkBytes.assign(1, 0); bulkLoads = 0;
        Json bulk = { { K::kFile, "collision.bin" }, { K::kVertices, vertices }, { K::kPolys, polygons } };
        assert(!ReadBulk(col, bulk, "collision.json"));
        assert(bulkLoads == 0 && col.vertices.empty() && col.polygons.empty());
        assert(col.collisionHeaderData.numVertices == 77 && col.collisionHeaderData.numPolygons == 88);
    };
    invalid(-4294967296ll, 0);
    invalid(0, 4294967296ll);
    invalid(0x20000001ll, 0);
    invalid(0, 0x80000000ll);
    CollisionHeader col;
    Json bulk = { { K::kFile, "collision.bin" }, { K::kVertices, 3 }, { K::kPolys, 2 } };
    bulkBytes.assign(3 * 12 + 2 * 28, 0); bulkLoads = 0;
    assert(ReadBulk(col, bulk, "collision.json"));
    assert(bulkLoads == 1 && col.vertices.size() == 3 && col.polygons.size() == 2);
    assert(col.collisionHeaderData.numVertices == 3 && col.collisionHeaderData.numPolygons == 2);
    col = {}; bulkBytes.pop_back();
    assert(!ReadBulk(col, bulk, "collision.json") && col.vertices.empty() && col.polygons.empty());
}
