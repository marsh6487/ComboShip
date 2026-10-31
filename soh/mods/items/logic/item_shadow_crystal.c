/** Shadow Crystal input bridge for the Wolf Link transformation. */
#include "mods/extended_inventory.h"
#include "mods/ext_buttons/ext_buttons.h"
#include "mods/transformation_masks/transformation_masks.h"
#include "mods/transformation_masks/wolf_link_form.h"

extern void MmForm_HandleMaskUse(PlayState* play, Player* player, s32 item);

void ShadowCrystal_TickInput(PlayState* play, Player* player) {
    const u16 pressed = play->state.input[0].press.button;
    if (pressed & ExtButton_GetInputMask(EXT_ITEM_SHADOW_CRYSTAL)) {
        MmForm_HandleMaskUse(play, player, EXT_ITEM_SHADOW_CRYSTAL);
    }
}
