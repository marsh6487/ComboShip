#ifndef DIN_FIRE_SWORD_H
#define DIN_FIRE_SWORD_H

#include <stdint.h>
#include "../../combo/render/sword_element_aura_profiles.h"

struct PlayState;
struct Player;
struct Actor;
struct ColliderInfo;

#ifdef __cplusplus
extern "C" {
#endif

uint32_t DinFireSword_DamageFlags(struct PlayState* play, struct Player* player, uint32_t original);
uint32_t DinFireSword_SetDamageFlags(struct PlayState* play, struct Player* player, int quad, uint32_t original);
void DinFireSword_RefreshDamage(struct PlayState* play, struct Player* player);
// Recover only this feature's exact player melee touchers. Other combined
// attacks keep their normal collision, damage-table and drop behavior.
uint32_t DinFireSword_OriginalDamageFlags(struct PlayState* play, const struct ColliderInfo* hitInfo);
uint8_t DinFireSword_DamageEntry(struct PlayState* play, struct Actor* target, const struct ColliderInfo* hitInfo,
                                 uint32_t receiverFlags, uint8_t vanillaEntry);
void DinFireSword_Reset(void);
void DinFireSword_Update(struct PlayState* play, struct Player* player);
void DinFireSword_BeginPlayerDraw(struct PlayState* play, struct Player* player);
// Gameplay left-hand matrix must be current. Ordinary sword hand only;
// other weapon owners and ceremonies keep their own draw path.
void DinFireSword_Draw(struct PlayState* play, struct Player* player);
// Final limb compositors register the actual blade and transfer that identity
// when copying a deferred hand list. NULL owner is the current local draw only.
void DinFireSword_SetHandModel(struct Player* player, void* handDL, int model);
void DinFireSword_CopyHandModel(struct Player* player, void* sourceDL, void* destinationDL);
void DinFireSword_DrawHand(struct PlayState* play, struct Player* player, void* finalHandDL);
// Submit the captured hand effect after body meshes and accessories, before XLU.
void DinFireSword_DrawAfterPlayer(struct PlayState* play, struct Player* player);
// Fit progressive swords to the existing fire layers without changing upgrade state.
void* DinFireSword_HandDL(struct PlayState* play, struct Player* player, void* closedHand, uint8_t r, uint8_t g,
                          uint8_t b);
// Call only alongside the custom pedestal sword, in its current model matrix.
void DinFireSword_DrawPedestal(struct PlayState* play);

#ifdef __cplusplus
}
#endif
#endif
