#ifdef SWORD_KALEIDO_MM
#include "global.h"
#else
#include "global.h"
#endif
#include "sword_element.h"
#include "sword_element_kaleido.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static GraphicsContext sGfx;
static PlayState sPlay;
static Gfx sOpa[1024], sOverlay[1024], sWork[32], sXlu[32];
static Gfx sArena[256];
static Vtx sVertices[4];
static char sText[256];
static size_t sTextLength;
static u32 sLastPrintColor;
static u8 sIceLabelGray;
static u8 sFailAllocation;
SaveContext gSaveContext;
void gSPVertex(Gfx* packet, uintptr_t vertices, int count, int first) { __gSPVertex(packet, vertices, count, first); }
uint8_t SwordElement_IsAvailable(uint8_t element) { return element != SWORD_ELEMENT_ICE; }
void FrameInterpolation_RecordOpenChild(const void* key, int id) {}
void FrameInterpolation_RecordCloseChild(void) {}
#ifdef SWORD_KALEIDO_MM
void Gfx_SetupDL42_Opa(GraphicsContext* context) {}
void Gfx_SetupDL39_Overlay(GraphicsContext* context) {}
void Graph_OpenDisps(Gfx** refs, Gfx* vals, GraphicsContext* context, const char* file, s32 line) {}
void Graph_CloseDisps(Gfx** refs, Gfx* vals, GraphicsContext* context, const char* file, s32 line) {}
#else
void Gfx_SetupDL_42Opa(GraphicsContext* context) {}
void Gfx_SetupDL_39Overlay(GraphicsContext* context) {}
void Graph_OpenDisps(Gfx** refs, GraphicsContext* context, const char* file, s32 line) {}
void Graph_CloseDisps(Gfx** refs, GraphicsContext* context, const char* file, s32 line) {}
void* Graph_Alloc(GraphicsContext* context, size_t size) {
    assert(size == sizeof(sVertices));
    return sFailAllocation ? NULL : sVertices;
}
#endif
/* The native printer boundary records requested text/color. Geometry and texture commands remain real GBI. */
void GfxPrint_Init(GfxPrint* p) { memset(p, 0, sizeof(*p)); }
void GfxPrint_Destroy(GfxPrint* p) {}
void GfxPrint_Open(GfxPrint* p, Gfx* display) { p->dList = display; }
Gfx* GfxPrint_Close(GfxPrint* p) { return p->dList; }
void GfxPrint_SetPosPx(GfxPrint* p, s32 x, s32 y) { assert(x >= 80 && x <= 220 && y >= 130 && y <= 208); }
void GfxPrint_SetColor(GfxPrint* p, u32 r, u32 g, u32 b, u32 a) { sLastPrintColor = r << 16 | g << 8 | b; }
s32 GfxPrint_Printf(GfxPrint* p, const char* fmt, ...) {
    char text[64];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);
    if (!strcmp(text, "Ice")) sIceLabelGray = sLastPrintColor == 0x646464;
    assert(sTextLength + n < sizeof(sText));
    memcpy(sText + sTextLength, text, n + 1);
    sTextLength += n;
    return n;
}
static void reset_draw(void) {
    memset(&sGfx, 0, sizeof(sGfx));
    memset(sOpa, 0, sizeof(sOpa));
    memset(sText, 0, sizeof(sText));
    sTextLength = sIceLabelGray = 0;
    sGfx.polyOpa.p = sOpa;
    sGfx.polyOpa.d = sArena + 256;
    sGfx.polyXlu.p = sXlu;
    sGfx.overlay.p = sOverlay;
    sGfx.work.p = sWork;
    sPlay.state.gfxCtx = &sGfx;
    sPlay.pauseCtx.alpha = 255;
}
static int texture(const char* fragment) {
    Gfx* starts[] = { sOpa, sOverlay };
    Gfx* ends[] = { sGfx.polyOpa.p, sGfx.overlay.p };
    for (int i = 0; i < 2; ++i) for (Gfx* p = starts[i]; p < ends[i]; ++p) {
        if ((p->words.w0 >> 24) == G_SETTIMG && strstr((const char*)p->words.w1, fragment)) return 1;
    }
    return 0;
}
static int command(int opcode, uintptr_t word1) {
    Gfx* starts[] = { sOpa, sOverlay };
    Gfx* ends[] = { sGfx.polyOpa.p, sGfx.overlay.p };
    for (int i = 0; i < 2; ++i) for (Gfx* p = starts[i]; p < ends[i]; ++p) {
        if ((p->words.w0 >> 24) == opcode && p->words.w1 == word1) return 1;
    }
    return 0;
}
static Vtx* vertices(void) {
#ifdef SWORD_KALEIDO_MM
    return (Vtx*)sGfx.polyOpa.d;
#else
    return sVertices;
#endif
}
int main(void) {
    SwordElementMenu menu = { 0 };
    SaveContext saveBefore;
    PauseContext pauseBefore;
    memset(&gSaveContext, 0x5a, sizeof(gSaveContext));
    reset_draw();
    SwordElementKaleido_Draw(&sPlay, &menu, (void*)"SwordIcon");
    assert(sGfx.polyOpa.p == sOpa && !sTextLength);
    menu.phase = SWORD_ELEMENT_MENU_CHOOSER;
    menu.selection = SWORD_ELEMENT_FIRE;
    saveBefore = gSaveContext;
    pauseBefore = sPlay.pauseCtx;
    SwordElementKaleido_Draw(&sPlay, &menu, (void*)"SwordIcon");
    assert(sGfx.polyOpa.p == sOpa && sGfx.overlay.p > sOverlay); /* fixed-screen chooser */
    assert(texture("SwordIcon") && texture("EquippedItemOutline"));
#ifdef SWORD_KALEIDO_MM
    assert(texture("FireArrow") && texture("IceArrow") && texture("LightArrow"));
#else
    assert(texture("ArrowFire") && texture("ArrowIce") && texture("ArrowLight"));
#endif
    assert(!texture("MagicArrowEquip"));
    assert(command(G_SETGRAYSCALE, true) && command(G_SETGRAYSCALE, false));
    assert(sIceLabelGray && strstr(sText, "NoneFireIceLight") && strstr(sText, "A Confirm  B Cancel"));
    assert(!memcmp(&pauseBefore, &sPlay.pauseCtx, sizeof(pauseBefore)));
    assert(!memcmp(&saveBefore, &gSaveContext, sizeof(saveBefore)));

    reset_draw();
    menu.phase = SWORD_ELEMENT_MENU_HOLD_SWORD;
    menu.selection = SWORD_ELEMENT_ICE;
    menu.alpha = 254;
    menu.x = 120;
    menu.y = 440;
    SwordElementKaleido_Draw(&sPlay, &menu, (void*)"SwordIcon");
    assert(texture("MagicArrowEquipEffectTex"));
    assert(command(G_SETPRIMCOLOR, 0x6464FFFE));
    assert(sGfx.overlay.p == sOverlay && !sTextLength); /* picker closed during fusion */
    Vtx* v = vertices();
    assert(v[0].v.ob[0] == -3 && v[0].v.ob[1] == 59);
    assert(v[3].v.ob[0] == 59 && v[3].v.ob[1] == -3);
    assert(!memcmp(&pauseBefore, &sPlay.pauseCtx, sizeof(pauseBefore)));
    assert(!memcmp(&saveBefore, &gSaveContext, sizeof(saveBefore)));
    reset_draw();
    sPlay.pauseCtx.alpha = 0;
    SwordElementKaleido_Draw(&sPlay, &menu, (void*)"SwordIcon");
    assert(sGfx.polyOpa.p == sOpa && !sTextLength);
    puts("sword elemental Kaleido rendering commands: PASS");
}
