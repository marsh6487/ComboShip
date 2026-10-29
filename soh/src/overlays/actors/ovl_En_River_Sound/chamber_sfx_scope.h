#ifndef CHAMBER_SFX_SCOPE_H
#define CHAMBER_SFX_SCOPE_H

/* Bounds of the accepted Djipi enclosure; z=-1200 is its tunnel mouth.
 * Comparisons fail closed for NaN and infinities. No persistent audio state. */
static int ChamberSfx_Contains(float x, float y, float z) {
    return x >= -1060.0f && x <= 1071.0f &&
           y >= -2137.0f && y <= 2574.0f &&
           z >= -3651.0f && z <= -1200.0f;
}

#endif
