#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

// Find the compact horizontal cap in the loaded collision, so Prelude can move
// the stump without leaving its reward at an obsolete position. No scene bytes
// or collision surfaces are changed.
namespace MagicMirrorAnchor {
struct Point {
    float x, y, z;
};
struct Triangle {
    std::array<Point, 3> p;
};
inline bool Finite(const Point& p) {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
}
inline bool Near(const Point& a, const Point& b) {
    return std::abs(a.x - b.x) < 1 && std::abs(a.y - b.y) < 1 && std::abs(a.z - b.z) < 1;
}
inline bool Touch(const Triangle& a, const Triangle& b) {
    for (const auto& p : a.p)
        for (const auto& q : b.p)
            if (Near(p, q))
                return true;
    return false;
}
inline float Area(const Triangle& t) {
    return std::abs((t.p[1].x - t.p[0].x) * (t.p[2].z - t.p[0].z) - (t.p[1].z - t.p[0].z) * (t.p[2].x - t.p[0].x)) *
           0.5f;
}
inline bool Candidate(const Triangle& triangle) {
    for (const auto& p : triangle.p)
        if (!Finite(p) || std::abs(p.x) > 1100 || std::abs(p.z) > 1100 || p.y < 60 || p.y > 200 ||
            std::abs(p.y - triangle.p[0].y) >= 1)
            return false;
    return Area(triangle) > 1;
}
inline bool Find(const std::vector<Triangle>& source, Point& result) {
    std::vector<Triangle> triangles;
    for (const auto& t : source) {
        if (Candidate(t))
            triangles.push_back(t);
        if (triangles.size() > 2048)
            return false;
    }
    std::vector<bool> visited(triangles.size());
    float best = std::numeric_limits<float>::infinity();
    bool found = false;
    for (size_t first = 0; first < triangles.size(); ++first) {
        if (visited[first])
            continue;
        std::vector<size_t> component{ first };
        visited[first] = true;
        float minX = triangles[first].p[0].x, maxX = minX, minZ = triangles[first].p[0].z, maxZ = minZ, area = 0;
        for (size_t next = 0; next < component.size(); ++next) {
            const auto& t = triangles[component[next]];
            area += Area(t);
            for (const auto& p : t.p) {
                minX = std::min(minX, p.x);
                maxX = std::max(maxX, p.x);
                minZ = std::min(minZ, p.z);
                maxZ = std::max(maxZ, p.z);
            }
            for (size_t other = 0; other < triangles.size(); ++other)
                if (!visited[other] && Touch(t, triangles[other])) {
                    visited[other] = true;
                    component.push_back(other);
                }
        }
        const float width = maxX - minX, depth = maxZ - minZ;
        if (component.size() < 4 || width < 70 || width > 260 || depth < 70 || depth > 260 ||
            area < width * depth * 0.5f)
            continue;
        const float rank = std::abs(width - 110) + std::abs(depth - 112);
        if (rank < best) {
            best = rank;
            result = { (minX + maxX) * 0.5f, triangles[first].p[0].y, (minZ + maxZ) * 0.5f };
            found = true;
        }
    }
    return found;
}
inline bool CanClaim(const Point& player, const Point& cap) {
    const float x = player.x - cap.x, z = player.z - cap.z, y = player.y - cap.y;
    return Finite(player) && Finite(cap) && x * x + z * z <= 130 * 130 && y >= -220 && y <= 90;
}
} // namespace MagicMirrorAnchor
