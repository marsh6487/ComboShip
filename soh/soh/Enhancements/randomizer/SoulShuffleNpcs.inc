// SOUL_NPC(actor, scene, rolePredicate, requiredSoul)
// EnNiwLady does not interpret params; scene distinguishes her native day/night roles.
SOUL_NPC(ACTOR_EN_NIW_LADY, SCENE_KAKARIKO_VILLAGE, true, RG_SOUL_NPC_ANJU)
SOUL_NPC(ACTOR_EN_NIW_LADY, SCENE_IMPAS_HOUSE, true, RG_SOUL_NPC_ANJU)

// Adult Grog owns the Cojiro trade. EnHs overwrites params from age in Init;
// his separate child counterpart EnHs2 has no alternate parameter role.
SOUL_NPC(ACTOR_EN_HS, SCENE_LOST_WOODS, true, RG_SOUL_NPC_GROG)
SOUL_NPC(ACTOR_EN_HS2, SCENE_KAKARIKO_VILLAGE, true, RG_SOUL_NPC_GROG)
// EnHy's low SEVEN bits select the native person; upper bits select paths.
// Type 0 is Dog Lady; type 5 is the bottle buyer (catalogued as Banker).
// Keep unknown scenes/roles, cutscene viewers and other citizens unaffected.
SOUL_NPC(ACTOR_EN_HY, SCENE_DOG_LADY_HOUSE, (params & 0x7F) == 0, RG_SOUL_NPC_DOG_LADY)
SOUL_NPC(ACTOR_EN_HY, SCENE_MARKET_DAY, (params & 0x7F) == 0, RG_SOUL_NPC_DOG_LADY)
SOUL_NPC(ACTOR_EN_HY, SCENE_KAKARIKO_CENTER_GUEST_HOUSE, (params & 0x7F) == 0, RG_SOUL_NPC_DOG_LADY)
SOUL_NPC(ACTOR_EN_HY, SCENE_MARKET_DAY, (params & 0x7F) == 5, RG_SOUL_NPC_BANKER)
SOUL_NPC(ACTOR_EN_HY, SCENE_KAKARIKO_VILLAGE, (params & 0x7F) == 5, RG_SOUL_NPC_BANKER)

// Native purchase/trade hosts have no parameter-selected alternate roles.
SOUL_NPC(ACTOR_EN_DS, SCENE_POTION_SHOP_GRANNY, true, RG_SOUL_NPC_OLD_HAG)
SOUL_NPC(ACTOR_EN_MK, SCENE_LAKESIDE_LABORATORY, true, RG_SOUL_NPC_SCIENTIST)
SOUL_NPC(ACTOR_EN_MS, SCENE_ZORAS_RIVER, true, RG_SOUL_NPC_BEAN_SALESMAN)
SOUL_NPC(ACTOR_EN_JS, SCENE_HAUNTED_WASTELAND, true, RG_SOUL_NPC_CARPET_MAN)
SOUL_NPC(ACTOR_EN_FU, SCENE_WINDMILL_AND_DAMPES_GRAVE, true, RG_SOUL_NPC_GURU_GURU)
