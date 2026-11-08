/**
 * item_voice.c - Form-aware player voice for custom items
 *
 * Unity-included from mods/items/logic/custom_items.c (helpers block), so it is
 * in scope for every item_*.c. See item_voice.h for the rationale.
 */

#include "item_voice.h"
#include "functions.h" // Player_PlaySfx
#include "variables.h"
void ItemVoice_Play(Player* p, u16 ootAdult, u16 ootChild) {
    (void)ootChild; // MM selects a native form bank, rather than OoT's age-specific IDs.
    if (!p)
        return;
    if (ootAdult >= 0x6820 && ootAdult < 0x6840)
        ootAdult -= 0x20;
    Player_AnimSfx_PlayVoice(p, ootAdult);
}

void ItemVoice_PlayId(Player* p, u16 ootVoiceId) {
    ItemVoice_Play(p, ootVoiceId, ootVoiceId);
}
