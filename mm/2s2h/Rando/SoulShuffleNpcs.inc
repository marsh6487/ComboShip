// SOUL_NPC(actor, scene, rolePredicate, requiredSoul)
// Native Grog uses 0xFE01; the unused params==1 alternate and other placements stay independent.
// Normalize the native s16 spawn argument before matching the role.
SOUL_NPC(ACTOR_EN_HS, SCENE_F01C, (params & 0xFFFF) == 0xFE01, RI_SOUL_NPC_GROG)
// EnAob01 is the unique race host; params have no role semantics in this actor.
SOUL_NPC(ACTOR_EN_AOB_01, SCENE_F01_B, true, RI_SOUL_NPC_DOG_LADY)

// Banker has no alternate parameter-selected role.
SOUL_NPC(ACTOR_EN_GINKO_MAN, SCENE_ICHIBA, true, RI_SOUL_NPC_BANKER)
// Normal day/night musician roles; the hidden params==2 cutscene variant stays safe.
SOUL_NPC(ACTOR_EN_GURUGURU, SCENE_ALLEY, (params & 0xFFFF) == 1, RI_SOUL_NPC_GURU_GURU)
SOUL_NPC(ACTOR_EN_GURUGURU, SCENE_YADOYA, (params & 0xFFFF) == 0, RI_SOUL_NPC_GURU_GURU)
// EnNb's native schedule explicitly selects these two scenes.
SOUL_NPC(ACTOR_EN_NB, SCENE_YADOYA, true, RI_SOUL_NPC_OLD_HAG)
SOUL_NPC(ACTOR_EN_NB, SCENE_OMOYA, true, RI_SOUL_NPC_OLD_HAG)
// Low byte selects teacher/scared roles; high byte is the native path index.
SOUL_NPC(ACTOR_EN_KENDO_JS, SCENE_DOUJOU, (params & 0xFF) <= 1, RI_SOUL_NPC_CARPET_MAN)

// This host owns each native gallery's targets; params only encode path index.
SOUL_NPC(ACTOR_EN_SYATEKI_MAN, SCENE_SYATEKI_MIZU, true, RI_SOUL_NPC_SHOOTING_GALLERY_OWNER)
SOUL_NPC(ACTOR_EN_SYATEKI_MAN, SCENE_SYATEKI_MORI, true, RI_SOUL_NPC_SHOOTING_GALLERY_OWNER)
