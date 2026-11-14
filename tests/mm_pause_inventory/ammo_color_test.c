/* Production MM pause renderer with native save/graphics types and a digit-draw boundary. */
#include "global.h"
#include "mods/extended_inventory.h"
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

SaveContext gSaveContext;
static SaveContext before;
static GraphicsContext gfx;
static PauseContext pause;
static Gfx opa[128], xlu[16], overlay[16];
static unsigned digitColor[2];
static int digitCount, shownNumber;
static int policy = -1;
static ItemId expectedItem;
static u8 seeds;
const char* gAmmoDigitTextures[10] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };

bool GameInteractor_Should(GIVanillaBehavior flag, uint32_t result, ...) {
    if (flag == VB_ITEM_BE_RESTRICTED) {
        va_list args;
        va_start(args, result);
        ItemId* item = va_arg(args, ItemId*);
        assert(*item == expectedItem);
        va_end(args);
        if (policy >= 0) {
            return policy;
        }
    }
    return result;
}

void FrameInterpolation_RecordOpenChild(const void* child, int id) {}
void FrameInterpolation_RecordCloseChild(void) {}
u8 Bottle_BottomlessOwned(void) { return 0; }
u8 Bottle_BottomlessCount(void) { return 0; }
u8 Nei_SlingshotSeeds(void) { return seeds; }
u8 Nei_SlingshotCapacity(void) { return 30; }

/* Read the real G_SETPRIMCOLOR commands at each production digit draw. */
Gfx* Gfx_DrawTexRectIA8(Gfx* next, TexturePtr texture, s16 tw, s16 th, s16 left, s16 top,
                      s16 width, s16 height, u16 dsdx, u16 dtdy) {
    unsigned rgba = 0;
    for (Gfx* command = opa; command < next; ++command) {
        if (((u32)command->words.w0 >> 24) == G_SETPRIMCOLOR) {
            rgba = (u32)command->words.w1;
        }
    }
    assert(digitCount < 2);
    digitColor[digitCount++] = rgba;
    int digit = -1;
    for (int i = 0; i < 10; i++) {
        if (texture == gAmmoDigitTextures[i]) {
            digit = i;
        }
    }
    assert(digit >= 0);
    shownNumber = shownNumber * 10 + digit;
    return next;
}

#include "ammo_production.inc"

static int check(const char* label, int form, int override, ItemId item, int count, unsigned want) {
    memset(&gSaveContext, 0, sizeof(gSaveContext));
    memset(&gfx, 0, sizeof(gfx));
    memset(&pause, 0, sizeof(pause));
    memset(opa, 0, sizeof(opa));
    memset(xlu, 0, sizeof(xlu));
    memset(overlay, 0, sizeof(overlay));
    gfx.polyOpa.p = opa;
    gfx.polyXlu.p = xlu;
    gfx.overlay.p = overlay;
    gSaveContext.save.playerForm = form;
    gSaveContext.save.saveInfo.inventory.upgrades = 1 << gUpgradeShifts[UPG_QUIVER];
    if (item == ITEM_FAIRY_SLINGSHOT) {
        seeds = count;
    } else {
        AMMO(item) = count;
    }
    pause.alpha = 255;
    policy = override;
    expectedItem = item;
    digitCount = shownNumber = 0;
    before = gSaveContext;
    KaleidoScope_DrawAmmoCount(&pause, &gfx, item, 3);
    assert(!memcmp(&before, &gSaveContext, sizeof(before)));
    assert(digitCount == (count >= 10 ? 2 : 1));
    assert(shownNumber == count);
    for (int i = 0; i < digitCount; i++) {
        if (digitColor[i] != want) {
            fprintf(stderr, "FAIL %s form=%d policy=%d ammo=%d digit=%d: RGBA %08x, want %08x\n",
                    label, form, override, count, i, digitColor[i], want);
            return 1;
        }
    }
    return 0;
}

int main(void) {
    int failed = 0;
    const int counts[] = { 0, 12, 30 };
    const unsigned allowed[] = { 0x828282ff, 0xffffffff, 0x78ff00ff };
    for (int form = PLAYER_FORM_FIERCE_DEITY; form <= PLAYER_FORM_HUMAN; form++) {
        for (int i = 0; i < 3; i++) {
            failed += check("native form rule", form, -1, ITEM_BOW, counts[i],
                            form == PLAYER_FORM_HUMAN ? allowed[i] : 0x646464ff);
            failed += check("unrestricted bow", form, 0, ITEM_BOW, counts[i], allowed[i]);
        }
    }
    failed += check("hook can impose restriction", PLAYER_FORM_HUMAN, 1, ITEM_BOW, 12, 0x646464ff);
    failed += check("slingshot native restriction", PLAYER_FORM_DEKU, -1, ITEM_FAIRY_SLINGSHOT, 12, 0x646464ff);
    failed += check("slingshot unrestricted", PLAYER_FORM_DEKU, 0, ITEM_FAIRY_SLINGSHOT, 12, 0xffffffff);
    failed += check("slingshot zero", PLAYER_FORM_HUMAN, -1, ITEM_FAIRY_SLINGSHOT, 0, 0x828282ff);
    failed += check("slingshot capacity", PLAYER_FORM_HUMAN, -1, ITEM_FAIRY_SLINGSHOT, 30, 0x78ff00ff);
    if (failed) {
        fprintf(stderr, "%d pause ammo color cases failed\n", failed);
        return 1;
    }
    puts("PASS MM pause ammo: native form restrictions, effective item hook, zero/full colors, slingshot bounds, unchanged save");
    return 0;
}
