// SOUL_NPC(actor, scene, rolePredicate, requiredSoul)
// Native Grog uses 0xFE01; the unused params==1 alternate and other placements stay independent.
// Normalize the native s16 spawn argument before matching the role.
SOUL_NPC(ACTOR_EN_HS, SCENE_F01C, (params & 0xFFFF) == 0xFE01, RI_SOUL_NPC_GROG)
// EnAob01 is the unique race host; params have no role semantics in this actor.
SOUL_NPC(ACTOR_EN_AOB_01, SCENE_F01_B, true, RI_SOUL_NPC_DOG_LADY)
