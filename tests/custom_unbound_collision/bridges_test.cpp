#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
#include "functions.h"
#include "entrances/FountainValley.h"
#include "entrances/FountainSnowheadBridge.h"
#include "entrances/LakeHyliaBridge.h"
#include "bridge_adapters.inc"

static void Validate(const CollisionHeader& mesh) {
    assert(mesh.surfaceTypeList && mesh.cameraDataList && mesh.cameraDataListLen == 1);
    assert(mesh.cameraDataList[0].cameraSType == CAM_SET_NONE);
    assert(mesh.surfaceTypeList[0].camera == 0 && mesh.surfaceTypeList[0].exit == 0);
    assert(mesh.numWaterBoxes == 0 && !mesh.waterBoxes);
    for (u32 p = 0; p < mesh.numPolygons; ++p)
        for (int j = 0; j < 3; ++j) {
            const auto& poly = mesh.polyList[p];
            const auto index = COLPOLY_VTX_INDEX(poly.vtxData[j]);
            assert(index < mesh.numVertices);
            const auto& v = mesh.vtxList[index];
            assert(v.x >= mesh.minBounds.x && v.x <= mesh.maxBounds.x);
            assert(v.y >= mesh.minBounds.y && v.y <= mesh.maxBounds.y);
            assert(v.z >= mesh.minBounds.z && v.z <= mesh.maxBounds.z);
            const float plane = (float(poly.normal.x) * v.x + float(poly.normal.y) * v.y +
                                 float(poly.normal.z) * v.z) / 32767.f + poly.dist;
            assert(std::fabs(plane) < .7f);
        }
}

int main() {
    CollisionHeader empty{};
    ValleyTest::Build(&empty);
    assert(ValleyTest::sCollision.numVertices == 129 && ValleyTest::sCollision.numPolygons == 43);
    Validate(ValleyTest::sCollision);
    for (size_t i = 0; i < ComboFountainValley::kTriangles.size(); ++i) {
        const auto& t = ComboFountainValley::kTriangles[i];
        const ComboFountainValley::Point expected[] = { t.a, t.b, t.c };
        for (int j = 0; j < 3; ++j) {
            const auto& actual = ValleyTest::sVertices[i * 3 + j];
            assert(actual.x == expected[j].x && actual.y == expected[j].y && actual.z == expected[j].z);
        }
    }
    // A legacy 13-bit mask falsely misses this already-authored scene face and
    // duplicates it. Wide indices must retain duplicate suppression and flags.
    std::vector<Vec3i> sceneVertices(9003);
    const auto& t = ComboFountainValley::kTriangles[0];
    const ComboFountainValley::Point first[] = { t.a, t.b, t.c };
    for (int j = 0; j < 3; ++j)
        sceneVertices[9000 + j] = { s32(first[j].x), s32(first[j].y), s32(first[j].z) };
    CollisionPoly existing{};
    existing.flags_vIA = 9000 | (1u << COLPOLY_VIA_FLAGS_SHIFT);
    existing.flags_vIB = 9001;
    existing.vIC = 9002;
    WaterBox water{};
    water.ySurface = 78000;
    CollisionHeader scene{};
    scene.numVertices = sceneVertices.size();
    scene.vtxList = sceneVertices.data();
    scene.numPolygons = 1;
    scene.polyList = &existing;
    scene.numWaterBoxes = 1;
    scene.waterBoxes = &water;
    ValleyTest::Build(&scene);
    assert(ValleyTest::sCollision.numPolygons == 42 && water.ySurface == 78000);
    assert(scene.polyList == &existing && existing.flags_vIA == (9000 | (1u << COLPOLY_VIA_FLAGS_SHIFT)));

    SnowTest::Build();
    assert(SnowTest::sCollision.numVertices == 108 && SnowTest::sCollision.numPolygons == 36);
    Validate(SnowTest::sCollision);
    for (size_t i = 0; i < SnowTest::sMesh.count; ++i) {
        const auto& t = SnowTest::sMesh.triangles[i];
        const ComboFountainSnowhead::Point points[] = { t.a, t.b, t.c };
        for (int j = 0; j < 3; ++j) {
            const auto& actual = SnowTest::sVertices[i * 3 + j];
            assert(actual.x == std::lround(points[j].x) && actual.y == std::lround(points[j].y) &&
                   actual.z == std::lround(points[j].z));
            const auto& draw = SnowTest::sDrawVertices[i * 3 + j].v;
            assert(draw.ob[0] == actual.x && draw.ob[1] == actual.y && draw.ob[2] == actual.z);
        }
    }
    LakeTest::CaveCollision lake;
    Validate(lake.header);
    size_t vertex = 0;
    for (const auto& q : LakeTest::Geometry().solid) {
        if (!q.collides)
            continue;
        for (const auto p : { q.a, q.b, q.c, q.d }) {
            const auto& actual = lake.vertices[vertex++];
            assert(actual.x == std::lround(p.x) && actual.y == std::lround(p.y) && actual.z == std::lround(p.z));
        }
    }
    assert(vertex == lake.header.numVertices && lake.header.numPolygons == vertex / 2);
    std::puts("PASS bridges: original authored Valley/Snowhead/Lake geometry and neutral camera/water fields; wide-index duplicate suppression");
}
