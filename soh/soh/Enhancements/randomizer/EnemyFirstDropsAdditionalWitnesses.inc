// Additional physical witnesses audited against OoTMM
// 0c0f8a8e1412f3bc979e77a2563293770d58a439, with native region access
// and native combat (never donor numeric actor IDs).
// data/world/oot/dungeons/dodongo_cavern.yml:24-28: Baby Dodongo's
// death explosion opens the side room from this corridor; sword is a safe kill.
ENEMY_DROP_WITNESS(BABY_DODONGO, RR_DODONGOS_CAVERN_SE_CORRIDOR, logic->CanUseSword())
// data/world/oot/dungeons_mq/deku_tree_mq.yml:132-137 explicitly requires
// Gohma Larvae in the slingshot room, matching the native Deku Baba clear route.
ENEMY_DROP_WITNESS(GOHMA_LARVA, RR_DEKU_TREE_MQ_SLINGSHOT_ROOM, logic->CanKillEnemy(RE_GOHMA_LARVA) && logic->CanKillEnemy(RE_DEKU_BABA))
// Native jabujabus_belly.cpp's Fork North exit documents the Bari kill;
// OoTMM data/world/oot/dungeons/jabu_jabu.yml:69-74 confirms the spawned Bari
// after the green parasite dies. Preserve that event before allowing the drop.
ENEMY_DROP_WITNESS(BIRI_BARI, RR_JABU_JABUS_BELLY_FORK_NORTH, logic->Get(LOGIC_JABU_NORTH_TENTACLE) && logic->CanKillEnemy(RE_BARI))
// data/world/oot/dungeons/forest_temple.yml:135-145 and
// data/world/oot/dungeons_mq/forest_temple_mq.yml:173-182 expose the same
// western Wallmaster room from the twisting hall. Native region access already
// accounts for the required keys and hallway switch in the selected layout.
ENEMY_DROP_WITNESS(WALLMASTER, RR_FOREST_TEMPLE_NW_HALLWAY_STRAIGHTENED, logic->CanKillEnemy(RE_WALLMASTER))
ENEMY_DROP_WITNESS(WALLMASTER, RR_FOREST_TEMPLE_MQ_STRAIGHT_HALLWAY, logic->CanKillEnemy(RE_WALLMASTER))
// Pinned OoTMM spirit_temple.yml:15-16 explicitly places flying pots in
// the lobby; MQ spirit_temple_mq.yml:105-106 places them in the statue room.
// Native EnTuboTrap_Fly breaks on a shield bounce as well as an AC hit.
ENEMY_DROP_WITNESS(FLYING_POT, RR_SPIRIT_TEMPLE_FOYER, logic->CanUseSword() || logic->CanStandingShield())
ENEMY_DROP_WITNESS(FLYING_POT, RR_SPIRIT_TEMPLE_MQ_STATUE_ROOM, logic->CanUseSword() || logic->CanStandingShield())
// Native hyrule_field.cpp's RR_HF_TEKTITE_GROTTO is the physical pond
// encounter, also present in pinned OoTMM hyrule_field.yml:329-336. The real
// EnTite damage table accepts both Slingshot and Normal arrow (1/2 damage).
// Requiring a usable projectile conservatively kills from the entrance ledge;
// no underwater melee or disabled damage trick is assumed.
ENEMY_DROP_WITNESS(TEKTITE, RR_HF_TEKTITE_GROTTO, logic->CanUse(RG_FAIRY_SLINGSHOT) || logic->CanUse(RG_FAIRY_BOW))
// Native sacred_forest_meadow.cpp documents the adult maze's club Moblin
// at the RR_SACRED_FOREST_MEADOW -> RR_SFM_ABOVE_MAZE ledge. Both EnMb
// damage tables accept Normal arrow (2 damage); ranged attacks from the
// maze avoid requiring the disabled club-knockback/hover-boots trick.
ENEMY_DROP_WITNESS(MOBLIN, RR_SACRED_FOREST_MEADOW, logic->IsAdult && logic->CanUse(RG_FAIRY_BOW))
// Native EnEncount1_SpawnStalchildOrWolfos explicitly chooses ACTOR_EN_SKB
// in SCENE_HYRULE_FIELD, aborting by day or with the Bunny Hood equipped.
// Child night access on field ground can always unequip the optional mask.
// Native EnSkb's sword damage table and confirmed-death callback are used.
ENEMY_DROP_WITNESS(STALCHILD, RR_HYRULE_FIELD, logic->IsChild && logic->AtNight && logic->CanUseSword())
// Native RC_LW_SKULL_KID belongs to RR_THE_LOST_WOODS. EnSkj_Init's
// combat roles initialize health=10 and a sword-vulnerable damage table;
// the additional hostile roles require the adult trade slot >= Poacher's Saw.
// Requiring adult + an owned Saw is conservative and preserves that gate.
ENEMY_DROP_WITNESS(SKULL_KID, RR_THE_LOST_WOODS, logic->IsAdult && logic->HasItem(RG_POACHERS_SAW) && logic->CanUseSword())
