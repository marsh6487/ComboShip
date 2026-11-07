#!/usr/bin/env python3
"""Execute native MM draw entrypoints: hidden commands, live draw-side state."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r'^[^\n;{}]*\b' + re.escape(name) + r'\([^;{}]*\)\s*\{', source, re.M)
    if match is None:
        raise RuntimeError(name)
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


prelude = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
typedef int32_t s32;
typedef struct { uintptr_t command; } Gfx;
typedef struct { Gfx *p, *d; } Arena;
typedef struct { Arena work, polyOpa, polyXlu, overlay, debug; } GraphicsContext;
typedef struct { GraphicsContext *gfxCtx; } GameState;
typedef struct { int currentTextId; } MessageContext;
typedef struct { GameState state; MessageContext msgCtx; } PlayState;
typedef struct { int unused; } Actor;
static int hidden, stateAdvances, inCutscene;
static inline int UiVisibility_IsHidden(void) { return hidden; }
#define COMBO_GUI_UI_VISIBILITY_H
#if __has_include("ui_visibility.h")
#include "ui_visibility.h"
#endif
#define OPEN_DISPS(gfx) { GraphicsContext *__gfxCtx = (gfx)
#define CLOSE_DISPS(gfx) }
#define POLY_OPA_DISP __gfxCtx->polyOpa.p
#define OVERLAY_DISP __gfxCtx->overlay.p
#define gSPDisplayList(pkt, list) ((pkt)->command = (uintptr_t)(list))
#define gSPEndDisplayList(pkt) ((pkt)->command = 0xDF)
static Gfx *Graph_GfxPlusOne(Gfx *p) { return p + 1; }
static void Graph_BranchDlist(Gfx *p, Gfx *next) { p->command = (uintptr_t)next; }
static bool Play_InCsMode(PlayState *play) { return inCutscene; }
// The renderer/text interpreter is the boundary: native wrappers must still call
// it, even when their resulting command streams will not be submitted.
static void Message_DrawMain(PlayState *play, Gfx **p) {
    ++stateAdvances;
    (*p)++;
    --play->state.gfxCtx->polyOpa.d;
    ++play->state.gfxCtx->polyXlu.p;
    ++play->state.gfxCtx->work.p;
    ++play->state.gfxCtx->debug.p;
}
static void EnMag_DrawInner(Actor *actor, PlayState *play, Gfx **p) {
    Message_DrawMain(play, p);
}
'''

test = r'''
static void check(void (*draw)(PlayState *), bool hide) {
    Gfx work[64], opa[64], xlu[64], overlay[64], debug[64];
    GraphicsContext gfx = { { work, work+64 }, { opa, opa+64 },
                            { xlu, xlu+64 }, { overlay, overlay+64 }, { debug, debug+64 } };
    PlayState play = { .state = { &gfx } };
    hidden = hide;
    stateAdvances = 0;
    draw(&play);
    assert(stateAdvances == 1);
    assert(gfx.polyOpa.d == opa+63); // Allocation lifetime must survive.
    if (hide) {
        assert(gfx.work.p == work && gfx.polyOpa.p == opa &&
               gfx.polyXlu.p == xlu && gfx.overlay.p == overlay && gfx.debug.p == debug);
    } else {
        assert(gfx.work.p == work+1 && gfx.polyOpa.p > opa &&
               gfx.polyXlu.p == xlu+1 && gfx.overlay.p == overlay+1);
    }
}
static void drawTitle(PlayState *play) { Actor actor = {0}; EnMag_Draw(&actor, play); }
int main(void) {
    check(Message_Draw, false);
    check(Message_Draw, true);
    check(Message_Draw, false); // No stale hidden state after toggling back.
    check(drawTitle, false);
    check(drawTitle, true);
    check(drawTitle, false);
    puts("PASS MM message/title submissions: normal, hidden, restored; draw-side state and allocations live");
}
'''

def main():
    source = prelude + function((ROOT/'mm/src/code/z_message.c').read_text(), 'Message_Draw')
    source += function((ROOT/'mm/src/overlays/actors/ovl_En_Mag/z_en_mag.c').read_text(), 'EnMag_Draw') + test
    with tempfile.TemporaryDirectory(prefix='mm-ui-draw-') as directory:
        path = Path(directory)/'test.c'
        path.write_text(source)
        binary = Path(directory)/'test'
        subprocess.run(['cc', '-std=gnu11', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-no-pie', '-I'+str(ROOT/'mm/include'), '-I'+str(ROOT/'combo'),
                        str(path), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})


if __name__ == "__main__":
    main()
