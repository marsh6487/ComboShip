// Verified witnesses from native combat routes. Never attach a global reward to Root.
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:82
ENEMY_DROP_WITNESS(ANUBIS, RR_SPIRIT_TEMPLE_1F_ANUBIS, (AnyAgeTime([]{return logic->CanHitSwitch() || logic->HasItem(RG_POWER_BRACELET) || logic->CanKillEnemy(RE_ANUBIS);})) && logic->CanKillEnemy(RE_ANUBIS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:705
ENEMY_DROP_WITNESS(ANUBIS, RR_SPIRIT_TEMPLE_MQ_ANUBIS_BRIDGE_CHEST, (logic->CanKillEnemy(RE_ANUBIS) && logic->CanKillEnemy(RE_KEESE, ED_BOOMERANG)) && logic->CanKillEnemy(RE_ANUBIS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/ganons_castle.cpp:473
ENEMY_DROP_WITNESS(ARMOS, RR_GANONS_CASTLE_MQ_LOBBY, (AnyAgeTime([]{return logic->CanKillEnemy(RE_IRON_KNUCKLE) && logic->CanKillEnemy(RE_ARMOS);})) && logic->CanKillEnemy(RE_ARMOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:47
ENEMY_DROP_WITNESS(ARMOS, RR_SPIRIT_TEMPLE_CHILD_SIDE_HUB, (AnyAgeTime([]{return logic->CanKillEnemy(RE_ARMOS);})) && logic->CanKillEnemy(RE_ARMOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:434
ENEMY_DROP_WITNESS(BEAMOS, RR_SPIRIT_TEMPLE_BEAMOS_PITS, (logic->CanKillEnemy(RE_BEAMOS)) && logic->CanKillEnemy(RE_BEAMOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:1030
ENEMY_DROP_WITNESS(BEAMOS, RR_SPIRIT_TEMPLE_MQ_BEAMOS_PITS, (logic->CanKillEnemy(RE_BEAMOS) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_BEAMOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:208
ENEMY_DROP_WITNESS(BUBBLE, RR_FOREST_TEMPLE_MAP_ROOM, (AnyAgeTime([]{return logic->CanKillEnemy(RE_BLUE_BUBBLE);})) && logic->CanKillEnemy(RE_BLUE_BUBBLE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/water_temple.cpp:577
ENEMY_DROP_WITNESS(DARK_LINK, RR_WATER_TEMPLE_DARK_LINK_ROOM, (logic->CanKillEnemy(RE_DARK_LINK)) && logic->CanKillEnemy(RE_DARK_LINK))
// soh/soh/Enhancements/randomizer/location_access/dungeons/water_temple.cpp:1176
ENEMY_DROP_WITNESS(DARK_LINK, RR_WATER_TEMPLE_MQ_DARK_LINK_ROOM, (AnyAgeTime([]{return logic->CanKillEnemy(RE_DARK_LINK);})) && logic->CanKillEnemy(RE_DARK_LINK))
// soh/soh/Enhancements/randomizer/location_access/dungeons/bottom_of_the_well.cpp:185
ENEMY_DROP_WITNESS(DEAD_HAND, RR_BOTW_DEAD_HAND_ROOM, (logic->CanKillEnemy(RE_DEAD_HAND)) && logic->CanKillEnemy(RE_DEAD_HAND))
// soh/soh/Enhancements/randomizer/location_access/dungeons/bottom_of_the_well.cpp:471
ENEMY_DROP_WITNESS(DEAD_HAND, RR_BOTW_MQ_DEAD_HAND_ROOM, (logic->CanKillEnemy(RE_DEAD_HAND)) && logic->CanKillEnemy(RE_DEAD_HAND))
// soh/soh/Enhancements/randomizer/location_access/dungeons/deku_tree.cpp:295
ENEMY_DROP_WITNESS(DEKU_BABA, RR_DEKU_TREE_MQ_SLINGSHOT_ROOM, (logic->CanKillEnemy(RE_DEKU_BABA)) && logic->CanKillEnemy(RE_DEKU_BABA))
// soh/soh/Enhancements/randomizer/location_access/overworld/kokiri_forest.cpp:106
ENEMY_DROP_WITNESS(DEKU_BABA, RR_KOKIRI_FOREST, (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOVER_BOOTS) || ((logic->IsChild || logic->CanKillEnemy(RE_DEKU_BABA) || Combo_SOH_CrossBossLogicalClear(COMBO_BOSS_PHANTOM_GANON, logic->Get(LOGIC_FOREST_TEMPLE_CLEAR))) && logic->CanClimbLadder())) && logic->CanKillEnemy(RE_DEKU_BABA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/deku_tree.cpp:371
ENEMY_DROP_WITNESS(DEKU_SCRUB, RR_DEKU_TREE_MQ_BASEMENT_SOUTHEAST_ROOM, (logic->CanKillEnemy(RE_MAD_SCRUB)) && logic->CanKillEnemy(RE_MAD_SCRUB))
// soh/soh/Enhancements/randomizer/location_access/dungeons/dodongos_cavern.cpp:449
ENEMY_DROP_WITNESS(DODONGO, RR_DODONGOS_CAVERN_MQ_DODONGO_ROOM, (AnyAgeTime([]{return logic->CanKillEnemy(RE_DODONGO) || logic->HasStrength(1);})) && logic->CanKillEnemy(RE_DODONGO))
// soh/soh/Enhancements/randomizer/location_access/dungeons/fire_temple.cpp:100
ENEMY_DROP_WITNESS(FLARE_DANCER, RR_FIRE_TEMPLE_LOOP_FLARE_DANCER, (AnyAgeTime([]{return logic->CanKillEnemy(RE_FLARE_DANCER);})) && logic->CanKillEnemy(RE_FLARE_DANCER))
// soh/soh/Enhancements/randomizer/location_access/dungeons/fire_temple.cpp:1091
ENEMY_DROP_WITNESS(FLARE_DANCER, RR_FIRE_TEMPLE_MQ_3F_FLARE_DANCER, (logic->CanKillEnemy(RE_FLARE_DANCER)) && logic->CanKillEnemy(RE_FLARE_DANCER))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:583
ENEMY_DROP_WITNESS(FLOORMASTER, RR_FOREST_TEMPLE_MQ_FLOORMASTER_ROOM, (AnyAgeTime([]{return logic->CanKillEnemy(RE_FLOORMASTER);})) && logic->CanKillEnemy(RE_FLOORMASTER))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:406
ENEMY_DROP_WITNESS(FLOORMASTER, RR_SHADOW_TEMPLE_X_CROSS, (AnyAgeTime([]{return (ctx->GetTrickOption(RT_LENS_SHADOW) || logic->CanUse(RG_LENS_OF_TRUTH)) && logic->CanKillEnemy(RE_FLOORMASTER);})) && logic->CanKillEnemy(RE_FLOORMASTER))
// soh/soh/Enhancements/randomizer/location_access/dungeons/gerudo_training_ground.cpp:334
ENEMY_DROP_WITNESS(FREEZARD, RR_GERUDO_TRAINING_GROUND_MQ_ROOM_BEHIND_BLOCK, (logic->CanKillEnemy(RE_FREEZARD) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_FREEZARD))
// soh/soh/Enhancements/randomizer/location_access/dungeons/ice_cavern.cpp:39
ENEMY_DROP_WITNESS(FREEZARD, RR_ICE_CAVERN_BEGINNING, (AnyAgeTime([]{return logic->CanKillEnemy(RE_FREEZARD, ED_CLOSE, true, 4);})) && logic->CanKillEnemy(RE_FREEZARD, ED_CLOSE, true, 4))
// soh/soh/Enhancements/randomizer/location_access/dungeons/fire_temple.cpp:567
ENEMY_DROP_WITNESS(IRON_KNUCKLE, RR_FIRE_TEMPLE_MQ_LOOP_5_TILE_ROOM, (AnyAgeTime([]{return logic->CanKillEnemy(RE_IRON_KNUCKLE);})) && logic->CanKillEnemy(RE_IRON_KNUCKLE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:355
ENEMY_DROP_WITNESS(IRON_KNUCKLE, RR_SPIRIT_TEMPLE_CHILD_THRONE, (AnyAgeTime([]{return logic->CanKillEnemy(RE_IRON_KNUCKLE);})) && logic->CanKillEnemy(RE_IRON_KNUCKLE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:284
ENEMY_DROP_WITNESS(KEESE, RR_SHADOW_TEMPLE_SKULL_JAR, (AnyAgeTime([]{return logic->CanKillEnemy(RE_KEESE);})) && logic->CanKillEnemy(RE_KEESE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:705
ENEMY_DROP_WITNESS(KEESE, RR_SPIRIT_TEMPLE_MQ_ANUBIS_BRIDGE_CHEST, (logic->CanKillEnemy(RE_ANUBIS) && logic->CanKillEnemy(RE_KEESE, ED_BOOMERANG)) && logic->CanKillEnemy(RE_KEESE, ED_BOOMERANG))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:982
ENEMY_DROP_WITNESS(LEEVER, RR_SPIRIT_TEMPLE_MQ_SAND_PIT, (logic->CanKillEnemy(RE_PURPLE_LEEVER) && (logic->CanUse(RG_HOOKSHOT) || logic->CanRecoilHover(RECOIL_HAMMER)) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_PURPLE_LEEVER))
// soh/soh/Enhancements/randomizer/location_access/dungeons/fire_temple.cpp:537
ENEMY_DROP_WITNESS(LIKE_LIKE, RR_FIRE_TEMPLE_MQ_LOOP_CAGE_FOYER_SIDE, (AnyAgeTime([]{return logic->CanKillEnemy(RE_LIKE_LIKE);})) && logic->CanKillEnemy(RE_LIKE_LIKE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/gerudo_training_ground.cpp:114
ENEMY_DROP_WITNESS(LIKE_LIKE, RR_GERUDO_TRAINING_GROUND_LIKE_LIKE_ROOM, (logic->CanKillEnemy(RE_LIKE_LIKE) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_LIKE_LIKE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/dodongos_cavern.cpp:93
ENEMY_DROP_WITNESS(LIZALFOS_DINOLFOS, RR_DODONGOS_CAVERN_LOWER_LIZALFOS, (logic->CanKillEnemy(RE_LIZALFOS, ED_CLOSE, true, 2)) && logic->CanKillEnemy(RE_LIZALFOS, ED_CLOSE, true, 2))
// soh/soh/Enhancements/randomizer/location_access/dungeons/jabujabus_belly.cpp:386
ENEMY_DROP_WITNESS(LIZALFOS_DINOLFOS, RR_JABU_JABUS_BELLY_MQ_WATER_SWITCH_ROOM, (logic->CanKillEnemy(RE_LIZALFOS)) && logic->CanKillEnemy(RE_LIZALFOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/jabujabus_belly.cpp:213
ENEMY_DROP_WITNESS(OCTOROK, RR_JABU_JABUS_BELLY_BIGOCTO_LEDGE, (logic->Get(LOGIC_JABU_RUTO_IN_1F) && AnyAgeTime([]{return logic->CanKillEnemy(RE_BIG_OCTO);})) && logic->CanKillEnemy(RE_BIG_OCTO))
// soh/soh/Enhancements/randomizer/location_access/dungeons/jabujabus_belly.cpp:551
ENEMY_DROP_WITNESS(OCTOROK, RR_JABU_JABUS_BELLY_MQ_ABOVE_BIGOCTO, (logic->TakeDamage() && AnyAgeTime([]{return logic->CanKillEnemy(RE_BIG_OCTO);})) && logic->CanKillEnemy(RE_BIG_OCTO))
// soh/soh/Enhancements/randomizer/location_access/dungeons/jabujabus_belly.cpp:140
ENEMY_DROP_WITNESS(PARASITE, RR_JABU_JABUS_BELLY_FORK_WEST, (logic->CanKillEnemy(RE_TENTACLE, ED_BOOMERANG)) && logic->CanKillEnemy(RE_TENTACLE, ED_BOOMERANG))
// soh/soh/Enhancements/randomizer/location_access/dungeons/jabujabus_belly.cpp:434
ENEMY_DROP_WITNESS(PARASITE, RR_JABU_JABUS_BELLY_MQ_FORK_WEST, (logic->CanKillEnemy(RE_TENTACLE, ED_BOOMERANG)) && logic->CanKillEnemy(RE_TENTACLE, ED_BOOMERANG))
// soh/soh/Enhancements/randomizer/location_access/dungeons/dodongos_cavern.cpp:701
ENEMY_DROP_WITNESS(POE, RR_DODONGOS_CAVERN_MQ_BACK_POE_ROOM, (AnyAgeTime([]{return logic->CanKillEnemy(RE_POE);})) && logic->CanKillEnemy(RE_POE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:47
ENEMY_DROP_WITNESS(POE, RR_FOREST_TEMPLE_LOBBY, (logic->Get(LOGIC_FOREST_JOELLE) && logic->Get(LOGIC_FOREST_BETH) && logic->Get(LOGIC_FOREST_AMY) && logic->CanKillEnemy(RE_MEG)) && logic->CanKillEnemy(RE_MEG))
// soh/soh/Enhancements/randomizer/location_access/overworld/kakariko.cpp:272
ENEMY_DROP_WITNESS(REDEAD_GIBDO, RR_KAK_REDEAD_GROTTO, (logic->CanKillEnemy(RE_REDEAD, ED_CLOSE, true, 2) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_REDEAD, ED_CLOSE, true, 2))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:533
ENEMY_DROP_WITNESS(REDEAD_GIBDO, RR_SHADOW_TEMPLE_MQ_B2_GIBDO_ROOM, (AnyAgeTime([]{return logic->CanKillEnemy(RE_GIBDO);})) && logic->CanKillEnemy(RE_GIBDO))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:311
ENEMY_DROP_WITNESS(REDEAD_GIBDO, RR_SHADOW_TEMPLE_WIND_TUNNEL_HINT_ROOM, (logic->CanKillEnemy(RE_REDEAD)) && logic->CanKillEnemy(RE_REDEAD))
// soh/soh/Enhancements/randomizer/location_access/dungeons/jabujabus_belly.cpp:162
ENEMY_DROP_WITNESS(SHABOM, RR_JABU_JABUS_BELLY_FORK_NORTH_WEST, (AnyAgeTime([]{return logic->CanKillEnemy(RE_SHABOM, ED_CLOSE, false, 9);})) && logic->CanKillEnemy(RE_SHABOM, ED_CLOSE, false, 9))
// soh/soh/Enhancements/randomizer/location_access/dungeons/water_temple.cpp:196
ENEMY_DROP_WITNESS(SHELL_BLADE, RR_WATER_TEMPLE_TORCH_ROOM, (AnyAgeTime([]{return (logic->WaterLevel(WL_LOW) && logic->CanKillEnemy(RE_SHELL_BLADE, ED_CLOSE, true, 3) && logic->HasItem(RG_OPEN_CHEST)) || logic->CanOpenUnderwaterChest();})) && logic->CanKillEnemy(RE_SHELL_BLADE, ED_CLOSE, true, 3))
// soh/soh/Enhancements/randomizer/location_access/dungeons/ganons_castle.cpp:420
ENEMY_DROP_WITNESS(SKULLTULA, RR_GANONS_CASTLE_LIGHT_TRIAL_CHESTS_ROOM, ((ctx->GetTrickOption(RT_LENS_GANON) || logic->CanUse(RG_LENS_OF_TRUTH)) && (logic->CanKillEnemy(RE_BIG_SKULLTULA) && logic->CanKillEnemy(RE_KEESE, ED_CLOSE, true, 3)) && logic->HasItem(RG_OPEN_CHEST)) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/shadow_temple.cpp:538
ENEMY_DROP_WITNESS(SKULLTULA, RR_SHADOW_TEMPLE_MQ_B2_SPINNING_BLADE_ROOM, (logic->CanKillEnemy(RE_BIG_SKULLTULA) && (logic->CanUse(RG_HOOKSHOT) || (logic->IsAdult && (logic->CanUse(RG_HOVER_BOOTS) || logic->CanGroundJump() || logic->BunnyHood())))) && logic->CanKillEnemy(RE_BIG_SKULLTULA))
// soh/soh/Enhancements/randomizer/location_access/dungeons/water_temple.cpp:183
ENEMY_DROP_WITNESS(SPIKE, RR_WATER_TEMPLE_4_SPIKES_ROOM, (AnyAgeTime([]{return logic->CanKillEnemy(RE_SPIKE, ED_CLOSE, true, 4);})) && logic->CanKillEnemy(RE_SPIKE, ED_CLOSE, true, 4))
// soh/soh/Enhancements/randomizer/location_access/dungeons/water_temple.cpp:882
ENEMY_DROP_WITNESS(SPIKE, RR_WATER_TEMPLE_MQ_TOWER_TARGET_ROOM, (logic->CanKillEnemy(RE_LIZALFOS) && logic->CanKillEnemy(RE_SPIKE)) && logic->CanKillEnemy(RE_SPIKE))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:78
ENEMY_DROP_WITNESS(STALFOS, RR_FOREST_TEMPLE_LOWER_STALFOS, (logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 2)) && logic->CanKillEnemy(RE_STALFOS, ED_CLOSE, true, 2))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:668
ENEMY_DROP_WITNESS(STALFOS, RR_SPIRIT_TEMPLE_MQ_TURNTABLE, (logic->CanKillEnemy(RE_STALFOS)) && logic->CanKillEnemy(RE_STALFOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/jabujabus_belly.cpp:199
ENEMY_DROP_WITNESS(STINGER, RR_JABU_JABUS_BELLY_FORK_EAST, ((logic->Get(LOGIC_JABU_RUTO_IN_1F) || logic->CanKillEnemy(RE_STINGER, ED_CLOSE, true, 4)) && logic->CanOpenLargeChest()) && logic->CanKillEnemy(RE_STINGER, ED_CLOSE, true, 4))
// soh/soh/Enhancements/randomizer/location_access/dungeons/jabujabus_belly.cpp:510
ENEMY_DROP_WITNESS(STINGER, RR_JABU_JABUS_BELLY_MQ_INVISIBLE_KEESE_ROOM, ((logic->HasFireProjectile()) && logic->CanGetEnemyDrop(RE_GOLD_SKULLTULA, ED_LONGSHOT) || ((logic->CanGetEnemyDrop(RE_GOLD_SKULLTULA, ED_BOOMERANG) || (logic->IsAdult && logic->CanGroundJumpslash())) && ((logic->CanUse(RG_HOVER_BOOTS) || AnyAgeTime([]{return (ctx->GetTrickOption(RT_LENS_JABU_MQ) || logic->CanUse(RG_LENS_OF_TRUTH)) && logic->CanKillEnemy(RE_STINGER, ED_BOOMERANG, false, 2, false, true) && (logic->CanKillEnemy(RE_KEESE, ED_LONGSHOT, false) || (ctx->GetTrickOption(RT_LENS_JABU_MQ) && logic->CanUse(RG_HOOKSHOT) && logic->CanUse(RG_IRON_BOOTS)));})) && ((logic->IsChild && logic->HasItem(RG_BRONZE_SCALE)) || (logic->IsAdult && logic->CanUse(RG_IRON_BOOTS)) || (logic->BunnyHood() && logic->CanJumpslash()))))) && logic->CanKillEnemy(RE_STINGER, ED_BOOMERANG, false, 2, false, true))
// soh/soh/Enhancements/randomizer/location_access/overworld/thieves_hideout.cpp:11
ENEMY_DROP_WITNESS(THIEVES, RR_TH_1_TORCH_CELL, (logic->CanKillEnemy(RE_GERUDO_WARRIOR) && logic->HasItem(RG_SPEAK_HYLIAN)) && logic->CanKillEnemy(RE_GERUDO_WARRIOR))
// soh/soh/Enhancements/randomizer/location_access/dungeons/fire_temple.cpp:83
ENEMY_DROP_WITNESS(TORCH_SLUG, RR_FIRE_TEMPLE_LOOP_HEXAGON_ROOM, (AnyAgeTime([]{return logic->CanKillEnemy(RE_TORCH_SLUG) && logic->CanKillEnemy(RE_FIRE_KEESE);})) && logic->CanKillEnemy(RE_TORCH_SLUG))
// soh/soh/Enhancements/randomizer/location_access/dungeons/spirit_temple.cpp:633
ENEMY_DROP_WITNESS(TORCH_SLUG, RR_SPIRIT_TEMPLE_MQ_CHILD_SIDE_HUB, (AnyAgeTime([]{return logic->CanKillEnemy(RE_TORCH_SLUG);})) && logic->CanKillEnemy(RE_TORCH_SLUG))
// soh/soh/Enhancements/randomizer/location_access/dungeons/forest_temple.cpp:491
ENEMY_DROP_WITNESS(WOLFOS, RR_FOREST_TEMPLE_MQ_WOLFOS_ROOM, (logic->CanKillEnemy(RE_WOLFOS)) && logic->CanKillEnemy(RE_WOLFOS))
// soh/soh/Enhancements/randomizer/location_access/dungeons/ice_cavern.cpp:228
ENEMY_DROP_WITNESS(WOLFOS, RR_ICE_CAVERN_FINAL_ROOM, (AnyAgeTime([]{return logic->CanKillEnemy(RE_WOLFOS);})) && logic->CanKillEnemy(RE_WOLFOS))
// soh/soh/Enhancements/randomizer/location_access/overworld/sacred_forest_meadow.cpp:10
ENEMY_DROP_WITNESS(WOLFOS, RR_SFM_ENTRYWAY, (logic->IsChild && logic->CanKillEnemy(RE_WOLFOS)) && logic->CanKillEnemy(RE_WOLFOS))
