#include "actors/ForestGiParticles.h"
#include <cassert>
#include <cstdio>

using NeiGi::Mesh;
using NeiGi::Point;
float Distance(Point a, Point b) {
    const Point d = a - b;
    return std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
}
void Check(const Mesh& mesh, bool green) {
    assert(mesh.count && mesh.count % 3 == 0 && mesh.count <= mesh.vertices.size());
    bool fading = false, bright = false;
    for (size_t i = 0; i < mesh.count; ++i) {
        const auto& v = mesh.vertices[i];
        assert(std::isfinite(v.p.x) && std::isfinite(v.p.y) && std::isfinite(v.p.z));
        const unsigned r = (v.rgb >> 16) & 255, g = (v.rgb >> 8) & 255, b = v.rgb & 255;
        assert(green ? g >= r && g > b : b >= r && r >= g);
        fading |= v.alpha == 0; bright |= v.alpha > 90;
    }
    assert(fading && bright);
    // Particle triangles are local to one tiny emitter. A doorway-sized band,
    // connected outline or large scrolling ring cannot satisfy this bound.
    for (size_t i = 0; i < mesh.count; i += 3)
        for (int j = 0; j < 3; ++j)
            assert(Distance(mesh.vertices[i+j].p, mesh.vertices[i+(j+1)%3].p) <= 4.6f);
}
void CheckGreenCore(const Mesh& mesh, int motes) {
    assert(mesh.count == size_t(motes) * ForestGiParticles::kHexVertices);
    for (int i = 0; i < motes; ++i) {
        const size_t first = i * ForestGiParticles::kHexVertices + 48;
        const auto& center = mesh.vertices[first];
        assert(center.rgb == 0xB0FFB0);
        // Every hex triangle includes the bright center, rather than a hollow
        // inner contour. Camera-facing corners keep its six-sided identity.
        for (int side = 0; side < 6; ++side) {
            assert(Distance(center.p, mesh.vertices[first + side*3].p) == 0);
            assert(Distance(center.p, mesh.vertices[first + side*3 + 1].p) < 1.21f);
        }
    }
}
int main() {
    using namespace ForestGiParticles;
    const NeiGi::Basis cameras[] = { {}, {{0,0,-1},{0,1,0},{1,0,0}},
        {{1,0,0},{0,.70710678f,.70710678f},{0,-.70710678f,.70710678f}} };
    for (const auto& camera : cameras)
        for (uint32_t frame : {0u, 1u, 30u, 160u, 192u, 100000u, 0xffffffffu}) {
            for (bool seated : {false, true}) {
                const auto mesh = Saria(frame, seated, camera); Check(mesh, true); CheckGreenCore(mesh, kSariaMotes);
                for (size_t i = 0; i < mesh.count; ++i) {
                    const auto p = mesh.vertices[i].p;
                    assert(std::abs(p.x) < 28 && std::abs(p.z) < 28);
                    assert(p.y > 1 && p.y < (seated ? 30 : 52));
                }
            }
            const auto mystery = Mystery(frame, camera); Check(mystery, true); CheckGreenCore(mystery, kMysteryMotes);
            for (int i = 0; i < kMysteryMotes; ++i) {
                const auto p = mystery.vertices[i*kHexVertices].p;
                assert(std::abs(p.x) > 42 || p.y > 104); // Doorway stays clear.
                assert(std::abs(p.x) < 73 && p.y >= 0 && p.y < 135 && p.z > -56 && p.z < 13);
            }
            for (bool packed : {false, true}) {
                const auto portal = Portal(frame, packed, camera); Check(portal, true); CheckGreenCore(portal, kPortalMotes);
                assert(portal.count == kPortalMotes * kHexVertices);
                for (size_t i = 0; i < portal.count; ++i) {
                    const auto p = portal.vertices[i].p;
                    assert(std::abs(p.x) < (packed ? 73 : 49));
                    assert(p.y > -3 && p.y < (packed ? 139 : 114));
                }
            }
        }
    // Emitters do not travel around either doorway as time advances.
    const auto a = Portal(0, true), b = Portal(90, true);
    for (int i = 0; i < kPortalMotes; ++i) {
        const auto p = a.vertices[i*kHexVertices].p, q = b.vertices[i*kHexVertices].p;
        assert(std::abs(p.x-q.x) < 1.3f && std::abs(p.y-q.y) < 2.1f);
    }
    puts("PASS GI particle policy: filled green hex cores, real Glow/Star geometry, finite bounded meshes, clear doorways, camera bases and non-scrolling emitters");
}
