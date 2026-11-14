#ifndef COMBO_ITEM_GET_LIGHT_SHAFT_BRIDGE_H
#define COMBO_ITEM_GET_LIGHT_SHAFT_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

struct PlayState;
struct Player;
void SoHItemGetLightShaft_MarkNative(struct PlayState* play, struct Player* player, float itemY);
void MMItemGetLightShaft_MarkNative(struct PlayState* play, struct Player* player, float itemY);

#ifdef __cplusplus
}
#endif
#endif
