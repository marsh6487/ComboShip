/* Timing/motion invariants supplement the unchanged recorded appearance
 * golden. They do not regenerate reference geometry from the native renderer. */
#include "combo/render/DinsFireRemake.h"
#include "combo/render/DinsFireTiming.h"
#include <stdio.h>
#include <stdlib.h>

#define REQUIRE(c)                                                       \
    do {                                                                 \
        if (!(c)) {                                                      \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); \
            exit(1);                                                     \
        }                                                                \
    } while (0)

static const CwDinsVec right = { 1, 0, 0 }, up = { 0, 1, 0 };

static float angularStep(float after, float before) {
    float delta = after - before;
    if (delta > 3.141592653589793f)
        delta -= 6.283185307179586f;
    if (delta < -3.141592653589793f)
        delta += 6.283185307179586f;
    return delta;
}

int main(void) {
    CwDinsMesh mesh, original;
    float previousAngles[6] = { 0 }, previousS[6] = { 0 };
    /* Accepted rates expressed per 20 Hz update, independently of the native
     * implementation's time variable and any chosen hold duration. */
    const float angleRates[6] = { .165f, .1755f, .186f, .1965f, .207f, .2175f };
    const float scrollRates[6] = { -.864f, -.944f, -1.024f, -1.104f, -1.184f, -1.264f };
    REQUIRE(CW_DINS_NATIVE_FADE_START_FRAME == 126 && CW_DINS_NATIVE_FIRE_TICKS == 138);
    REQUIRE(CW_DINS_NATIVE_FADE_START_FRAME - CW_DINS_NATIVE_RECOVERY_TICKS == 6 * CW_DINS_NATIVE_RATE);
    CwDinsFireRemake_BuildNativeFire(&mesh, 52, right, up, 0xFFC800, 0xFF0000);
    REQUIRE(mesh.count == 590); /* The old expiry is now ordinary moving fire. */
    for (int frame = 0; frame < CW_DINS_NATIVE_FIRE_TICKS; ++frame) {
        CwDinsFireRemake_BuildNativeFire(&mesh, frame, right, up, 0xFFC800, 0xFF0000);
        REQUIRE(mesh.count >= 590 && mesh.count <= 596);
        if (frame <= 40) {
            CwDinsFireRemake_BuildFireRecipe(&original, frame, 40, 12, right, up, 0xFFC800, 0xFF0000);
            REQUIRE(mesh.count == original.count);
            for (size_t i = 0; i < mesh.count; ++i) {
                const CwDinsQuad* current = &mesh.quads[i];
                const CwDinsQuad* accepted = &original.quads[i];
                REQUIRE(current->s == accepted->s && current->t == accepted->t);
                REQUIRE(current->sSpan == accepted->sSpan && current->tSpan == accepted->tSpan);
                REQUIRE(current->primary == accepted->primary && current->secondary == accepted->secondary);
                for (int v = 0; v < 4; ++v) {
                    REQUIRE(current->p[v].x == accepted->p[v].x && current->p[v].y == accepted->p[v].y &&
                            current->p[v].z == accepted->p[v].z && current->alpha[v] == accepted->alpha[v]);
                }
            }
        }
        if (frame >= CW_DINS_NATIVE_GROW_TICKS && frame <= CW_DINS_NATIVE_FADE_START_FRAME) {
            REQUIRE(mesh.quads[0].alpha[0] == 147);
            REQUIRE(CwDins_NativeRadius(frame / (float)CW_DINS_NATIVE_RATE) == CwDins_NativeRadius(.4f));
            for (int band = 0; band < 6; ++band) {
                const CwDinsQuad* q = &mesh.quads[4 + band * 96];
                REQUIRE(q->burn == 0 && q->burnFrame == 0);
                float angle = atan2f(q->p[0].z, q->p[0].x);
                REQUIRE(fabsf(q->s - frame * scrollRates[band]) < .00005f);
                if (frame > CW_DINS_NATIVE_GROW_TICKS) {
                    REQUIRE(fabsf(angularStep(angle, previousAngles[band]) - angleRates[band]) < .00001f);
                    REQUIRE(fabsf(q->s - previousS[band] - scrollRates[band]) < .00005f);
                }
                previousAngles[band] = angle;
                previousS[band] = q->s;
            }
        }
        if (frame >= CW_DINS_NATIVE_FADE_START_FRAME) {
            int phase = frame - CW_DINS_NATIVE_FADE_START_FRAME;
            CwDinsFireRemake_BuildFireRecipe(&original, 40 + phase, 40, 12, right, up, 0xFFC800, 0xFF0000);
            REQUIRE(mesh.count == original.count);
            REQUIRE(abs(mesh.quads[0].alpha[0] - original.quads[0].alpha[0]) <= 1);
            REQUIRE(fabsf(mesh.quads[0].p[1].x - original.quads[0].p[1].x) < .0001f);
            for (int band = 0; band < 6; ++band) {
                const CwDinsQuad* q = &mesh.quads[4 + band * 96];
                REQUIRE(q->burn == original.quads[4 + band * 96].burn);
                REQUIRE(q->burnFrame == (phase ? frame : 0));
            }
            if (phase == 8) {
                REQUIRE(mesh.count > 590);
                CwDinsVec delta = CwDins_Sub(mesh.quads[590].p[0], original.quads[590].p[0]);
                REQUIRE(CwDins_Dot(delta, delta) > 1); /* Embers use the later moving ribbon anchors. */
            }
        }
    }
    CwDinsFireRemake_BuildNativeFire(&mesh, CW_DINS_NATIVE_FIRE_TICKS, right, up, 0xFFC800, 0xFF0000);
    REQUIRE(mesh.count == 0);
    CwDinsFireRemake_BuildFireRecipe(&original, 52, 40, 12, right, up, 0xFFC800, 0xFF0000);
    REQUIRE(original.count == 0);
    puts("PASS native linger lifetime: 138 fire updates; six usable seconds after recovery before 12-update burnout");
    puts("PASS continuous motion: accepted angular and unwrapped UV steps through every hold update; no loop reset or "
         "slowdown");
    puts("PASS preserved recipe: initial 41 updates unchanged; original fade envelope shifted 86 updates with later "
         "ember anchors");
    return 0;
}
