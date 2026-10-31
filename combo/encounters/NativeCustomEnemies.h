#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct PlayState;
struct Actor;
void CustomEnemies_RecordEntry(struct PlayState*, int16_t id, float x, float y, float z, int16_t params);
void CustomEnemies_ReplaceEntry(struct PlayState*, int16_t* id, int16_t* params, float x, float y, float z);
int CustomEnemies_Id(int kind);
int CustomEnemies_CanSpawn(struct PlayState*, int kind, float x, float y, float z);
int CustomEnemies_Kind(int id);
void CustomEnemies_Tick(void);
void CustomEnemies_Reset(void);
void CustomEnemies_ActorDestroyed(struct Actor*);
#ifdef __cplusplus
}
#endif
