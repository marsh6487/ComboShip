#ifndef ITEM_VISUALS_H
#define ITEM_VISUALS_H

#ifdef __cplusplus
extern "C" {
#endif

// Returns false for an unknown owner, unsupported draw, disabled option or missing model.
s32 GetItem_DrawDungeonItem(PlayState* play, s16 drawId, s32 owner);

#ifdef __cplusplus
}
#endif

#endif
