#pragma once

#include "soh/Enhancements/audio/SeasonWeather.h"

// Scope the renderer treatment to the selected foliage draw. This also reaches
// native and Alt model replacements without recoloring trunks or cut stumps.
static inline int SeasonFoliage_ActorBegin(Gfx** display, const PlayState* play, int canopy) {
    const int season = SeasonWeather_SeasonForPlay(play);
    unsigned red, green, blue, blend, mode;
    switch (season) {
        case 0:
            red = 246;
            green = 193;
            blue = 217;
            blend = canopy ? 224 : 32;
            mode = 2; // Mean-preserving partial blossom blend, as in MM.
            break;
        case 2:
            red = canopy ? 244 : 217;
            green = canopy ? 183 : 156;
            blue = canopy ? 131 : 69;
            blend = 255;
            mode = 1;
            break;
        case 3:
            red = canopy ? 204 : 229;
            green = canopy ? 220 : 237;
            blue = canopy ? 238 : 243;
            blend = 255;
            mode = 3; // Existing frost normalization preserves texture detail.
            break;
        default:
            return 0;
    }
    gDPSetGrayscaleColor((*display)++, red, green, blue, blend);
    gSPGrayscale((*display)++, mode);
    return 1;
}

static inline void SeasonFoliage_ActorEnd(Gfx** display, int active) {
    if (active) {
        gSPGrayscale((*display)++, false);
    }
}
