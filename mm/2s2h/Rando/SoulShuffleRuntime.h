#pragma once
#ifdef __cplusplus
extern "C" {
#endif
struct PlayState;
int MM_SoulCanInteract(struct PlayState* play, int actorId, unsigned int params);
int MM_SoulCanSpawnNonEnemy(struct PlayState* play, int actorId, unsigned int params);
#ifdef __cplusplus
}
#endif
