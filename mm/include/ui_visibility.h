#ifndef MM_UI_VISIBILITY_H
#define MM_UI_VISIBILITY_H

/* Draw paths also advance text, photos, clocks and menus. Keep that work and
 * its allocations alive; hide only commands submitted by this UI draw. */
typedef struct {
    Gfx* work;
    Gfx* opa;
    Gfx* xlu;
    Gfx* overlay;
    Gfx* debug;
    int discard;
} MmUiDrawState;

static inline MmUiDrawState MmUiDraw_Begin(GraphicsContext* gfx, int discard) {
    return (MmUiDrawState){ gfx->work.p, gfx->polyOpa.p, gfx->polyXlu.p, gfx->overlay.p, gfx->debug.p, discard };
}

static inline void MmUiDraw_End(GraphicsContext* gfx, MmUiDrawState state) {
    if (state.discard) {
        gfx->work.p = state.work;
        gfx->polyOpa.p = state.opa;
        gfx->polyXlu.p = state.xlu;
        gfx->overlay.p = state.overlay;
        gfx->debug.p = state.debug;
    }
}

#endif
