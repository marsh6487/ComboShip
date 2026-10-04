#ifndef COMBO_SONG_DRAW_H
#define COMBO_SONG_DRAW_H
#include <stdint.h>

// Stable presentation indices follow MM's native song block, independently
// of either game's item/GI numeric values.
enum {
    CW_SONG_DOUBLE_TIME,
    CW_SONG_ELEGY,
    CW_SONG_EPONA,
    CW_SONG_HEALING,
    CW_SONG_INVERTED_TIME,
    CW_SONG_LULLABY_INTRO,
    CW_SONG_LULLABY,
    CW_SONG_NOVA,
    CW_SONG_OATH,
    CW_SONG_SARIA,
    CW_SONG_SOARING,
    CW_SONG_SONATA,
    CW_SONG_STORMS,
    CW_SONG_SUN,
    CW_SONG_TIME
};
static inline int ComboSongShimmerColor(int song, uint8_t color[4]) {
    static const uint8_t colors[15][4] = { { 98, 177, 211, 255 },  { 255, 98, 0, 255 },    { 146, 87, 49, 255 },
                                           { 255, 150, 230, 255 }, { 98, 177, 211, 255 },  { 255, 100, 100, 255 },
                                           { 255, 20, 20, 255 },   { 20, 20, 255, 255 },   { 98, 0, 98, 255 },
                                           { 98, 255, 98, 255 },   { 200, 160, 255, 255 }, { 98, 255, 98, 255 },
                                           { 146, 146, 146, 255 }, { 237, 231, 62, 255 },  { 98, 177, 211, 255 } };
    if (song < 0 || song >= 15)
        return 0;
    for (int i = 0; i < 4; ++i)
        color[i] = colors[song][i];
    return 1;
}
#endif
