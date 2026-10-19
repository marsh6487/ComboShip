#pragma once

// Texture-free geometry shared by the native OoT/MM draw path and the icon/preview
// exporter. Coordinates are ordinary game units; Draw uses 1/16-unit Vtx precision.
#include <array>
#include <cmath>
#include <cstdint>

namespace ComboMagicMirrorGeometry {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kVertexPrecision = 16.0f;
constexpr uint32_t kAnimationFrames = 960;
constexpr int kRimSegments = 32;
constexpr int kGlassRings = 5;
constexpr int kMaxVertices = 1280;
constexpr int kMaxTriangles = 1280;
constexpr int kMaxBatches = 96;

struct Color {
    uint8_t r, g, b, a;
};
struct Vertex {
    float x, y, z;
    uint8_t r, g, b, a;
};
struct Triangle {
    uint8_t a, b, c;
};
struct Batch {
    uint16_t firstVertex, firstTriangle;
    uint8_t vertexCount, triangleCount;
};
struct Mesh {
    std::array<Vertex, kMaxVertices> vertices;
    std::array<Triangle, kMaxTriangles> triangles;
    std::array<Batch, kMaxBatches> batches;
    uint16_t vertexCount = 0, triangleCount = 0, batchCount = 0, opaqueBatchCount = 0;
};

inline uint8_t Channel(float value) {
    return static_cast<uint8_t>(value < 0 ? 0 : value > 255 ? 255 : value + 0.5f);
}
inline Color Mix(Color a, Color b, float amount) {
    return { Channel(a.r + (b.r - a.r) * amount), Channel(a.g + (b.g - a.g) * amount),
             Channel(a.b + (b.b - a.b) * amount), Channel(a.a + (b.a - a.a) * amount) };
}
inline Vertex Point(float x, float y, float z, Color color) {
    return { x, y, z, color.r, color.g, color.b, color.a };
}

class Builder {
  public:
    explicit Builder(Mesh& mesh) : mesh(mesh) {
        mesh.vertexCount = mesh.triangleCount = mesh.batchCount = 0;
    }
    void Begin() {
        mesh.batches[mesh.batchCount++] = { mesh.vertexCount, mesh.triangleCount, 0, 0 };
    }
    void Add(Vertex vertex) {
        mesh.vertices[mesh.vertexCount++] = vertex;
        ++Current().vertexCount;
    }
    void Tri(int a, int b, int c) {
        mesh.triangles[mesh.triangleCount++] = { static_cast<uint8_t>(a), static_cast<uint8_t>(b),
                                                 static_cast<uint8_t>(c) };
        ++Current().triangleCount;
    }
    void Quad(int a, int b, int c, int d) {
        Tri(a, b, c);
        Tri(a, c, d);
    }

