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

inline Mesh SampleSong(int song, uint32_t frame, const Basis& camera = {}) {
    Mesh mesh;
    const bool warp = song >= CW_SONG_OOT_MINUET && song <= CW_SONG_OOT_PRELUDE;
    const bool approved =
        song == CW_SONG_OOT_ZELDA || song == CW_SONG_SARIA || song == CW_SONG_OOT_SARIA || song == CW_SONG_SOARING;
    if (!warp && !approved)
        return mesh; // Plain regular notes; Epona/Sun use only the shared shimmer, Storms uses rain.
    const uint32_t color = ComboSongColorHex(song);
    const float t = (frame % 720u) * (Tau / 360.f);
    if (song == CW_SONG_OOT_BOLERO || song == CW_SONG_OOT_SERENADE)
        return mesh; // Keep the native note and shared shimmer; no encircling overlay.
    if (song == CW_SONG_SOARING) {
        for (int i = 0; i < 6; ++i) {
            const float phase = float((frame % 240u + i * 40u) % 240u) / 240.f;
            const float angle = i * Tau / 6 + t * .25f;
            const Point p{ 23 * std::cos(angle), 28 - 56 * phase, 23 * std::sin(angle) };
            SongFeather(mesh, p, t * .4f + i, color, uint8_t(210 * std::sin(phase * Tau * .5f)), camera);
        }
        return mesh;
    }
    const bool light = song == CW_SONG_OOT_PRELUDE;
    const bool leaves = song == CW_SONG_SARIA || song == CW_SONG_OOT_SARIA || song == CW_SONG_OOT_MINUET;
    for (int i = 0; i < 9; ++i) {
        const float phase = float((frame % 240u + i * 27u) % 240u) / 240.f;
        const float angle = i * 2.399963f + t * .4f;
        Point p{ 23 * std::cos(angle), light ? -28 + 56 * phase : 28 - 56 * phase, 23 * std::sin(angle) };
        const uint8_t alpha = uint8_t(210 * std::sin(phase * Tau * .5f));
        if (leaves) {
            const float roll = t + i;
            const Point along = Plane(camera, std::cos(roll) * 3, std::sin(roll) * 3);
            const Point across = Plane(camera, -std::sin(roll) * 1.4f, std::cos(roll) * 1.4f);
            mesh.Tri({ p + along, color, alpha }, { p + across, color, alpha }, { p - along, 0xD8FFD0, alpha });
            mesh.Tri({ p + along, color, alpha }, { p - along, 0xD8FFD0, alpha }, { p - across, color, alpha });
        } else {
            // Prelude has rising light motes; Zelda/Nocturne/Requiem keep their
            // existing bounded particles.
            Glow(mesh, p, 2.5f, color, alpha, camera);
        }
    }
    return mesh;
}
} // namespace NeiGi
