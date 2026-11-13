/* Run Interface_Draw's complete B icon/label branch with native GBI commands.
 * Item-icon lookup/drawing and window dimensions are boundary doubles. The
 * production branch chooses the texture, emits its load, and preserves A's
 * pending transition segments without a running renderer.
 */
#include "global.h"
#include "mods/ext_buttons/ext_buttons.h"
#include "mods/transformation_masks/mm_mask_wear.h"
#include "mods/transformation_masks/transformation_masks.h"
#include "soh/Enhancements/cosmetics/cosmeticsTypes.h"
#include "soh/OTRGlobals.h"
#include <assert.h>

/* NATIVE_HUD_DEFINES */

static s16 Top_HUD_Margin, Left_HUD_Margin, Right_HUD_Margin;
int BlastTest_BIconCount;

u8 MasterCycle_IsRiding(void) { return 0; }
u8 TransformMasks_IsTransformed(void) { return 0; }
u16 ExtButton_GetItem(s32 button) { return gSaveContext.equips.buttonItems[button]; }
void* ExtInv_GetItemIcon(u16 item) { return NULL; }
void* PikaMode_ButtonIcon(s32 button, void* texture) { return texture; }
void Interface_DrawItemIconTexture(PlayState* play, void* texture, s16 button) {
    assert(button == 0);
    ++BlastTest_BIconCount;
}
void Interface_DrawAmmoCount(PlayState* play, s16 button, s16 alpha) {}
s32 Flags_GetSwitch(PlayState* play, s32 flag) { return 0; }
f32 OTRGetDimensionFromLeftEdge(f32 value) { return value; }
f32 OTRGetDimensionFromRightEdge(f32 value) { return value; }
s16 OTRGetRectDimensionFromRightEdge(f32 value) { return (s16)value; }

void BlastTest_DrawB(PlayState* play) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    Player* player = GET_PLAYER(play);
    GraphicsContext* __gfxCtx = play->state.gfxCtx;
    bool fullUi = true;
    s32 languageOffset = gSaveContext.language;
    if (languageOffset == LANGUAGE_JPN) languageOffset = LANGUAGE_ENG;
    BlastTest_BIconCount = 0;
    /* NATIVE_B_HUD */
}
