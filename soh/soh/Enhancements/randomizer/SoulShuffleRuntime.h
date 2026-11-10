#pragma once
#ifdef __cplusplus
extern "C" {
#endif
struct PlayState;
struct Actor;
void Randomizer_SoulResetRoomState(struct PlayState* play, int room);
void Randomizer_SoulNoteRemovedActor(struct PlayState* play, struct Actor* actor);
int Randomizer_SoulCanClearRoom(struct PlayState* play, int room);
void Randomizer_SoulUpdateActor(struct PlayState* play, struct Actor* actor);
int Randomizer_SoulCanAccessCheck(int check);
int Randomizer_SoulCanInteract(struct PlayState* play, int actorId, unsigned int params);
int Randomizer_SoulCanSpawnNonEnemy(struct PlayState* play, int actorId, unsigned int params);
int Randomizer_SoulCanDamageActor(struct PlayState* play, struct Actor* actor);
int Randomizer_SoulShouldUpdateActor(struct PlayState* play, struct Actor* actor);
#ifdef __cplusplus
}
#endif