  private:
    Batch& Current() {
        return mesh.batches[mesh.batchCount - 1];
    }
    Mesh& mesh;
};

inline Color Gold(float angle, int profile) {
    // Baked upper-left light keeps the antique bevel readable without native lights.
    constexpr Color colors[] = { { 126, 73, 25, 255 }, { 255, 225, 131, 255 }, { 224, 165, 61, 255 },
                                 { 148, 87, 26, 255 }, { 188, 124, 42, 255 },  { 110, 60, 22, 255 } };
    const float light = 0.73f + 0.27f * (0.5f + 0.5f * (-0.65f * std::cos(angle) + 0.76f * std::sin(angle)));
    const auto color = colors[profile];
    return { Channel(color.r * light), Channel(color.g * light), Channel(color.b * light), 255 };
}

inline void Diamond(Builder& builder, float x, float y, float rx, float ry, float z, float rise, bool emerald) {
    builder.Begin();
    const Color light = emerald ? Color{ 117, 235, 173, 255 } : Color{ 255, 231, 144, 255 };
    const Color dark = emerald ? Color{ 13, 86, 73, 255 } : Color{ 128, 72, 26, 255 };
    builder.Add(Point(x, y, z + rise, emerald ? Color{ 45, 165, 125, 255 } : Color{ 225, 165, 64, 255 }));
    builder.Add(Point(x - rx, y, z, light));
    builder.Add(Point(x, y + ry, z, light));
    builder.Add(Point(x + rx, y, z, dark));
    builder.Add(Point(x, y - ry, z, dark));
    for (int i = 0; i < 4; ++i)
        builder.Tri(0, i + 1, (i + 1) % 4 + 1);
}

inline Vertex HandlePoint(float width, float y, int index) {
    const float angle = index * (2.0f * kPi / 8.0f);
    const float light = 0.66f + 0.34f * (0.5f + 0.5f * (-0.45f * std::cos(angle) + 0.89f * std::sin(angle)));
    return Point(width * std::cos(angle), y, 0.9f * std::sin(angle),
                 { Channel(246 * light), Channel(183 * light), Channel(72 * light), 255 });
}

inline Vertex GlassPoint(float radius, float angle, float phase) {
    const float nx = radius * std::cos(angle), ny = radius * std::sin(angle);
    const float ripple =
        0.5f + 0.5f * std::sin(ny * 10.0f + nx * 3.0f + phase * 2.0f + 0.7f * std::sin(nx * 4.0f - phase));
    const float violet = 0.5f + 0.5f * std::sin(ny * 6.0f - nx * 4.0f - phase);
    const float purple = violet * violet * (0.25f + 0.5f * (1.0f - ny) * 0.5f);
    Color color = Mix({ 43, 143, 111, 242 }, { 103, 213, 210, 242 }, ripple);
    color = Mix(color, { 113, 76, 163, 242 }, purple);
    // A broad reflected highlight, independent of the animated waves, reads as glass.
    const float reflection = std::fmax(0.0f, 1.0f - std::fabs(nx + ny * 0.38f + 0.22f) * 3.2f) * 0.28f;
    color = Mix(color, { 200, 247, 228, 242 }, reflection);
    const float z = 0.15f + 0.22f * (1.0f - radius * radius) + 0.025f * std::sin(nx * 4 + ny * 7 - phase * 2);
    return Point(nx * 10.15f, 3.0f + ny * 14.6f, z, color);
}

// Geometry and color are pure functions of the bounded animation phase. No RNG,
// global matrix, persistent vertex storage, resource manager or save state is used.
inline void Build(uint32_t frame, Mesh& mesh) {
    Builder builder(mesh);
    const float phase = (frame % kAnimationFrames) * (2.0f * kPi / kAnimationFrames);
    constexpr float rimX[] = { 10.25f, 10.78f, 12.05f, 12.70f, 12.05f, 10.25f };
    constexpr float rimY[] = { 14.80f, 15.45f, 16.65f, 17.20f, 16.65f, 14.80f };
    constexpr float rimZ[] = { 1.15f, 1.75f, 1.15f, 0.0f, -1.20f, -1.20f };
    // Each closed profile forms one smooth oval-frame segment, front and back.
    for (int segment = 0; segment < kRimSegments; ++segment) {
        builder.Begin();
        for (int end = 0; end < 2; ++end) {
            const float angle = ((segment + end) % kRimSegments) * (2 * kPi / kRimSegments);
            for (int p = 0; p < 6; ++p)
                builder.Add(Point(rimX[p] * std::cos(angle), 3 + rimY[p] * std::sin(angle), rimZ[p], Gold(angle, p)));
        }
        for (int p = 0; p < 6; ++p)
            builder.Quad(p, (p + 1) % 6, 6 + (p + 1) % 6, 6 + p);
    }
    constexpr float handleY[] = { -12.8f, -14.4f, -16.0f, -17.0f, -23.0f, -24.0f, -25.0f, -27.0f, -27.8f };
    constexpr float handleX[] = { 1.75f, 2.65f, 2.1f, 1.55f, 1.35f, 2.2f, 2.7f, 1.8f, 0.7f };
    for (int level = 0; level < 8; ++level) {
        builder.Begin();
        for (int end = 0; end < 2; ++end)
            for (int side = 0; side < 8; ++side)
                builder.Add(HandlePoint(handleX[level + end], handleY[level + end], side));
        for (int side = 0; side < 8; ++side)
            builder.Quad(side, (side + 1) % 8, 8 + (side + 1) % 8, 8 + side);
    }
    builder.Begin();
    builder.Add(Point(0, -27.8f, 0, { 147, 87, 26, 255 }));
    for (int i = 0; i < 8; ++i)
        builder.Add(HandlePoint(0.7f, -27.8f, i));
    for (int i = 0; i < 8; ++i)
        builder.Tri(0, i + 1, (i + 1) % 8 + 1);
    // Small chased ornaments and emeralds keep the silhouette close to the
    // handheld Magic Mirror, including when viewed in a 32-pixel inventory cell.
    Diamond(builder, 0, 19.5f, 2.3f, 2.0f, 1.35f, 0.35f, false);
    Diamond(builder, 0, 19.3f, 0.85f, 1.25f, 1.8f, 0.25f, true);
    for (int side : { -1, 1 }) {
        Diamond(builder, side * 11.2f, 10.0f, 1.15f, 2.6f, 1.2f, 0.35f, false);
        Diamond(builder, side * 6.6f, -11.4f, 1.2f, 2.0f, 1.2f, 0.35f, false);
    }
    Diamond(builder, 0, -25.5f, 1.0f, 1.35f, 0.95f, 0.45f, true);
    mesh.opaqueBatchCount = mesh.batchCount;

    // One double-sided, nearly opaque reflecting pane. It is recessed inside
    // the gold rim; the colored ripple never grows into a portal or outer aura.
    for (int segment = 0; segment < kRimSegments; ++segment) {
        builder.Begin();
        builder.Add(GlassPoint(0, 0, phase));
        for (int ring = 1; ring <= kGlassRings; ++ring)
            for (int end = 0; end < 2; ++end) {
                const float angle = ((segment + end) % kRimSegments) * (2 * kPi / kRimSegments);
                builder.Add(GlassPoint(ring / float(kGlassRings), angle, phase));
            }
        builder.Tri(0, 1, 2);
        for (int ring = 1; ring < kGlassRings; ++ring) {
            const int a = 1 + (ring - 1) * 2;
            builder.Quad(a, a + 2, a + 3, a + 1);
        }
    }
    // A slender reflected arc and two softly pulsing pinpoints, with transparent
    // edges. Shimmer is intentionally confined to the object.
    for (int segment = 0; segment < 8; ++segment) {
        builder.Begin();
        for (int end = 0; end < 2; ++end) {
            const float angle = (0.55f + (segment + end) * 0.067f) * kPi;
            const float fade = std::sin((segment + end) * (kPi / 8.0f));
            for (int edge = 0; edge < 2; ++edge) {
                const float radius = edge ? 0.81f : 0.77f;
                builder.Add(Point(std::cos(angle) * 10.15f * radius, 3 + std::sin(angle) * 14.6f * radius, 0.55f,
                                  { 207, 255, 238, Channel(fade * (55 + 15 * std::sin(phase))) }));
            }
        }
        builder.Quad(0, 1, 3, 2);
    }
    for (int glint = 0; glint < 2; ++glint) {
        const float x = glint ? 8.2f : -7.6f, y = glint ? -9.0f : 15.7f;
        const float pulse = 0.5f + 0.5f * std::sin(phase * 2 + glint * kPi);
        builder.Begin();
        builder.Add(Point(x, y, 1.95f, { 255, 248, 196, Channel(45 + pulse * 100) }));
        builder.Add(Point(x - 0.55f, y, 1.95f, { 255, 238, 166, 0 }));
        builder.Add(Point(x, y + 1.1f, 1.95f, { 255, 238, 166, 0 }));
        builder.Add(Point(x + 0.55f, y, 1.95f, { 255, 238, 166, 0 }));
        builder.Add(Point(x, y - 1.1f, 1.95f, { 255, 238, 166, 0 }));
        for (int i = 0; i < 4; ++i)
            builder.Tri(0, i + 1, (i + 1) % 4 + 1);
    }
}
} // namespace ComboMagicMirrorGeometry
