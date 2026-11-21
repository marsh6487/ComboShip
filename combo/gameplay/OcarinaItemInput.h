#pragma once

#include "z64.h"
#include "macros.h"

#define OCARINA_ITEM_DPAD (BTN_DUP | BTN_DDOWN | BTN_DLEFT | BTN_DRIGHT)

// Ocarina messages also cover lessons and recording without a player-held instrument.
static inline int OcarinaItemInput_Active(const PlayState* play) {
    if (play == NULL)
        return 0;
    if (play->msgCtx.msgMode >= MSGMODE_OCARINA_STARTING && play->msgCtx.msgMode < MSGMODE_TEXT_AWAIT_NEXT)
        return 1;
    const Player* player = GET_PLAYER(play);
    if (player == NULL)
        return 0;

#ifdef PLAYER_STATE2_USING_OCARINA
    // The native item request can wait for the instrument camera before the playing flag is set.
    return (player->stateFlags2 & PLAYER_STATE2_USING_OCARINA) || (player->stateFlags3 & PLAYER_STATE3_20) ||
           (player->itemAction == PLAYER_IA_OCARINA && player->unk_AA5 == PLAYER_UNKAA5_5) ||
           ((player->stateFlags3 & PLAYER_STATE3_START_CHANGING_HELD_ITEM) &&
            (player->itemAction == PLAYER_IA_OCARINA || player->heldItemAction == PLAYER_IA_OCARINA));
#else
    const int requestedOcarina =
        player->itemAction == PLAYER_IA_OCARINA_FAIRY || player->itemAction == PLAYER_IA_OCARINA_OF_TIME;
    const int heldOcarina =
        player->heldItemAction == PLAYER_IA_OCARINA_FAIRY || player->heldItemAction == PLAYER_IA_OCARINA_OF_TIME;
    // Player_UseItem sets unk_6AD to 4 for a queued cutscene item such as an ocarina.
    return (player->stateFlags2 & PLAYER_STATE2_OCARINA_PLAYING) ||
           (player->stateFlags3 & PLAYER_STATE3_FORCE_PULL_OCARINA) || (requestedOcarina && player->unk_6AD == 4) ||
           ((player->stateFlags1 & PLAYER_STATE1_START_CHANGING_HELD_ITEM) && (requestedOcarina || heldOcarina));
#endif
}

// Filter the item reader's local bits. Audio and message input still use the original pad.
static inline u16 OcarinaItemInput_FilterDpad(const PlayState* play, u16 buttons) {
    return (buttons & OCARINA_ITEM_DPAD) && OcarinaItemInput_Active(play) ? buttons & ~OCARINA_ITEM_DPAD : buttons;
}
