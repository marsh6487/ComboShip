// Source-audited witnesses from native combat/pass routes. Never attach a global reward to Root.
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:82
ENEMY_DROP_WITNESS(ANUBIS, RR_SPIRIT_TEMPLE_1F_ANUBIS, SCENE_SPIRIT_TEMPLE, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanHitSwitch() || logic->HasItem(RG_POWER_BRACELET) || logic->CanKillEnemy(RE_ANUBIS);})) && logic->CanKillEnemy(RE_ANUBIS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:705
ENEMY_DROP_WITNESS(ANUBIS, RR_SPIRIT_TEMPLE_MQ_ANUBIS_BRIDGE_CHEST, SCENE_SPIRIT_TEMPLE, RCQUEST_MQ, (logic->CanKillEnemy(RE_ANUBIS) && logic->CanKillEnemy(RE_KEESE, ED_BOOMERANG)) && logic->CanKillEnemy(RE_ANUBIS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:691
ENEMY_DROP_WITNESS(ANUBIS, RR_SPIRIT_TEMPLE_MQ_ANUBIS_BRIDGE_GRAVE, SCENE_SPIRIT_TEMPLE, RCQUEST_MQ, ((logic->CanKillEnemy(RE_ANUBIS) && logic->CanKillEnemy(RE_KEESE)) && (logic->HasItem(RG_POWER_BRACELET) || logic->CanUse(RG_HOVER_BOOTS) || logic->CanUse(RG_HOOKSHOT) || (logic->IsAdult && logic->BunnyHood())) || (logic->CanKillEnemy(RE_ANUBIS, ED_BOOMERANG) && logic->CanKillEnemy(RE_KEESE, ED_FAR))) && logic->CanKillEnemy(RE_ANUBIS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/dodongos_cavern.cpp:691
ENEMY_DROP_WITNESS(ARMOS, RR_DODONGOS_CAVERN_MQ_BACK_BEHIND_FIRE, SCENE_DODONGOS_CAVERN, RCQUEST_MQ, ((logic->CanPassEnemy(RE_ARMOS) && (logic->IsAdult || logic->Get(LOGIC_DC_MQ_BEHIND_FIRE_SWITCH))) || logic->CanUse(RG_HOVER_BOOTS)) && logic->CanKillEnemy(RE_ARMOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/dodongos_cavern.cpp:664
ENEMY_DROP_WITNESS(ARMOS, RR_DODONGOS_CAVERN_MQ_BEHIND_MOUTH, SCENE_DODONGOS_CAVERN, RCQUEST_MQ, (logic->CanGetEnemyDrop(RE_GOLD_SKULLTULA, ED_BOOMERANG) || (logic->IsAdult && logic->HasItem(RG_POWER_BRACELET) && (logic->CanPassEnemy(RE_ARMOS) || logic->CanUse(RG_HOVER_BOOTS) || (ctx->GetTrickOption(RT_UNINTUITIVE_JUMPS)) || logic->BunnyHood()))) && logic->CanKillEnemy(RE_ARMOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/ganons_castle.cpp:552
ENEMY_DROP_WITNESS(ARMOS, RR_GANONS_CASTLE_MQ_FOREST_TRIAL_BEAMOS_ROOM_MIDDLE, SCENE_INSIDE_GANONS_CASTLE, RCQUEST_MQ, ((logic->CanAvoidEnemy(RE_BEAMOS) || logic->CanKillEnemy(RE_ARMOS)) && ((logic->Get(LOGIC_FOREST_TRIAL_MQ_SPAWN_BEAMOS_CHEST) && logic->CanUse(RG_LONGSHOT)) || (((logic->IsAdult && logic->CanUse(RG_HOVER_BOOTS)) || (logic->CanMiddairGroundJump()))))) && logic->CanKillEnemy(RE_ARMOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/ganons_castle.cpp:473
ENEMY_DROP_WITNESS(ARMOS, RR_GANONS_CASTLE_MQ_LOBBY, SCENE_INSIDE_GANONS_CASTLE, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_IRON_KNUCKLE) && logic->CanKillEnemy(RE_ARMOS);})) && logic->CanKillEnemy(RE_ARMOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:47
ENEMY_DROP_WITNESS(ARMOS, RR_SPIRIT_TEMPLE_CHILD_SIDE_HUB, SCENE_SPIRIT_TEMPLE, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanKillEnemy(RE_ARMOS);})) && logic->CanKillEnemy(RE_ARMOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/gerudo_training_ground.cpp:215
ENEMY_DROP_WITNESS(BEAMOS, RR_GERUDO_TRAINING_GROUND_DINALFOS, SCENE_GERUDO_TRAINING_GROUND, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanKillEnemy(RE_BEAMOS) && logic->CanKillEnemy(RE_DINOLFOS, ED_CLOSE, true, 2, true);})) && logic->CanKillEnemy(RE_BEAMOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:434
ENEMY_DROP_WITNESS(BEAMOS, RR_SPIRIT_TEMPLE_BEAMOS_PITS, SCENE_SPIRIT_TEMPLE, RCQUEST_VANILLA, (logic->CanKillEnemy(RE_BEAMOS)) && logic->CanKillEnemy(RE_BEAMOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:1030
ENEMY_DROP_WITNESS(BEAMOS, RR_SPIRIT_TEMPLE_MQ_BEAMOS_PITS, SCENE_SPIRIT_TEMPLE, RCQUEST_MQ, (logic->CanKillEnemy(RE_BEAMOS) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_BEAMOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:743
ENEMY_DROP_WITNESS(BEAMOS, RR_SPIRIT_TEMPLE_MQ_SUN_ON_FLOOR, SCENE_SPIRIT_TEMPLE, RCQUEST_MQ, (SpiritShared(RR_SPIRIT_TEMPLE_MQ_SUN_ON_FLOOR, []{return logic->CanKillEnemy(RE_BEAMOS) && logic->HasItem(RG_OPEN_CHEST);})) && logic->CanKillEnemy(RE_BEAMOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:298
ENEMY_DROP_WITNESS(BEAMOS, RR_SPIRIT_TEMPLE_STATUE_ROOM, SCENE_SPIRIT_TEMPLE, RCQUEST_VANILLA, (ctx->GetOption(RSK_SHUFFLE_DUNGEON_ENTRANCES).Is(RO_DUNGEON_ENTRANCE_SHUFFLE_OFF) && logic->HasStrength(2) && logic->IsAdult && ((logic->CanKillEnemy(RE_BEAMOS) && logic->SmallKeys(SCENE_SPIRIT_TEMPLE, (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOOKSHOT)) && logic->HasItem(RG_POWER_BRACELET) ? 2 : 3)) || ((logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOOKSHOT)) && logic->HasItem(RG_POWER_BRACELET) && logic->CanKillEnemy(RE_IRON_KNUCKLE) && logic->SmallKeys(SCENE_SPIRIT_TEMPLE, 4)))) && logic->CanKillEnemy(RE_BEAMOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:236
ENEMY_DROP_WITNESS(BUBBLE, RR_FOREST_TEMPLE_BELOW_BOSS_KEY_CHEST, SCENE_FOREST_TEMPLE, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanKillEnemy(RE_BLUE_BUBBLE);})) && logic->CanKillEnemy(RE_BLUE_BUBBLE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:208
ENEMY_DROP_WITNESS(BUBBLE, RR_FOREST_TEMPLE_MAP_ROOM, SCENE_FOREST_TEMPLE, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanKillEnemy(RE_BLUE_BUBBLE);})) && logic->CanKillEnemy(RE_BLUE_BUBBLE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/ganons_castle.cpp:471
ENEMY_DROP_WITNESS(BUBBLE, RR_GANONS_CASTLE_MQ_LOBBY, SCENE_INSIDE_GANONS_CASTLE, RCQUEST_MQ, (logic->CanPassEnemy(RE_GREEN_BUBBLE) || AnyAgeTime([]{return logic->CanKillEnemy(RE_IRON_KNUCKLE) && logic->CanKillEnemy(RE_ARMOS);})) && logic->CanKillEnemy(RE_GREEN_BUBBLE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/ganons_castle.cpp:858
ENEMY_DROP_WITNESS(BUBBLE, RR_GANONS_CASTLE_MQ_SPIRIT_TRIAL_BEFORE_SWITCH, SCENE_INSIDE_GANONS_CASTLE, RCQUEST_MQ, (logic->CanPassEnemy(RE_GREEN_BUBBLE) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_GREEN_BUBBLE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:70
ENEMY_DROP_WITNESS(BUBBLE, RR_SPIRIT_TEMPLE_SWITCH_BRIDGE_NORTH, SCENE_SPIRIT_TEMPLE, RCQUEST_VANILLA, (logic->CanUse(RG_HOVER_BOOTS) || (logic->Get(LOGIC_SPIRIT_CHILD_SWITCH_BRIDGE) && logic->CanPassEnemy(RE_GREEN_BUBBLE, ED_CLOSE, false)) || (logic->IsAdult && logic->BunnyHood())) && logic->CanKillEnemy(RE_GREEN_BUBBLE, ED_CLOSE, false))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:58
ENEMY_DROP_WITNESS(BUBBLE, RR_SPIRIT_TEMPLE_SWITCH_BRIDGE_SOUTH, SCENE_SPIRIT_TEMPLE, RCQUEST_VANILLA, ((logic->Get(LOGIC_SPIRIT_CHILD_SWITCH_BRIDGE) && logic->CanPassEnemy(RE_GREEN_BUBBLE, ED_CLOSE, false)) || logic->CanUse(RG_HOVER_BOOTS) || logic->CanUse(RG_LONGSHOT) || (logic->IsAdult && logic->BunnyHood())) && logic->CanKillEnemy(RE_GREEN_BUBBLE, ED_CLOSE, false))
// soh/soh/Enhancements/randomizer/location_access/dungeons/water_temple.cpp:577
ENEMY_DROP_WITNESS(DARK_LINK, RR_WATER_TEMPLE_DARK_LINK_ROOM, SCENE_WATER_TEMPLE, RCQUEST_VANILLA, (logic->CanKillEnemy(RE_DARK_LINK)) && logic->CanKillEnemy(RE_DARK_LINK))
// soh/soh/Enhancements/randomizer/location_access/dungeons/water_temple.cpp:1176
ENEMY_DROP_WITNESS(DARK_LINK, RR_WATER_TEMPLE_MQ_DARK_LINK_ROOM, SCENE_WATER_TEMPLE, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_DARK_LINK);})) && logic->CanKillEnemy(RE_DARK_LINK))
// soh/soh/Enhancements/randomizer/location_access/dungeons/bottom_of_the_well.cpp:185
ENEMY_DROP_WITNESS(DEAD_HAND, RR_BOTW_DEAD_HAND_ROOM, SCENE_BOTTOM_OF_THE_WELL, RCQUEST_VANILLA, (logic->CanKillEnemy(RE_DEAD_HAND)) && logic->CanKillEnemy(RE_DEAD_HAND))
// soh/soh/Enhancements/randomizer/location_access/dungeons/bottom_of_the_well.cpp:471
ENEMY_DROP_WITNESS(DEAD_HAND, RR_BOTW_MQ_DEAD_HAND_ROOM, SCENE_BOTTOM_OF_THE_WELL, RCQUEST_MQ, (logic->CanKillEnemy(RE_DEAD_HAND)) && logic->CanKillEnemy(RE_DEAD_HAND))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:85
ENEMY_DROP_WITNESS(DEAD_HAND, RR_SHADOW_TEMPLE_DEAD_HAND, SCENE_SHADOW_TEMPLE, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanKillEnemy(RE_DEAD_HAND);})) && logic->CanKillEnemy(RE_DEAD_HAND))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:513
ENEMY_DROP_WITNESS(DEAD_HAND, RR_SHADOW_TEMPLE_MQ_WHISPERING_WALLS_DEAD_HAND, SCENE_SHADOW_TEMPLE, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_DEAD_HAND);})) && logic->CanKillEnemy(RE_DEAD_HAND))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:918
ENEMY_DROP_WITNESS(DEAD_HAND, RR_SHADOW_TEMPLE_MQ_X_CROSS, SCENE_SHADOW_TEMPLE, RCQUEST_MQ, ((logic->CanUse(RG_LENS_OF_TRUTH) || ctx->GetTrickOption(RT_LENS_SHADOW_MQ_DEADHAND)) && logic->CanKillEnemy(RE_DEAD_HAND) && (logic->CanDetonateBombFlowers() || logic->HasStrength(1)) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_DEAD_HAND))
// soh/soh/Enhancements/randomizer/location_access/dungeons/deku_tree.cpp:225
ENEMY_DROP_WITNESS(DEKU_BABA, RR_DEKU_TREE_MQ_1F, SCENE_DEKU_TREE, RCQUEST_MQ, (logic->CanKillEnemy(RE_WITHERED_DEKU_BABA)) && logic->CanKillEnemy(RE_WITHERED_DEKU_BABA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/deku_tree.cpp:402
ENEMY_DROP_WITNESS(DEKU_BABA, RR_DEKU_TREE_MQ_BASEMENT_WATER_ROOM_BACK, SCENE_DEKU_TREE, RCQUEST_MQ, (logic->CanKillEnemy(RE_WITHERED_DEKU_BABA)) && logic->CanKillEnemy(RE_WITHERED_DEKU_BABA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/deku_tree.cpp:295
ENEMY_DROP_WITNESS(DEKU_BABA, RR_DEKU_TREE_MQ_SLINGSHOT_ROOM, SCENE_DEKU_TREE, RCQUEST_MQ, (logic->CanKillEnemy(RE_DEKU_BABA)) && logic->CanKillEnemy(RE_DEKU_BABA))
// soh/soh/Enhancements/randomizer/location_access/overworld/kokiri_forest.cpp:106
ENEMY_DROP_WITNESS(DEKU_BABA, RR_KOKIRI_FOREST, SCENE_KOKIRI_FOREST, RCQUEST_BOTH, (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOVER_BOOTS) || ((logic->IsChild || logic->CanKillEnemy(RE_DEKU_BABA) || Combo_SOH_CrossBossLogicalClear(COMBO_BOSS_PHANTOM_GANON, logic->Get(LOGIC_FOREST_TEMPLE_CLEAR))) && logic->CanClimbLadder())) && logic->CanKillEnemy(RE_DEKU_BABA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/deku_tree.cpp:371
ENEMY_DROP_WITNESS(DEKU_SCRUB, RR_DEKU_TREE_MQ_BASEMENT_SOUTHEAST_ROOM, SCENE_DEKU_TREE, RCQUEST_MQ, (logic->CanKillEnemy(RE_MAD_SCRUB)) && logic->CanKillEnemy(RE_MAD_SCRUB))
// soh/soh/Enhancements/randomizer/location_access/dungeons/deku_tree.cpp:423
ENEMY_DROP_WITNESS(DEKU_SCRUB, RR_DEKU_TREE_MQ_BASEMENT_SOUTHWEST_ROOM, SCENE_DEKU_TREE, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_MAD_SCRUB) && logic->CanKillEnemy(RE_KEESE);})) && logic->CanKillEnemy(RE_MAD_SCRUB))
// soh/soh/Enhancements/randomizer/location_access/dungeons/dodongos_cavern.cpp:655
ENEMY_DROP_WITNESS(DEKU_SCRUB, RR_DODONGOS_CAVERN_MQ_MAD_SCRUB_ROOM, SCENE_DODONGOS_CAVERN, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_FIRE_KEESE) && logic->CanKillEnemy(RE_MAD_SCRUB);})) && logic->CanKillEnemy(RE_MAD_SCRUB))
// soh/soh/Enhancements/randomizer/location_access/dungeons/dodongos_cavern.cpp:449
ENEMY_DROP_WITNESS(DODONGO, RR_DODONGOS_CAVERN_MQ_DODONGO_ROOM, SCENE_DODONGOS_CAVERN, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_DODONGO) || logic->HasStrength(1);})) && logic->CanKillEnemy(RE_DODONGO))
// soh/soh/Enhancements/randomizer/location_access/dungeons/water_temple.cpp:1524
ENEMY_DROP_WITNESS(DODONGO, RR_WATER_TEMPLE_MQ_DODONGO_ROOM, SCENE_WATER_TEMPLE, RCQUEST_MQ, ((logic->CanUse(RG_HOOKSHOT) || logic->CanUse(RG_HOVER_BOOTS)) && AnyAgeTime([]{return logic->CanKillEnemy(RE_DODONGO, ED_CLOSE, true, 5);})) && logic->CanKillEnemy(RE_DODONGO, ED_CLOSE, true, 5))
// soh/soh/Enhancements/randomizer/location_access/dungeons/fire_temple.cpp:436
ENEMY_DROP_WITNESS(FLARE_DANCER, RR_FIRE_TEMPLE_3F_FLARE_DANCER, SCENE_FIRE_TEMPLE, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanKillEnemy(RE_FLARE_DANCER);})) && logic->CanKillEnemy(RE_FLARE_DANCER))
// soh/soh/Enhancements/randomizer/location_access/dungeons/fire_temple.cpp:100
ENEMY_DROP_WITNESS(FLARE_DANCER, RR_FIRE_TEMPLE_LOOP_FLARE_DANCER, SCENE_FIRE_TEMPLE, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanKillEnemy(RE_FLARE_DANCER);})) && logic->CanKillEnemy(RE_FLARE_DANCER))
// soh/soh/Enhancements/randomizer/location_access/dungeons/fire_temple.cpp:1091
ENEMY_DROP_WITNESS(FLARE_DANCER, RR_FIRE_TEMPLE_MQ_3F_FLARE_DANCER, SCENE_FIRE_TEMPLE, RCQUEST_MQ, (logic->CanKillEnemy(RE_FLARE_DANCER)) && logic->CanKillEnemy(RE_FLARE_DANCER))
// soh/soh/Enhancements/randomizer/location_access/dungeons/fire_temple.cpp:576
ENEMY_DROP_WITNESS(FLARE_DANCER, RR_FIRE_TEMPLE_MQ_LOOP_FLARE_DANCER, SCENE_FIRE_TEMPLE, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_FLARE_DANCER);})) && logic->CanKillEnemy(RE_FLARE_DANCER))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:570
ENEMY_DROP_WITNESS(FLOORMASTER, RR_FOREST_TEMPLE_MQ_BLOCK_PUZZLE_TOP, SCENE_FOREST_TEMPLE, RCQUEST_MQ, (logic->SmallKeys(SCENE_FOREST_TEMPLE, 2) && AnyAgeTime([]{return logic->CanKillEnemy(RE_FLOORMASTER);})) && logic->CanKillEnemy(RE_FLOORMASTER))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:583
ENEMY_DROP_WITNESS(FLOORMASTER, RR_FOREST_TEMPLE_MQ_FLOORMASTER_ROOM, SCENE_FOREST_TEMPLE, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_FLOORMASTER);})) && logic->CanKillEnemy(RE_FLOORMASTER))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:406
ENEMY_DROP_WITNESS(FLOORMASTER, RR_SHADOW_TEMPLE_X_CROSS, SCENE_SHADOW_TEMPLE, RCQUEST_VANILLA, (AnyAgeTime([]{return (ctx->GetTrickOption(RT_LENS_SHADOW) || logic->CanUse(RG_LENS_OF_TRUTH)) && logic->CanKillEnemy(RE_FLOORMASTER);})) && logic->CanKillEnemy(RE_FLOORMASTER))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:1055
ENEMY_DROP_WITNESS(FLOORMASTER, RR_SPIRIT_TEMPLE_MQ_FLOORMASTER_STAIRS, SCENE_SPIRIT_TEMPLE, RCQUEST_MQ, ((ctx->GetTrickOption(RT_LENS_SPIRIT_MQ) || logic->CanUse(RG_LENS_OF_TRUTH)) && AnyAgeTime([]{return logic->CanKillEnemy(RE_FLOORMASTER);})) && logic->CanKillEnemy(RE_FLOORMASTER))
// soh/soh/Enhancements/randomizer/location_access/dungeons/gerudo_training_ground.cpp:334
ENEMY_DROP_WITNESS(FREEZARD, RR_GERUDO_TRAINING_GROUND_MQ_ROOM_BEHIND_BLOCK, SCENE_GERUDO_TRAINING_GROUND, RCQUEST_MQ, (logic->CanKillEnemy(RE_FREEZARD) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_FREEZARD))
// soh/soh/Enhancements/randomizer/location_access/dungeons/ice_cavern.cpp:39
ENEMY_DROP_WITNESS(FREEZARD, RR_ICE_CAVERN_BEGINNING, SCENE_ICE_CAVERN, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanKillEnemy(RE_FREEZARD, ED_CLOSE, true, 4);})) && logic->CanKillEnemy(RE_FREEZARD, ED_CLOSE, true, 4))
// soh/soh/Enhancements/randomizer/location_access/dungeons/ice_cavern.cpp:291
ENEMY_DROP_WITNESS(FREEZARD, RR_ICE_CAVERN_MQ_HUB, SCENE_ICE_CAVERN, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_WHITE_WOLFOS) && logic->CanKillEnemy(RE_FREEZARD);})) && logic->CanKillEnemy(RE_FREEZARD))
// soh/soh/Enhancements/randomizer/location_access/dungeons/fire_temple.cpp:567
ENEMY_DROP_WITNESS(IRON_KNUCKLE, RR_FIRE_TEMPLE_MQ_LOOP_5_TILE_ROOM, SCENE_FIRE_TEMPLE, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_IRON_KNUCKLE);})) && logic->CanKillEnemy(RE_IRON_KNUCKLE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/ganons_castle.cpp:473
ENEMY_DROP_WITNESS(IRON_KNUCKLE, RR_GANONS_CASTLE_MQ_LOBBY, SCENE_INSIDE_GANONS_CASTLE, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_IRON_KNUCKLE) && logic->CanKillEnemy(RE_ARMOS);})) && logic->CanKillEnemy(RE_IRON_KNUCKLE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/ganons_castle.cpp:991
ENEMY_DROP_WITNESS(IRON_KNUCKLE, RR_GANONS_TOWER_FLOOR_3, SCENE_GANONS_TOWER, RCQUEST_BOTH, (AnyAgeTime([]{return logic->CanKillEnemy(RE_IRON_KNUCKLE, ED_CLOSE, true, 2);})) && logic->CanKillEnemy(RE_IRON_KNUCKLE, ED_CLOSE, true, 2))
// soh/soh/Enhancements/randomizer/location_access/dungeons/gerudo_training_ground.cpp:277
ENEMY_DROP_WITNESS(IRON_KNUCKLE, RR_GERUDO_TRAINING_GROUND_MQ_SAND_ROOM, SCENE_GERUDO_TRAINING_GROUND, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_IRON_KNUCKLE);})) && logic->CanKillEnemy(RE_IRON_KNUCKLE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/gerudo_training_ground.cpp:383
ENEMY_DROP_WITNESS(IRON_KNUCKLE, RR_GERUDO_TRAINING_GROUND_MQ_TORCH_SLUG_ROOM, SCENE_GERUDO_TRAINING_GROUND, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_IRON_KNUCKLE);})) && logic->CanKillEnemy(RE_IRON_KNUCKLE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:468
ENEMY_DROP_WITNESS(IRON_KNUCKLE, RR_SPIRIT_TEMPLE_ADULT_THRONE, SCENE_SPIRIT_TEMPLE, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanKillEnemy(RE_IRON_KNUCKLE);})) && logic->CanKillEnemy(RE_IRON_KNUCKLE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:355
ENEMY_DROP_WITNESS(IRON_KNUCKLE, RR_SPIRIT_TEMPLE_CHILD_THRONE, SCENE_SPIRIT_TEMPLE, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanKillEnemy(RE_IRON_KNUCKLE);})) && logic->CanKillEnemy(RE_IRON_KNUCKLE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:1061
ENEMY_DROP_WITNESS(IRON_KNUCKLE, RR_SPIRIT_TEMPLE_MQ_ADULT_THRONE, SCENE_SPIRIT_TEMPLE, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_IRON_KNUCKLE);})) && logic->CanKillEnemy(RE_IRON_KNUCKLE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:864
ENEMY_DROP_WITNESS(IRON_KNUCKLE, RR_SPIRIT_TEMPLE_MQ_CHILD_THRONE, SCENE_SPIRIT_TEMPLE, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_IRON_KNUCKLE);})) && logic->CanKillEnemy(RE_IRON_KNUCKLE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:298
ENEMY_DROP_WITNESS(IRON_KNUCKLE, RR_SPIRIT_TEMPLE_STATUE_ROOM, SCENE_SPIRIT_TEMPLE, RCQUEST_VANILLA, (ctx->GetOption(RSK_SHUFFLE_DUNGEON_ENTRANCES).Is(RO_DUNGEON_ENTRANCE_SHUFFLE_OFF) && logic->HasStrength(2) && logic->IsAdult && ((logic->CanKillEnemy(RE_BEAMOS) && logic->SmallKeys(SCENE_SPIRIT_TEMPLE, (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOOKSHOT)) && logic->HasItem(RG_POWER_BRACELET) ? 2 : 3)) || ((logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOOKSHOT)) && logic->HasItem(RG_POWER_BRACELET) && logic->CanKillEnemy(RE_IRON_KNUCKLE) && logic->SmallKeys(SCENE_SPIRIT_TEMPLE, 4)))) && logic->CanKillEnemy(RE_IRON_KNUCKLE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:247
ENEMY_DROP_WITNESS(IRON_KNUCKLE, RR_SPIRIT_TEMPLE_STATUE_ROOM_CHILD, SCENE_SPIRIT_TEMPLE, RCQUEST_VANILLA, (ctx->GetOption(RSK_SHUFFLE_DUNGEON_ENTRANCES).Is(RO_DUNGEON_ENTRANCE_SHUFFLE_OFF) && logic->HasItem(RG_POWER_BRACELET) && logic->CanUse(RG_CRAWL) && logic->SmallKeys(SCENE_SPIRIT_TEMPLE, 4) && logic->CanKillEnemy(RE_IRON_KNUCKLE)) && logic->CanKillEnemy(RE_IRON_KNUCKLE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/deku_tree.cpp:423
ENEMY_DROP_WITNESS(KEESE, RR_DEKU_TREE_MQ_BASEMENT_SOUTHWEST_ROOM, SCENE_DEKU_TREE, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_MAD_SCRUB) && logic->CanKillEnemy(RE_KEESE);})) && logic->CanKillEnemy(RE_KEESE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/dodongos_cavern.cpp:655
ENEMY_DROP_WITNESS(KEESE, RR_DODONGOS_CAVERN_MQ_MAD_SCRUB_ROOM, SCENE_DODONGOS_CAVERN, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_FIRE_KEESE) && logic->CanKillEnemy(RE_MAD_SCRUB);})) && logic->CanKillEnemy(RE_FIRE_KEESE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/fire_temple.cpp:83
ENEMY_DROP_WITNESS(KEESE, RR_FIRE_TEMPLE_LOOP_HEXAGON_ROOM, SCENE_FIRE_TEMPLE, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanKillEnemy(RE_TORCH_SLUG) && logic->CanKillEnemy(RE_FIRE_KEESE);})) && logic->CanKillEnemy(RE_FIRE_KEESE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/ganons_castle.cpp:420
ENEMY_DROP_WITNESS(KEESE, RR_GANONS_CASTLE_LIGHT_TRIAL_CHESTS_ROOM, SCENE_INSIDE_GANONS_CASTLE, RCQUEST_VANILLA, ((ctx->GetTrickOption(RT_LENS_GANON) || logic->CanUse(RG_LENS_OF_TRUTH)) && (logic->CanKillEnemy(RE_BIG_SKULLTULA) && logic->CanKillEnemy(RE_KEESE, ED_CLOSE, true, 3)) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_KEESE, ED_CLOSE, true, 3))
// soh/soh/Enhancements/randomizer/location_access/dungeons/jabujabus_belly.cpp:510
ENEMY_DROP_WITNESS(KEESE, RR_JABU_JABUS_BELLY_MQ_INVISIBLE_KEESE_ROOM, SCENE_JABU_JABU, RCQUEST_MQ, ((logic->HasFireProjectile()) && logic->CanGetEnemyDrop(RE_GOLD_SKULLTULA, ED_LONGSHOT) || ((logic->CanGetEnemyDrop(RE_GOLD_SKULLTULA, ED_BOOMERANG) || (logic->IsAdult && logic->CanGroundJumpslash())) && ((logic->CanUse(RG_HOVER_BOOTS) || AnyAgeTime([]{return (ctx->GetTrickOption(RT_LENS_JABU_MQ) || logic->CanUse(RG_LENS_OF_TRUTH)) && logic->CanKillEnemy(RE_STINGER, ED_BOOMERANG, false, 2, false, true) && (logic->CanKillEnemy(RE_KEESE, ED_LONGSHOT, false) || (ctx->GetTrickOption(RT_LENS_JABU_MQ) && logic->CanUse(RG_HOOKSHOT) && logic->CanUse(RG_IRON_BOOTS)));})) && ((logic->IsChild && logic->HasItem(RG_BRONZE_SCALE)) || (logic->IsAdult && logic->CanUse(RG_IRON_BOOTS)) || (logic->BunnyHood() && logic->CanJumpslash()))))) && logic->CanKillEnemy(RE_KEESE, ED_LONGSHOT, false))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:206
ENEMY_DROP_WITNESS(KEESE, RR_SHADOW_TEMPLE_INVISIBLE_SPINNING_BLADES, SCENE_SHADOW_TEMPLE, RCQUEST_VANILLA, (logic->CanKillEnemy(RE_LIKE_LIKE) && logic->CanKillEnemy(RE_KEESE) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_KEESE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:284
ENEMY_DROP_WITNESS(KEESE, RR_SHADOW_TEMPLE_SKULL_JAR, SCENE_SHADOW_TEMPLE, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanKillEnemy(RE_KEESE);})) && logic->CanKillEnemy(RE_KEESE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:77
ENEMY_DROP_WITNESS(KEESE, RR_SHADOW_TEMPLE_WHISPERING_WALLS_SIDE_ROOM, SCENE_SHADOW_TEMPLE, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanKillEnemy(RE_REDEAD) && logic->CanKillEnemy(RE_KEESE);})) && logic->CanKillEnemy(RE_KEESE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:705
ENEMY_DROP_WITNESS(KEESE, RR_SPIRIT_TEMPLE_MQ_ANUBIS_BRIDGE_CHEST, SCENE_SPIRIT_TEMPLE, RCQUEST_MQ, (logic->CanKillEnemy(RE_ANUBIS) && logic->CanKillEnemy(RE_KEESE, ED_BOOMERANG)) && logic->CanKillEnemy(RE_KEESE, ED_BOOMERANG))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:691
ENEMY_DROP_WITNESS(KEESE, RR_SPIRIT_TEMPLE_MQ_ANUBIS_BRIDGE_GRAVE, SCENE_SPIRIT_TEMPLE, RCQUEST_MQ, ((logic->CanKillEnemy(RE_ANUBIS) && logic->CanKillEnemy(RE_KEESE)) && (logic->HasItem(RG_POWER_BRACELET) || logic->CanUse(RG_HOVER_BOOTS) || logic->CanUse(RG_HOOKSHOT) || (logic->IsAdult && logic->BunnyHood())) || (logic->CanKillEnemy(RE_ANUBIS, ED_BOOMERANG) && logic->CanKillEnemy(RE_KEESE, ED_FAR))) && logic->CanKillEnemy(RE_KEESE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:1094
ENEMY_DROP_WITNESS(KEESE, RR_SPIRIT_TEMPLE_MQ_BIG_WALL_BASE, SCENE_SPIRIT_TEMPLE, RCQUEST_MQ, (logic->CanUse(RG_LONGSHOT) || (logic->HasItem(RG_CLIMB) && (logic->CanKillEnemy(RE_KEESE) || logic->CanUse(RG_SKULL_MASK)))) && logic->CanKillEnemy(RE_KEESE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:1106
ENEMY_DROP_WITNESS(KEESE, RR_SPIRIT_TEMPLE_MQ_BIG_WALL_UPPER, SCENE_SPIRIT_TEMPLE, RCQUEST_MQ, ((logic->CanKillEnemy(RE_KEESE) || logic->CanUse(RG_SKULL_MASK)) && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT))) && logic->CanKillEnemy(RE_KEESE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:982
ENEMY_DROP_WITNESS(LEEVER, RR_SPIRIT_TEMPLE_MQ_SAND_PIT, SCENE_SPIRIT_TEMPLE, RCQUEST_MQ, (logic->CanKillEnemy(RE_PURPLE_LEEVER) && (logic->CanUse(RG_HOOKSHOT) || logic->CanRecoilHover(RECOIL_HAMMER)) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_PURPLE_LEEVER))
// soh/soh/Enhancements/randomizer/location_access/dungeons/fire_temple.cpp:537
ENEMY_DROP_WITNESS(LIKE_LIKE, RR_FIRE_TEMPLE_MQ_LOOP_CAGE_FOYER_SIDE, SCENE_FIRE_TEMPLE, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_LIKE_LIKE);})) && logic->CanKillEnemy(RE_LIKE_LIKE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/gerudo_training_ground.cpp:114
ENEMY_DROP_WITNESS(LIKE_LIKE, RR_GERUDO_TRAINING_GROUND_LIKE_LIKE_ROOM, SCENE_GERUDO_TRAINING_GROUND, RCQUEST_VANILLA, (logic->CanKillEnemy(RE_LIKE_LIKE) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_LIKE_LIKE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:206
ENEMY_DROP_WITNESS(LIKE_LIKE, RR_SHADOW_TEMPLE_INVISIBLE_SPINNING_BLADES, SCENE_SHADOW_TEMPLE, RCQUEST_VANILLA, (logic->CanKillEnemy(RE_LIKE_LIKE) && logic->CanKillEnemy(RE_KEESE) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_LIKE_LIKE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/dodongos_cavern.cpp:93
ENEMY_DROP_WITNESS(LIZALFOS_DINOLFOS, RR_DODONGOS_CAVERN_LOWER_LIZALFOS, SCENE_DODONGOS_CAVERN, RCQUEST_VANILLA, (logic->CanKillEnemy(RE_LIZALFOS, ED_CLOSE, true, 2)) && logic->CanKillEnemy(RE_LIZALFOS, ED_CLOSE, true, 2))
// soh/soh/Enhancements/randomizer/location_access/dungeons/dodongos_cavern.cpp:622
ENEMY_DROP_WITNESS(LIZALFOS_DINOLFOS, RR_DODONGOS_CAVERN_MQ_LOWER_LIZALFOS, SCENE_DODONGOS_CAVERN, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_LIZALFOS);})) && logic->CanKillEnemy(RE_LIZALFOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/dodongos_cavern.cpp:539
ENEMY_DROP_WITNESS(LIZALFOS_DINOLFOS, RR_DODONGOS_CAVERN_MQ_UPPER_LIZALFOS, SCENE_DODONGOS_CAVERN, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_LIZALFOS);})) && logic->CanKillEnemy(RE_LIZALFOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/dodongos_cavern.cpp:232
ENEMY_DROP_WITNESS(LIZALFOS_DINOLFOS, RR_DODONGOS_CAVERN_UPPER_LIZALFOS, SCENE_DODONGOS_CAVERN, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanKillEnemy(RE_LIZALFOS, ED_CLOSE, true, 2);})) && logic->CanKillEnemy(RE_LIZALFOS, ED_CLOSE, true, 2))
// soh/soh/Enhancements/randomizer/location_access/dungeons/ganons_castle.cpp:896
ENEMY_DROP_WITNESS(LIZALFOS_DINOLFOS, RR_GANONS_CASTLE_MQ_LIGHT_TRIAL_DINOLFOS_ROOM, SCENE_INSIDE_GANONS_CASTLE, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_DINOLFOS) && logic->CanKillEnemy(RE_TORCH_SLUG);})) && logic->CanKillEnemy(RE_DINOLFOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/ganons_castle.cpp:964
ENEMY_DROP_WITNESS(LIZALFOS_DINOLFOS, RR_GANONS_TOWER_FLOOR_1, SCENE_GANONS_TOWER, RCQUEST_BOTH, (AnyAgeTime([]{return logic->CanKillEnemy(RE_DINOLFOS, ED_CLOSE, true, 2);})) && logic->CanKillEnemy(RE_DINOLFOS, ED_CLOSE, true, 2))
// soh/soh/Enhancements/randomizer/location_access/dungeons/gerudo_training_ground.cpp:215
ENEMY_DROP_WITNESS(LIZALFOS_DINOLFOS, RR_GERUDO_TRAINING_GROUND_DINALFOS, SCENE_GERUDO_TRAINING_GROUND, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanKillEnemy(RE_BEAMOS) && logic->CanKillEnemy(RE_DINOLFOS, ED_CLOSE, true, 2, true);})) && logic->CanKillEnemy(RE_DINOLFOS, ED_CLOSE, true, 2, true))
// soh/soh/Enhancements/randomizer/location_access/dungeons/jabujabus_belly.cpp:386
ENEMY_DROP_WITNESS(LIZALFOS_DINOLFOS, RR_JABU_JABUS_BELLY_MQ_WATER_SWITCH_ROOM, SCENE_JABU_JABU, RCQUEST_MQ, (logic->CanKillEnemy(RE_LIZALFOS)) && logic->CanKillEnemy(RE_LIZALFOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/water_temple.cpp:882
ENEMY_DROP_WITNESS(LIZALFOS_DINOLFOS, RR_WATER_TEMPLE_MQ_TOWER_TARGET_ROOM, SCENE_WATER_TEMPLE, RCQUEST_MQ, (logic->CanKillEnemy(RE_LIZALFOS) && logic->CanKillEnemy(RE_SPIKE)) && logic->CanKillEnemy(RE_LIZALFOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/jabujabus_belly.cpp:218
ENEMY_DROP_WITNESS(OCTOROK, RR_JABU_JABUS_BELLY_BIGOCTO, SCENE_JABU_JABU, RCQUEST_VANILLA, (logic->Get(LOGIC_JABU_RUTO_IN_1F) && AnyAgeTime([]{return logic->CanKillEnemy(RE_BIG_OCTO);})) && logic->CanKillEnemy(RE_BIG_OCTO))
// soh/soh/Enhancements/randomizer/location_access/dungeons/jabujabus_belly.cpp:213
ENEMY_DROP_WITNESS(OCTOROK, RR_JABU_JABUS_BELLY_BIGOCTO_LEDGE, SCENE_JABU_JABU, RCQUEST_VANILLA, (logic->Get(LOGIC_JABU_RUTO_IN_1F) && AnyAgeTime([]{return logic->CanKillEnemy(RE_BIG_OCTO);})) && logic->CanKillEnemy(RE_BIG_OCTO))
// soh/soh/Enhancements/randomizer/location_access/dungeons/jabujabus_belly.cpp:551
ENEMY_DROP_WITNESS(OCTOROK, RR_JABU_JABUS_BELLY_MQ_ABOVE_BIGOCTO, SCENE_JABU_JABU, RCQUEST_MQ, (logic->TakeDamage() && AnyAgeTime([]{return logic->CanKillEnemy(RE_BIG_OCTO);})) && logic->CanKillEnemy(RE_BIG_OCTO))
// soh/soh/Enhancements/randomizer/location_access/dungeons/jabujabus_belly.cpp:539
ENEMY_DROP_WITNESS(OCTOROK, RR_JABU_JABUS_BELLY_MQ_BIGOCTO, SCENE_JABU_JABU, RCQUEST_MQ, (AnyAgeTime([]{return logic->HasItem(RG_SPEAK_ZORA) && logic->HasItem(RG_POWER_BRACELET) && logic->CanKillEnemy(RE_BIG_OCTO);})) && logic->CanKillEnemy(RE_BIG_OCTO))
// soh/soh/Enhancements/randomizer/location_access/dungeons/jabujabus_belly.cpp:174
ENEMY_DROP_WITNESS(PARASITE, RR_JABU_JABUS_BELLY_FORK_NORTH, SCENE_JABU_JABU, RCQUEST_VANILLA, (logic->CanKillEnemy(RE_TENTACLE, ED_BOOMERANG)) && logic->CanKillEnemy(RE_TENTACLE, ED_BOOMERANG))
// soh/soh/Enhancements/randomizer/location_access/dungeons/jabujabus_belly.cpp:190
ENEMY_DROP_WITNESS(PARASITE, RR_JABU_JABUS_BELLY_FORK_NORTH_EAST, SCENE_JABU_JABU, RCQUEST_VANILLA, (logic->CanKillEnemy(RE_TENTACLE, ED_BOOMERANG)) && logic->CanKillEnemy(RE_TENTACLE, ED_BOOMERANG))
// soh/soh/Enhancements/randomizer/location_access/dungeons/jabujabus_belly.cpp:140
ENEMY_DROP_WITNESS(PARASITE, RR_JABU_JABUS_BELLY_FORK_WEST, SCENE_JABU_JABU, RCQUEST_VANILLA, (logic->CanKillEnemy(RE_TENTACLE, ED_BOOMERANG)) && logic->CanKillEnemy(RE_TENTACLE, ED_BOOMERANG))
// soh/soh/Enhancements/randomizer/location_access/dungeons/jabujabus_belly.cpp:496
ENEMY_DROP_WITNESS(PARASITE, RR_JABU_JABUS_BELLY_MQ_FORK_EAST, SCENE_JABU_JABU, RCQUEST_MQ, (logic->CanKillEnemy(RE_TENTACLE, ED_BOOMERANG)) && logic->CanKillEnemy(RE_TENTACLE, ED_BOOMERANG))
// soh/soh/Enhancements/randomizer/location_access/dungeons/jabujabus_belly.cpp:434
ENEMY_DROP_WITNESS(PARASITE, RR_JABU_JABUS_BELLY_MQ_FORK_WEST, SCENE_JABU_JABU, RCQUEST_MQ, (logic->CanKillEnemy(RE_TENTACLE, ED_BOOMERANG)) && logic->CanKillEnemy(RE_TENTACLE, ED_BOOMERANG))
// soh/soh/Enhancements/randomizer/location_access/dungeons/dodongos_cavern.cpp:701
ENEMY_DROP_WITNESS(POE, RR_DODONGOS_CAVERN_MQ_BACK_POE_ROOM, SCENE_DODONGOS_CAVERN, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_POE);})) && logic->CanKillEnemy(RE_POE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:47
ENEMY_DROP_WITNESS(POE, RR_FOREST_TEMPLE_LOBBY, SCENE_FOREST_TEMPLE, RCQUEST_VANILLA, (logic->Get(LOGIC_FOREST_JOELLE) && logic->Get(LOGIC_FOREST_BETH) && logic->Get(LOGIC_FOREST_AMY) && logic->CanKillEnemy(RE_MEG)) && logic->CanKillEnemy(RE_MEG))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:461
ENEMY_DROP_WITNESS(POE, RR_FOREST_TEMPLE_MQ_LOBBY, SCENE_FOREST_TEMPLE, RCQUEST_MQ, (logic->Get(LOGIC_FOREST_JOELLE) && logic->Get(LOGIC_FOREST_BETH) && logic->Get(LOGIC_FOREST_AMY) && logic->CanKillEnemy(RE_MEG)) && logic->CanKillEnemy(RE_MEG))
// soh/soh/Enhancements/randomizer/location_access/dungeons/bottom_of_the_well.cpp:490
ENEMY_DROP_WITNESS(REDEAD_GIBDO, RR_BOTW_MQ_B3_PLATFORM, SCENE_BOTTOM_OF_THE_WELL, RCQUEST_MQ, (logic->CanPassEnemy(RE_REDEAD) && logic->CanOpenLargeChest()) && logic->CanKillEnemy(RE_REDEAD))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:614
ENEMY_DROP_WITNESS(REDEAD_GIBDO, RR_FOREST_TEMPLE_MQ_REDEAD_ROOM, SCENE_FOREST_TEMPLE, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_REDEAD);})) && logic->CanKillEnemy(RE_REDEAD))
// soh/soh/Enhancements/randomizer/location_access/overworld/kakariko.cpp:272
ENEMY_DROP_WITNESS(REDEAD_GIBDO, RR_KAK_REDEAD_GROTTO, SCENE_GROTTOS, RCQUEST_BOTH, (logic->CanKillEnemy(RE_REDEAD, ED_CLOSE, true, 2) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_REDEAD, ED_CLOSE, true, 2))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:104
ENEMY_DROP_WITNESS(REDEAD_GIBDO, RR_SHADOW_TEMPLE_COMPASS_ROOM, SCENE_SHADOW_TEMPLE, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanKillEnemy(RE_GIBDO);})) && logic->CanKillEnemy(RE_GIBDO))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:234
ENEMY_DROP_WITNESS(REDEAD_GIBDO, RR_SHADOW_TEMPLE_FLOOR_SPIKES_ROOM, SCENE_SHADOW_TEMPLE, RCQUEST_VANILLA, (logic->CanKillEnemy(RE_REDEAD) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_REDEAD))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:533
ENEMY_DROP_WITNESS(REDEAD_GIBDO, RR_SHADOW_TEMPLE_MQ_B2_GIBDO_ROOM, SCENE_SHADOW_TEMPLE, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_GIBDO);})) && logic->CanKillEnemy(RE_GIBDO))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:828
ENEMY_DROP_WITNESS(REDEAD_GIBDO, RR_SHADOW_TEMPLE_MQ_B4_GIBDO_ROOM, SCENE_SHADOW_TEMPLE, RCQUEST_MQ, (logic->CanKillEnemy(RE_GIBDO) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_GIBDO))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:701
ENEMY_DROP_WITNESS(REDEAD_GIBDO, RR_SHADOW_TEMPLE_MQ_FLOOR_SPIKES_ROOM, SCENE_SHADOW_TEMPLE, RCQUEST_MQ, (logic->CanKillEnemy(RE_REDEAD) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_REDEAD))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:505
ENEMY_DROP_WITNESS(REDEAD_GIBDO, RR_SHADOW_TEMPLE_MQ_WHISPERING_WALLS_SIDE_ROOM, SCENE_SHADOW_TEMPLE, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_REDEAD);})) && logic->CanKillEnemy(RE_REDEAD))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:808
ENEMY_DROP_WITNESS(REDEAD_GIBDO, RR_SHADOW_TEMPLE_MQ_WIND_HINT_ROOM, SCENE_SHADOW_TEMPLE, RCQUEST_MQ, ((ctx->GetTrickOption(RT_LENS_SHADOW_MQ) || logic->CanUse(RG_LENS_OF_TRUTH)) && logic->CanPassEnemy(RE_REDEAD) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_REDEAD))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:322
ENEMY_DROP_WITNESS(REDEAD_GIBDO, RR_SHADOW_TEMPLE_ROOM_TO_BOAT, SCENE_SHADOW_TEMPLE, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanKillEnemy(RE_GIBDO, ED_CLOSE, true, 2);})) && logic->CanKillEnemy(RE_GIBDO, ED_CLOSE, true, 2))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:77
ENEMY_DROP_WITNESS(REDEAD_GIBDO, RR_SHADOW_TEMPLE_WHISPERING_WALLS_SIDE_ROOM, SCENE_SHADOW_TEMPLE, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanKillEnemy(RE_REDEAD) && logic->CanKillEnemy(RE_KEESE);})) && logic->CanKillEnemy(RE_REDEAD))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:311
ENEMY_DROP_WITNESS(REDEAD_GIBDO, RR_SHADOW_TEMPLE_WIND_TUNNEL_HINT_ROOM, SCENE_SHADOW_TEMPLE, RCQUEST_VANILLA, (logic->CanKillEnemy(RE_REDEAD)) && logic->CanKillEnemy(RE_REDEAD))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:640
ENEMY_DROP_WITNESS(REDEAD_GIBDO, RR_SPIRIT_TEMPLE_MQ_GIBDO_GRAVES, SCENE_SPIRIT_TEMPLE, RCQUEST_MQ, (logic->HasItem(RG_POWER_BRACELET) && ((logic->CanUse(RG_BOMBCHU_5) && logic->CanHitEyeTargets()) || logic->CanUse(RG_HOVER_BOOTS) || (logic->IsAdult && logic->BunnyHood())) && logic->CanKillEnemy(RE_GIBDO, ED_CLOSE, true, 3)) && logic->CanKillEnemy(RE_GIBDO, ED_CLOSE, true, 3))
// soh/soh/Enhancements/randomizer/location_access/dungeons/jabujabus_belly.cpp:162
ENEMY_DROP_WITNESS(SHABOM, RR_JABU_JABUS_BELLY_FORK_NORTH_WEST, SCENE_JABU_JABU, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanKillEnemy(RE_SHABOM, ED_CLOSE, false, 9);})) && logic->CanKillEnemy(RE_SHABOM, ED_CLOSE, false, 9))
// soh/soh/Enhancements/randomizer/location_access/dungeons/water_temple.cpp:196
ENEMY_DROP_WITNESS(SHELL_BLADE, RR_WATER_TEMPLE_TORCH_ROOM, SCENE_WATER_TEMPLE, RCQUEST_VANILLA, (AnyAgeTime([]{return (logic->WaterLevel(WL_LOW) && logic->CanKillEnemy(RE_SHELL_BLADE, ED_CLOSE, true, 3) && logic->HasItem(RG_OPEN_CHEST)) || logic->CanOpenUnderwaterChest();})) && logic->CanKillEnemy(RE_SHELL_BLADE, ED_CLOSE, true, 3))
// soh/soh/Enhancements/randomizer/location_access/dungeons/bottom_of_the_well.cpp:24
ENEMY_DROP_WITNESS(SKULLTULA, RR_BOTW_CORRIDOR, SCENE_BOTTOM_OF_THE_WELL, RCQUEST_VANILLA, (logic->CanPassEnemy(RE_BIG_SKULLTULA)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/bottom_of_the_well.cpp:64
ENEMY_DROP_WITNESS(SKULLTULA, RR_BOTW_MIDDLE, SCENE_BOTTOM_OF_THE_WELL, RCQUEST_VANILLA, ((logic->CanPassEnemy(RE_BIG_SKULLTULA) || logic->TakeDamage()) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/bottom_of_the_well.cpp:334
ENEMY_DROP_WITNESS(SKULLTULA, RR_BOTW_MQ_PERIMETER, SCENE_BOTTOM_OF_THE_WELL, RCQUEST_MQ, (AnyAgeTime([]{return logic->BlastOrSmash();}) && logic->CanPassEnemy(RE_BIG_SKULLTULA)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/bottom_of_the_well.cpp:406
ENEMY_DROP_WITNESS(SKULLTULA, RR_BOTW_MQ_PIT_CAGE, SCENE_BOTTOM_OF_THE_WELL, RCQUEST_MQ, (logic->BlastOrSmash() && (logic->CanPassEnemy(RE_BIG_SKULLTULA) || ctx->GetTrickOption(RT_BOTW_PITS))) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/bottom_of_the_well.cpp:45
ENEMY_DROP_WITNESS(SKULLTULA, RR_BOTW_PERIMETER, SCENE_BOTTOM_OF_THE_WELL, RCQUEST_VANILLA, (logic->CanPassEnemy(RE_BIG_SKULLTULA)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/deku_tree.cpp:54
ENEMY_DROP_WITNESS(SKULLTULA, RR_DEKU_TREE_LOBBY_3F, SCENE_DEKU_TREE, RCQUEST_VANILLA, (logic->CanPassEnemy(RE_BIG_SKULLTULA)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/deku_tree.cpp:407
ENEMY_DROP_WITNESS(SKULLTULA, RR_DEKU_TREE_MQ_BASEMENT_WATER_ROOM_BACK, SCENE_DEKU_TREE, RCQUEST_MQ, (logic->CanUse(RG_SONG_OF_TIME) && logic->CanPassEnemy(RE_BIG_SKULLTULA) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/dodongos_cavern.cpp:494
ENEMY_DROP_WITNESS(SKULLTULA, RR_DODONGOS_CAVERN_MQ_BIG_BLOCK_ROOM, SCENE_DODONGOS_CAVERN, RCQUEST_MQ, (logic->CanPassEnemy(RE_BIG_SKULLTULA)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/dodongos_cavern.cpp:434
ENEMY_DROP_WITNESS(SKULLTULA, RR_DODONGOS_CAVERN_MQ_STAIRS_PAST_BIG_SKULLTULAS, SCENE_DODONGOS_CAVERN, RCQUEST_MQ, (logic->CanPassEnemy(RE_BIG_SKULLTULA) || logic->CanUse(RG_HOVER_BOOTS)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/dodongos_cavern.cpp:426
ENEMY_DROP_WITNESS(SKULLTULA, RR_DODONGOS_CAVERN_MQ_STAIRS_UPPER, SCENE_DODONGOS_CAVERN, RCQUEST_MQ, (logic->CanPassEnemy(RE_BIG_SKULLTULA) || logic->CanUse(RG_HOVER_BOOTS)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:414
ENEMY_DROP_WITNESS(SKULLTULA, RR_FOREST_TEMPLE_BLUE_DOORMAT_HALLWAY, SCENE_FOREST_TEMPLE, RCQUEST_VANILLA, (logic->CanPassEnemy(RE_BIG_SKULLTULA)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:409
ENEMY_DROP_WITNESS(SKULLTULA, RR_FOREST_TEMPLE_BLUE_DOORMAT_HALLWAY_DOORMAT, SCENE_FOREST_TEMPLE, RCQUEST_VANILLA, (logic->CanPassEnemy(RE_BIG_SKULLTULA)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:833
ENEMY_DROP_WITNESS(SKULLTULA, RR_FOREST_TEMPLE_MQ_BASEMENT, SCENE_FOREST_TEMPLE, RCQUEST_MQ, (logic->HasItem(RG_POWER_BRACELET) && (logic->CanPassEnemy(RE_BIG_SKULLTULA) || logic->TakeDamage())) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:845
ENEMY_DROP_WITNESS(SKULLTULA, RR_FOREST_TEMPLE_MQ_BASEMENT_POT_ROOM, SCENE_FOREST_TEMPLE, RCQUEST_MQ, (logic->CanPassEnemy(RE_BIG_SKULLTULA)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:634
ENEMY_DROP_WITNESS(SKULLTULA, RR_FOREST_TEMPLE_MQ_NW_COURTYARD, SCENE_FOREST_TEMPLE, RCQUEST_MQ, ((logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT)) && logic->Get(LOGIC_FOREST_MQ_BURNED_WEB) && (logic->CanKillEnemy(RE_BIG_SKULLTULA, ED_LONGSHOT) || logic->CanUse(RG_BOMBCHU_5))) && logic->CanKillEnemy(RE_BIG_SKULLTULA, ED_LONGSHOT))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:651
ENEMY_DROP_WITNESS(SKULLTULA, RR_FOREST_TEMPLE_MQ_NW_COURTYARD_UPPER_ALCOVE, SCENE_FOREST_TEMPLE, RCQUEST_MQ, (logic->CanPassEnemy(RE_BIG_SKULLTULA) || logic->TakeDamage()) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:447
ENEMY_DROP_WITNESS(SKULLTULA, RR_FOREST_TEMPLE_MQ_OVERGROWN_HALLWAY_LOWER, SCENE_FOREST_TEMPLE, RCQUEST_MQ, (logic->CanPassEnemy(RE_BIG_SKULLTULA)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:455
ENEMY_DROP_WITNESS(SKULLTULA, RR_FOREST_TEMPLE_MQ_OVERGROWN_HALLWAY_UPPER, SCENE_FOREST_TEMPLE, RCQUEST_MQ, (logic->CanPassEnemy(RE_BIG_SKULLTULA)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:437
ENEMY_DROP_WITNESS(SKULLTULA, RR_FOREST_TEMPLE_MQ_TREES, SCENE_FOREST_TEMPLE, RCQUEST_MQ, ((logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT)) && (logic->CanPassEnemy(RE_BIG_SKULLTULA, ED_SHORT_JUMPSLASH, false) || logic->CanUse(RG_HOVER_BOOTS) || logic->BunnyHood()) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_BIG_SKULLTULA, ED_SHORT_JUMPSLASH, false))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:36
ENEMY_DROP_WITNESS(SKULLTULA, RR_FOREST_TEMPLE_OVERGROWN_HALLWAY_LOWER, SCENE_FOREST_TEMPLE, RCQUEST_VANILLA, (logic->CanPassEnemy(RE_BIG_SKULLTULA)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:41
ENEMY_DROP_WITNESS(SKULLTULA, RR_FOREST_TEMPLE_OVERGROWN_HALLWAY_UPPER, SCENE_FOREST_TEMPLE, RCQUEST_VANILLA, (logic->CanPassEnemy(RE_BIG_SKULLTULA)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:250
ENEMY_DROP_WITNESS(SKULLTULA, RR_FOREST_TEMPLE_RED_DOORMAT_HALLWAY, SCENE_FOREST_TEMPLE, RCQUEST_VANILLA, (logic->CanPassEnemy(RE_BIG_SKULLTULA)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:255
ENEMY_DROP_WITNESS(SKULLTULA, RR_FOREST_TEMPLE_RED_DOORMAT_HALLWAY_DOORMAT, SCENE_FOREST_TEMPLE, RCQUEST_VANILLA, (logic->CanPassEnemy(RE_BIG_SKULLTULA)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/ganons_castle.cpp:420
ENEMY_DROP_WITNESS(SKULLTULA, RR_GANONS_CASTLE_LIGHT_TRIAL_CHESTS_ROOM, SCENE_INSIDE_GANONS_CASTLE, RCQUEST_VANILLA, ((ctx->GetTrickOption(RT_LENS_GANON) || logic->CanUse(RG_LENS_OF_TRUTH)) && (logic->CanKillEnemy(RE_BIG_SKULLTULA) && logic->CanKillEnemy(RE_KEESE, ED_CLOSE, true, 3)) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/overworld/kokiri_forest.cpp:189
ENEMY_DROP_WITNESS(SKULLTULA, RR_KF_OUTSIDE_DEKU_TREE, SCENE_KOKIRI_FOREST, RCQUEST_BOTH, ((logic->IsAdult && (logic->CanPassEnemy(RE_BIG_SKULLTULA) || Combo_SOH_CrossBossLogicalClear(COMBO_BOSS_GOHMA, logic->Get(LOGIC_DEKU_TREE_CLEAR)))) || ctx->GetOption(RSK_FOREST).Is(RO_CLOSED_FOREST_OFF) || logic->Get(LOGIC_SHOWED_MIDO_SWORD_AND_SHIELD)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/overworld/kokiri_forest.cpp:113
ENEMY_DROP_WITNESS(SKULLTULA, RR_KOKIRI_FOREST, SCENE_KOKIRI_FOREST, RCQUEST_BOTH, ((logic->IsAdult && (logic->CanPassEnemy(RE_BIG_SKULLTULA) || Combo_SOH_CrossBossLogicalClear(COMBO_BOSS_PHANTOM_GANON, logic->Get(LOGIC_FOREST_TEMPLE_CLEAR)))) || logic->Get(LOGIC_SHOWED_MIDO_SWORD_AND_SHIELD)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:127
ENEMY_DROP_WITNESS(SKULLTULA, RR_SHADOW_TEMPLE_B2_TO_B3_CORRIDOR_B2, SCENE_SHADOW_TEMPLE, RCQUEST_VANILLA, (logic->CanPassEnemy(RE_BIG_SKULLTULA)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:133
ENEMY_DROP_WITNESS(SKULLTULA, RR_SHADOW_TEMPLE_B2_TO_B3_CORRIDOR_B3, SCENE_SHADOW_TEMPLE, RCQUEST_VANILLA, (logic->CanPassEnemy(RE_BIG_SKULLTULA)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:542
ENEMY_DROP_WITNESS(SKULLTULA, RR_SHADOW_TEMPLE_MQ_B2_SPINNING_BLADE_ROOM, SCENE_SHADOW_TEMPLE, RCQUEST_MQ, (logic->CanPassEnemy(RE_BIG_SKULLTULA)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:558
ENEMY_DROP_WITNESS(SKULLTULA, RR_SHADOW_TEMPLE_MQ_SHORTCUT_PATH, SCENE_SHADOW_TEMPLE, RCQUEST_MQ, (logic->CanPassEnemy(RE_BIG_SKULLTULA)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:796
ENEMY_DROP_WITNESS(SKULLTULA, RR_SHADOW_TEMPLE_MQ_UPPER_WIND_TUNNEL, SCENE_SHADOW_TEMPLE, RCQUEST_MQ, (((logic->CanUse(RG_HOVER_BOOTS) || logic->BunnyHood()) && logic->CanPassEnemy(RE_BIG_SKULLTULA)) || logic->CanUse(RG_HOOKSHOT)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:290
ENEMY_DROP_WITNESS(SKULLTULA, RR_SHADOW_TEMPLE_UPPER_WIND_TUNNEL, SCENE_SHADOW_TEMPLE, RCQUEST_VANILLA, (logic->CanUse(RG_HOOKSHOT) || ((logic->CanUse(RG_HOVER_BOOTS) || logic->BunnyHood()) && logic->CanPassEnemy(RE_BIG_SKULLTULA))) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:1009
ENEMY_DROP_WITNESS(SKULLTULA, RR_SPIRIT_TEMPLE_MQ_SKULLTULA_ROOM, SCENE_SPIRIT_TEMPLE, RCQUEST_MQ, (logic->CanPassEnemy(RE_BIG_SKULLTULA) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:491
ENEMY_DROP_WITNESS(SKULLWALLTULA, RR_SPIRIT_TEMPLE_BIG_WALL_BASE, SCENE_SPIRIT_TEMPLE, RCQUEST_VANILLA, ((ctx->GetTrickOption(RT_SPIRIT_WALL) || (logic->CanAvoidEnemy(RE_BEAMOS, ED_CLOSE, true, 2) && logic->CanPassEnemy(RE_WALLTULA, ED_BOOMERANG))) && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT))) && logic->CanKillEnemy(RE_WALLTULA, ED_BOOMERANG))
// soh/soh/Enhancements/randomizer/location_access/dungeons/water_temple.cpp:183
ENEMY_DROP_WITNESS(SPIKE, RR_WATER_TEMPLE_4_SPIKES_ROOM, SCENE_WATER_TEMPLE, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanKillEnemy(RE_SPIKE, ED_CLOSE, true, 4);})) && logic->CanKillEnemy(RE_SPIKE, ED_CLOSE, true, 4))
// soh/soh/Enhancements/randomizer/location_access/dungeons/water_temple.cpp:882
ENEMY_DROP_WITNESS(SPIKE, RR_WATER_TEMPLE_MQ_TOWER_TARGET_ROOM, SCENE_WATER_TEMPLE, RCQUEST_MQ, (logic->CanKillEnemy(RE_LIZALFOS) && logic->CanKillEnemy(RE_SPIKE)) && logic->CanKillEnemy(RE_SPIKE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/fire_temple.cpp:547
ENEMY_DROP_WITNESS(STALFOS, RR_FIRE_TEMPLE_MQ_LOOP_HEXAGON_ROOM, SCENE_FIRE_TEMPLE, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 2);})) && logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 2))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:78
ENEMY_DROP_WITNESS(STALFOS, RR_FOREST_TEMPLE_LOWER_STALFOS, SCENE_FOREST_TEMPLE, RCQUEST_VANILLA, (logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 2)) && logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 2))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:761
ENEMY_DROP_WITNESS(STALFOS, RR_FOREST_TEMPLE_MQ_3_STALFOS_ROOM, SCENE_FOREST_TEMPLE, RCQUEST_MQ, (logic->Get(LOGIC_FOREST_CLEAR_BETWEEN_JOELLE_AND_BETH) && logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 3)) && logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 3))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:504
ENEMY_DROP_WITNESS(STALFOS, RR_FOREST_TEMPLE_MQ_RED_DOORMAT_HALLWAY, SCENE_FOREST_TEMPLE, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_STALFOS);})) && logic->CanKillEnemy(RE_STALFOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:333
ENEMY_DROP_WITNESS(STALFOS, RR_FOREST_TEMPLE_UPPER_STALFOS, SCENE_FOREST_TEMPLE, RCQUEST_VANILLA, (logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 2)) && logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 2))
// soh/soh/Enhancements/randomizer/location_access/dungeons/ganons_castle.cpp:528
ENEMY_DROP_WITNESS(STALFOS, RR_GANONS_CASTLE_MQ_FOREST_TRIAL_STALFOS_ROOM, SCENE_INSIDE_GANONS_CASTLE, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 2);})) && logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 2))
// soh/soh/Enhancements/randomizer/location_access/dungeons/ganons_castle.cpp:979
ENEMY_DROP_WITNESS(STALFOS, RR_GANONS_TOWER_FLOOR_2, SCENE_GANONS_TOWER, RCQUEST_BOTH, (AnyAgeTime([]{return logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 2);})) && logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 2))
// soh/soh/Enhancements/randomizer/location_access/dungeons/gerudo_training_ground.cpp:312
ENEMY_DROP_WITNESS(STALFOS, RR_GERUDO_TRAINING_GROUND_MQ_STALFOS_ROOM, SCENE_GERUDO_TRAINING_GROUND, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 2, true);})) && logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 2, true))
// soh/soh/Enhancements/randomizer/location_access/dungeons/gerudo_training_ground.cpp:39
ENEMY_DROP_WITNESS(STALFOS, RR_GERUDO_TRAINING_GROUND_SAND_ROOM, SCENE_GERUDO_TRAINING_GROUND, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 2, true);})) && logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 2, true))
// soh/soh/Enhancements/randomizer/location_access/dungeons/ice_cavern.cpp:380
ENEMY_DROP_WITNESS(STALFOS, RR_ICE_CAVERN_MQ_STALFOS_ROOM, SCENE_ICE_CAVERN, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_STALFOS);})) && logic->CanKillEnemy(RE_STALFOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:790
ENEMY_DROP_WITNESS(STALFOS, RR_SHADOW_TEMPLE_MQ_STALFOS_ROOM, SCENE_SHADOW_TEMPLE, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 2);})) && logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 2))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:945
ENEMY_DROP_WITNESS(STALFOS, RR_SPIRIT_TEMPLE_MQ_3_SUNS_ROOM_2F, SCENE_SPIRIT_TEMPLE, RCQUEST_MQ, (((logic->CanReflectLight()) && logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 2)) || logic->SunlightArrows()) && logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 2))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:668
ENEMY_DROP_WITNESS(STALFOS, RR_SPIRIT_TEMPLE_MQ_TURNTABLE, SCENE_SPIRIT_TEMPLE, RCQUEST_MQ, (logic->CanKillEnemy(RE_STALFOS)) && logic->CanKillEnemy(RE_STALFOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/water_temple.cpp:877
ENEMY_DROP_WITNESS(STALFOS, RR_WATER_TEMPLE_MQ_3_STALFOS_ROOM, SCENE_WATER_TEMPLE, RCQUEST_MQ, (logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 4)) && logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 4))
// soh/soh/Enhancements/randomizer/location_access/dungeons/water_temple.cpp:1487
ENEMY_DROP_WITNESS(STALFOS, RR_WATER_TEMPLE_MQ_FREESTANDING_ROOM, SCENE_WATER_TEMPLE, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_STALFOS);})) && logic->CanKillEnemy(RE_STALFOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/water_temple.cpp:1131
ENEMY_DROP_WITNESS(STALFOS, RR_WATER_TEMPLE_MQ_STALFOS_PIT_LOWER, SCENE_WATER_TEMPLE, RCQUEST_MQ, (((logic->IsAdult && logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 3, false, true)) || (logic->CanUse(RG_IRON_BOOTS) && logic->CanUse(RG_HOOKSHOT) && logic->CanKillEnemy(RE_STALFOS, ED_BOMB_THROW, true, 3, false, true)))) && logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 3, false, true))
// soh/soh/Enhancements/randomizer/location_access/dungeons/jabujabus_belly.cpp:199
ENEMY_DROP_WITNESS(STINGER, RR_JABU_JABUS_BELLY_FORK_EAST, SCENE_JABU_JABU, RCQUEST_VANILLA, ((logic->Get(LOGIC_JABU_RUTO_IN_1F) || logic->CanKillEnemy(RE_STINGER, ED_CLOSE, true, 4)) && logic->CanOpenLargeChest()) && logic->CanKillEnemy(RE_STINGER, ED_CLOSE, true, 4))
// soh/soh/Enhancements/randomizer/location_access/dungeons/jabujabus_belly.cpp:510
ENEMY_DROP_WITNESS(STINGER, RR_JABU_JABUS_BELLY_MQ_INVISIBLE_KEESE_ROOM, SCENE_JABU_JABU, RCQUEST_MQ, ((logic->HasFireProjectile()) && logic->CanGetEnemyDrop(RE_GOLD_SKULLTULA, ED_LONGSHOT) || ((logic->CanGetEnemyDrop(RE_GOLD_SKULLTULA, ED_BOOMERANG) || (logic->IsAdult && logic->CanGroundJumpslash())) && ((logic->CanUse(RG_HOVER_BOOTS) || AnyAgeTime([]{return (ctx->GetTrickOption(RT_LENS_JABU_MQ) || logic->CanUse(RG_LENS_OF_TRUTH)) && logic->CanKillEnemy(RE_STINGER, ED_BOOMERANG, false, 2, false, true) && (logic->CanKillEnemy(RE_KEESE, ED_LONGSHOT, false) || (ctx->GetTrickOption(RT_LENS_JABU_MQ) && logic->CanUse(RG_HOOKSHOT) && logic->CanUse(RG_IRON_BOOTS)));})) && ((logic->IsChild && logic->HasItem(RG_BRONZE_SCALE)) || (logic->IsAdult && logic->CanUse(RG_IRON_BOOTS)) || (logic->BunnyHood() && logic->CanJumpslash()))))) && logic->CanKillEnemy(RE_STINGER, ED_BOOMERANG, false, 2, false, true))
// soh/soh/Enhancements/randomizer/location_access/overworld/thieves_hideout.cpp:11
ENEMY_DROP_WITNESS(THIEVES, RR_TH_1_TORCH_CELL, SCENE_THIEVES_HIDEOUT, RCQUEST_BOTH, (logic->CanKillEnemy(RE_GERUDO_WARRIOR) && logic->HasItem(RG_SPEAK_HYLIAN)) && logic->CanKillEnemy(RE_GERUDO_WARRIOR))
// soh/soh/Enhancements/randomizer/location_access/overworld/thieves_hideout.cpp:57
ENEMY_DROP_WITNESS(THIEVES, RR_TH_DEAD_END_CELL, SCENE_THIEVES_HIDEOUT, RCQUEST_BOTH, (logic->CanKillEnemy(RE_GERUDO_WARRIOR) && logic->HasItem(RG_SPEAK_HYLIAN)) && logic->CanKillEnemy(RE_GERUDO_WARRIOR))
// soh/soh/Enhancements/randomizer/location_access/overworld/thieves_hideout.cpp:31
ENEMY_DROP_WITNESS(THIEVES, RR_TH_DOUBLE_CELL, SCENE_THIEVES_HIDEOUT, RCQUEST_BOTH, (logic->CanKillEnemy(RE_GERUDO_WARRIOR) && logic->HasItem(RG_SPEAK_HYLIAN)) && logic->CanKillEnemy(RE_GERUDO_WARRIOR))
// soh/soh/Enhancements/randomizer/location_access/overworld/thieves_hideout.cpp:73
ENEMY_DROP_WITNESS(THIEVES, RR_TH_STEEP_SLOPE_CELL, SCENE_THIEVES_HIDEOUT, RCQUEST_BOTH, (logic->CanKillEnemy(RE_GERUDO_WARRIOR) && logic->HasItem(RG_SPEAK_HYLIAN)) && logic->CanKillEnemy(RE_GERUDO_WARRIOR))
// soh/soh/Enhancements/randomizer/location_access/dungeons/fire_temple.cpp:83
ENEMY_DROP_WITNESS(TORCH_SLUG, RR_FIRE_TEMPLE_LOOP_HEXAGON_ROOM, SCENE_FIRE_TEMPLE, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanKillEnemy(RE_TORCH_SLUG) && logic->CanKillEnemy(RE_FIRE_KEESE);})) && logic->CanKillEnemy(RE_TORCH_SLUG))
// soh/soh/Enhancements/randomizer/location_access/dungeons/ganons_castle.cpp:896
ENEMY_DROP_WITNESS(TORCH_SLUG, RR_GANONS_CASTLE_MQ_LIGHT_TRIAL_DINOLFOS_ROOM, SCENE_INSIDE_GANONS_CASTLE, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_DINOLFOS) && logic->CanKillEnemy(RE_TORCH_SLUG);})) && logic->CanKillEnemy(RE_TORCH_SLUG))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:633
ENEMY_DROP_WITNESS(TORCH_SLUG, RR_SPIRIT_TEMPLE_MQ_CHILD_SIDE_HUB, SCENE_SPIRIT_TEMPLE, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_TORCH_SLUG);})) && logic->CanKillEnemy(RE_TORCH_SLUG))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:751
ENEMY_DROP_WITNESS(WOLFOS, RR_FOREST_TEMPLE_MQ_3_STALFOS_ROOM, SCENE_FOREST_TEMPLE, RCQUEST_MQ, (logic->CanKillEnemy(RE_WOLFOS)) && logic->CanKillEnemy(RE_WOLFOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:491
ENEMY_DROP_WITNESS(WOLFOS, RR_FOREST_TEMPLE_MQ_WOLFOS_ROOM, SCENE_FOREST_TEMPLE, RCQUEST_MQ, (logic->CanKillEnemy(RE_WOLFOS)) && logic->CanKillEnemy(RE_WOLFOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/ganons_castle.cpp:69
ENEMY_DROP_WITNESS(WOLFOS, RR_GANONS_CASTLE_FOREST_TRIAL_WOLFOS_ROOM, SCENE_INSIDE_GANONS_CASTLE, RCQUEST_VANILLA, (logic->CanKillEnemy(RE_WOLFOS) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_WOLFOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/gerudo_training_ground.cpp:89
ENEMY_DROP_WITNESS(WOLFOS, RR_GERUDO_TRAINING_GROUND_HEAVY_BLOCK_ROOM, SCENE_GERUDO_TRAINING_GROUND, RCQUEST_VANILLA, (logic->CanKillEnemy(RE_WOLFOS, ED_CLOSE, true, 4, true) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_WOLFOS, ED_CLOSE, true, 4, true))
// soh/soh/Enhancements/randomizer/location_access/dungeons/ice_cavern.cpp:228
ENEMY_DROP_WITNESS(WOLFOS, RR_ICE_CAVERN_FINAL_ROOM, SCENE_ICE_CAVERN, RCQUEST_VANILLA, (AnyAgeTime([]{return logic->CanKillEnemy(RE_WOLFOS);})) && logic->CanKillEnemy(RE_WOLFOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/ice_cavern.cpp:291
ENEMY_DROP_WITNESS(WOLFOS, RR_ICE_CAVERN_MQ_HUB, SCENE_ICE_CAVERN, RCQUEST_MQ, (AnyAgeTime([]{return logic->CanKillEnemy(RE_WHITE_WOLFOS) && logic->CanKillEnemy(RE_FREEZARD);})) && logic->CanKillEnemy(RE_WHITE_WOLFOS))
// soh/soh/Enhancements/randomizer/location_access/overworld/sacred_forest_meadow.cpp:10
ENEMY_DROP_WITNESS(WOLFOS, RR_SFM_ENTRYWAY, SCENE_SACRED_FOREST_MEADOW, RCQUEST_BOTH, (logic->IsChild && logic->CanKillEnemy(RE_WOLFOS)) && logic->CanKillEnemy(RE_WOLFOS))
// soh/soh/Enhancements/randomizer/location_access/overworld/sacred_forest_meadow.cpp:90
ENEMY_DROP_WITNESS(WOLFOS, RR_SFM_WOLFOS_GROTTO, SCENE_GROTTOS, RCQUEST_BOTH, (logic->CanKillEnemy(RE_WOLFOS, ED_CLOSE, true, 2) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_WOLFOS, ED_CLOSE, true, 2))
