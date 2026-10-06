#pragma once
#include "NeiGiEffectPolicy.h"
#include "ComboSongDraw.h"

namespace NeiGi {
// A curved vane with a raised rachis and tapered, notched barbs. It is a
// triangulated feather silhouette, independently of any archive model.
inline void SongFeather(Mesh& mesh, Point center, float roll, uint32_t color, uint8_t alpha, const Basis& camera) {
    const auto point = [&](float u, float side) {
        const float bend = .65f * std::sin(u * Tau * .5f);
        const float width = 1.8f * std::sin(u * Tau * .5f) * (1.f - .22f * u);
        const float x = bend + side * width;
        const float y = 12.f * (u - .5f);
        return center + Plane(camera, x * std::cos(roll) - y * std::sin(roll), x * std::sin(roll) + y * std::cos(roll),
                              .6f * std::sin(u * Tau * .5f) - .35f * std::abs(side));
    };
    for (int j = 0; j < 6; ++j) {
        const float u = j / 6.f, next = (j + 1) / 6.f;
        for (float side : { -1.f, 1.f }) {
            const Point a = point(u, 0), b = point(u, side), c = point(next, side), d = point(next, 0);
            mesh.Tri({ a, 0xFFF9FF, alpha }, { b, color, alpha }, { c, color, alpha });
            mesh.Tri({ a, 0xFFF9FF, alpha }, { c, color, alpha }, { d, 0xFFF9FF, alpha });
            // Individual barb veins make the two vanes readable as a feather.
            const Point mid = point((u + next) * .5f, side * .88f);
            mesh.Tri({ a, color, alpha }, { mid, 0xEEE1FF, alpha }, { d, color, alpha });
        }
        Band(mesh, point(u, 0), point(next, 0), .12f, color, 0xFFF9FF, camera, alpha);
    }
}

inline void SongHeart(Mesh& mesh, Point center, float radius, uint32_t color, uint8_t alpha, const Basis& camera) {
    for (int j = 0; j < 20; ++j) {
        const auto point = [&](int k) {
            const float a = k * Tau / 20, s = std::sin(a);
            return center + Plane(camera, radius * s * s * s,
                                  radius * (.8125f * std::cos(a) - .3125f * std::cos(2 * a) - .125f * std::cos(3 * a) -
                                            .0625f * std::cos(4 * a)));
        };
        mesh.Tri({ center, 0xFFF0FB, alpha }, { point(j), color, alpha }, { point(j + 1), color, alpha });
    }
}

// A filled fire silhouette with three joined tongues and a broad yellow core.
// The old transparent ribbons made only their thin centers visible, producing
// detached red squiggles even when their outer geometry was wide.
inline void SongFlame(Mesh& mesh, Point origin, float height, float width, float phase, float opacity,
                      const Basis& camera) {
    const Point axis{ 0, 1, 0 };
    Point side = Cross(axis, camera.forward);
    if (side.x * side.x + side.y * side.y + side.z * side.z < .001f)
        side = camera.right;
    side = Unit(side);
    const auto tongue = [&](Point base, float length, float breadth, float bend, bool core) {
        const auto center = [&](float u) {
            return base + axis * (length * u) +
                   side * (bend * u * u + .12f * breadth * std::sin(phase + u * 2.f) * u * u);
        };
        const auto edge = [&](float u) {
            return side * (breadth * std::sin((.2f + .8f * u) * Tau * .5f) * (1.f - .22f * u));
        };
        const auto vertex = [&](float u, float across) {
            const uint32_t middle = core ? (u < .6f ? 0xFFF2AD : 0xFFD35B) : (u < .6f ? 0xFFAF36 : 0xFF7D20);
            const uint32_t rim = core ? middle : 0xF45D19;
            const uint8_t alpha = uint8_t((core ? 245.f : (across ? 155.f : 225.f)) * opacity);
            return EffectVertex{ center(u) + edge(u) * across, across ? rim : middle, u >= 1.f ? uint8_t(0) : alpha };
        };
        for (int j = 0; j < 5; ++j) {
            const float u = j / 5.f, next = (j + 1) / 5.f;
            for (float across : { -1.f, 1.f }) {
                mesh.Tri(vertex(u, 0), vertex(u, across), vertex(next, across));
                mesh.Tri(vertex(u, 0), vertex(next, across), vertex(next, 0));
            }
        }
    };
    // Short side tongues share the main flame's base and split toward their
    // tips. Only the tips flicker sideways; the body stays broad and upright.
    tongue(origin - side * (width * .48f), height * .67f, width * .56f, -width * .72f, false);
    tongue(origin + side * (width * .43f), height * .78f, width * .5f, width * .62f, false);
    tongue(origin, height, width, width * .12f * std::sin(phase), false);
    tongue(origin + axis * (height * .04f), height * .61f, width * .48f, 0, true);
}

inline Mesh SampleSong(int song, uint32_t frame, const Basis& camera = {}) {
    Mesh mesh;
    const bool warp = song >= CW_SONG_OOT_MINUET && song <= CW_SONG_OOT_PRELUDE;
    const bool approved = song == CW_SONG_OOT_ZELDA || song == CW_SONG_SARIA || song == CW_SONG_OOT_SARIA;
    if (!warp && !approved)
        return mesh; // Plain regular notes; Epona/Sun use only the shared shimmer, Storms uses rain.
    const uint32_t color = ComboSongColorHex(song);
    const float t = (frame % 720u) * (Tau / 360.f);
    if (song == CW_SONG_OOT_BOLERO) {
        for (int i = 0; i < 6; ++i) {
            const float phase = float((frame % 120u + i * 20u) % 120u) / 120.f;
            const float angle = i * Tau / 6 + t * .25f;
            const Point p{ 21 * std::cos(angle), -24 + phase * 38, 21 * std::sin(angle) };
            SongFlame(mesh, p, 17 + 2 * std::sin(t * 2 + i), 6.6f, t * 4 + i, .7f + .3f * std::sin(phase * Tau * .5f),
                      camera);
        }
        return mesh;
    }
    if (song == CW_SONG_OOT_SERENADE)
        return mesh; // Keep the native note and shared shimmer; no encircling overlay.
    const bool sun = song == CW_SONG_OOT_PRELUDE;
    const bool leaves = song == CW_SONG_SARIA || song == CW_SONG_OOT_SARIA || song == CW_SONG_OOT_MINUET;
    if (sun) {
        for (int ring = 0; ring < 1; ++ring) {
            const auto point = [&](int j) {
                const float a = j * Tau / 16 + t * .5f;
                return Plane(camera, 20 * std::cos(a), 20 * std::sin(a));
            };
            for (int j = 0; j < 16; ++j)
                Band(mesh, point(j), point(j + 1), .65f, color, 0xFFF8FF, camera,
                     uint8_t(120 + 90 * (.5f + .5f * std::sin(t + j * .3f))));
        }
        if (sun)
            Glow(mesh, {}, 28.f, color, 85, camera);
        return mesh;
    }
    for (int i = 0; i < 9; ++i) {
        const float phase = float((frame % 240u + i * 27u) % 240u) / 240.f;
        const float angle = i * 2.399963f + t * .4f;
        Point p{ 23 * std::cos(angle), 28 - 56 * phase, 23 * std::sin(angle) };
        const uint8_t alpha = uint8_t(210 * std::sin(phase * Tau * .5f));
        if (leaves) {
            const float roll = t + i;
            const Point along = Plane(camera, std::cos(roll) * 3, std::sin(roll) * 3);
            const Point across = Plane(camera, -std::sin(roll) * 1.4f, std::cos(roll) * 1.4f);
            mesh.Tri({ p + along, color, alpha }, { p + across, color, alpha }, { p - along, 0xD8FFD0, alpha });
            mesh.Tri({ p + along, color, alpha }, { p - along, 0xD8FFD0, alpha }, { p - across, color, alpha });
        } else {
            // Zelda/Nocturne/Requiem retain their existing bounded song motes.
            Glow(mesh, p, 2.5f, color, alpha, camera);
        }
    }
    return mesh;
}
} // namespace NeiGi
