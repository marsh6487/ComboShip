#ifndef SSBB_SPAWN_H
#define SSBB_SPAWN_H

#include "z64.h"

#ifdef __cplusplus
extern "C" {
#endif

// The native MM human player keeps its controls and collision in Pikachu Mode.
s32 PikachuForm_IsSelected(void);
void PikachuForm_Update(PlayState* play, Player* player);
void PikachuForm_Destroy(void);
s32 PikachuForm_Draw(PlayState* play, Player* player);
s32 PikachuForm_DrawPause(PlayState* play);

struct SSBBAnim;
const struct SSBBAnim* PikachuForm_GetAnimation(s32 index);

#ifdef __cplusplus
}
#endif

#endif // SSBB_SPAWN_H
