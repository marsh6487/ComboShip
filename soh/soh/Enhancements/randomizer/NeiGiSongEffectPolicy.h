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
    if (song < 0 || song >= CW_SONG_COUNT || song == CW_SONG_STORMS)
        return mesh; // Storms is the separate native rain/lightning recipe.
    const uint32_t color = ComboSongColorHex(song);
    const float t = (frame % 720u) * (Tau / 360.f);
    if (song == CW_SONG_SOARING) {
        for (int i = 0; i < 6; ++i) {
            const float phase = float((frame % 240u + i * 40u) % 240u) / 240.f;
            const float angle = t * .45f + i * Tau / 6;
            const uint8_t alpha = uint8_t(225 * std::fmin(1.f, std::fmin(phase, 1 - phase) * 7));
            SongFeather(mesh, { 23 * std::cos(angle), 31 - 62 * phase, 23 * std::sin(angle) },
                        .7f * std::sin(t + i) + i, color, alpha, camera);
        }
        return mesh;
    }
    if (song == CW_SONG_HEALING) {
        for (int i = 0; i < 6; ++i) {
            const float phase = float((frame % 240u + i * 40u) % 240u) / 240.f;
            SongHeart(mesh, { 21 * std::cos(t * .5f + i), 48 * phase - 24, 21 * std::sin(t * .5f + i) }, 2.8f, color,
                      uint8_t(220 * std::sin(phase * Tau * .5f)), camera);
        }
        return mesh;
    }
    const bool clock = song == CW_SONG_DOUBLE_TIME || song == CW_SONG_INVERTED_TIME || song == CW_SONG_TIME;
    const bool water = song == CW_SONG_NOVA || song == CW_SONG_OOT_SERENADE;
    const bool sun = song == CW_SONG_SUN || song == CW_SONG_OOT_PRELUDE;
    const bool leaves =
        song == CW_SONG_SARIA || song == CW_SONG_SONATA || song == CW_SONG_OOT_SARIA || song == CW_SONG_OOT_MINUET;
    const bool horse = song == CW_SONG_EPONA || song == CW_SONG_OOT_EPONA;
    if (clock || water || sun || horse || song == CW_SONG_OATH || song == CW_SONG_ELEGY) {
        const int rings = song == CW_SONG_DOUBLE_TIME ? 2 : song == CW_SONG_OATH ? 4 : horse ? 3 : 1;
        for (int ring = 0; ring < rings; ++ring) {
            const auto point = [&](int j) {
                const float direction = song == CW_SONG_INVERTED_TIME ? -1.f : 1.f;
                const float arc = horse ? .72f : 1.f;
                const float a = j * Tau * arc / 16 + direction * t * .5f + ring * Tau / rings;
                const float radius = horse ? 4.f : 20.f + ring * 3.f;
                const Point center =
                    horse ? Point{ 20 * std::cos(ring * 2.4f), 10 * std::sin(t + ring), 20 * std::sin(ring * 2.4f) }
                          : Point{};
                return center + (water ? Point{ radius * std::cos(a), 3.f * std::sin(2 * a + t), radius * std::sin(a) }
                                       : Plane(camera, radius * std::cos(a), radius * std::sin(a), ring * .45f));
            };
            for (int j = 0; j < 16; ++j)
                Band(mesh, point(j), point(j + 1), horse ? .5f : .65f, color, 0xFFF8FF, camera,
                     uint8_t(120 + 90 * (.5f + .5f * std::sin(t + j * .3f))));
            if (clock) {
                for (int j = 0; j < 12; ++j) {
                    const Point p = point(j * 16 / 12);
                    Band(mesh, p * .93f, p * 1.07f, .24f, color, 0xFFFFFF, camera);
                }
            }
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
        } else if (song == CW_SONG_OOT_BOLERO) {
            Band(mesh, p, p + Point{ 1, 5, 0 }, .9f, color, 0xFFF1C0, camera, alpha);
        } else {
            // Lullaby/Nocturne/Zelda/Requiem retain their exact recovered hue
            // with slow, bounded song motes rather than a generic item shimmer.
            Glow(mesh, p, 2.5f, color, alpha, camera);
        }
    }
    return mesh;
}
} // namespace NeiGi
