/** MM-native custom-item voice dispatch, shared by rods, Roc items and other tools. */
#ifndef ITEM_VOICE_H
#define ITEM_VOICE_H
#include "z64.h"
#ifdef __cplusplus
extern "C" {
#endif
// The action is an unbanked 0x6800..0x681F voice. MM applies the native form's
// offset; the legacy child argument is retained for shared item call sites.
void ItemVoice_Play(Player* player, u16 baseVoice, u16 legacyChildVoice);
void ItemVoice_PlayId(Player* player, u16 baseVoice);
void Player_AnimSfx_PlayVoice(Player* player, u16 baseVoice);
#ifdef __cplusplus
}
#endif
#endif
