#ifndef OOT_UI_VISIBILITY_H
#define OOT_UI_VISIBILITY_H

#include "global.h"
#include "../../combo/gui/UiVisibility.h"

/* Native UI draw routines also advance text, timer, cursor and animation state.
 * Run them normally, then discard only their command heads. Keep tail-side
 * Graph_Alloc storage alive for any state produced during the draw. */
typedef struct {
    Gfx* work;
    Gfx* opa;
    Gfx* xlu;
    Gfx* overlay;
    int hidden;
} OotUiDrawScope;

static inline OotUiDrawScope OotUi_BeginDraw(GraphicsContext* gfxCtx) {
    OotUiDrawScope scope = {
        gfxCtx->work.p, gfxCtx->polyOpa.p, gfxCtx->polyXlu.p, gfxCtx->overlay.p, UiVisibility_IsHidden(),
    };
    return scope;
}

static inline void OotUi_EndDraw(GraphicsContext* gfxCtx, OotUiDrawScope scope) {
    if (scope.hidden) {
        gfxCtx->work.p = scope.work;
        gfxCtx->polyOpa.p = scope.opa;
        gfxCtx->polyXlu.p = scope.xlu;
        gfxCtx->overlay.p = scope.overlay;
    }
}

#endif
