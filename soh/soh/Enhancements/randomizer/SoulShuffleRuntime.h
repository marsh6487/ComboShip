#pragma once
#ifdef __cplusplus
extern "C" {
#endif
struct PlayState;
struct Actor;
int Randomizer_SoulCanSpawnNonEnemy(struct PlayState* play, int actorId, unsigned int params);
int Randomizer_SoulCanDamageActor(struct PlayState* play, struct Actor* actor);
int Randomizer_SoulShouldUpdateActor(struct PlayState* play, struct Actor* actor);
#ifdef __cplusplus
}
#endif
