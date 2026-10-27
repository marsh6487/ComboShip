#pragma once

#ifdef COMBO_BUILD
struct Actor;
struct PlayState;
#ifdef __cplusplus
extern "C" {
#endif
void ComboTempleSages_DrawZeldaAura(struct Actor* actor, struct PlayState* play);
#ifdef __cplusplus
}
#endif
#endif
