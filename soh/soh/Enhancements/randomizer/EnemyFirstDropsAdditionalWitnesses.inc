// Additional physical witnesses audited against OoTMM
// 0c0f8a8e1412f3bc979e77a2563293770d58a439, with native region access
// and native combat (never donor numeric actor IDs).
// data/world/oot/dungeons/dodongo_cavern.yml:24-28: Baby Dodongo's
// death explosion opens the side room from this corridor; sword is a safe kill.
ENEMY_DROP_WITNESS(BABY_DODONGO, RR_DODONGOS_CAVERN_SE_CORRIDOR, SCENE_DODONGOS_CAVERN, RCQUEST_VANILLA, logic->CanUseSword())
// data/world/oot/dungeons_mq/deku_tree_mq.yml:132-137 explicitly requires
// Gohma Larvae in the slingshot room, matching the native Deku Baba clear route.
ENEMY_DROP_WITNESS(GOHMA_LARVA, RR_DEKU_TREE_MQ_SLINGSHOT_ROOM, SCENE_DEKU_TREE, RCQUEST_MQ, logic->CanKillEnemy(RE_GOHMA_LARVA) && logic->CanKillEnemy(RE_DEKU_BABA))
// Native jabujabus_belly.cpp's Fork North exit documents the Bari kill;
// OoTMM data/world/oot/dungeons/jabu_jabu.yml:69-74 confirms the spawned Bari
// after the green parasite dies. Preserve that event before allowing the drop.
ENEMY_DROP_WITNESS(BIRI_BARI, RR_JABU_JABUS_BELLY_FORK_NORTH, SCENE_JABU_JABU, RCQUEST_VANILLA, logic->Get(LOGIC_JABU_NORTH_TENTACLE) && logic->CanKillEnemy(RE_BARI))
// data/world/oot/dungeons/forest_temple.yml:135-145 and
// data/world/oot/dungeons_mq/forest_temple_mq.yml:173-182 expose the same
// western Wallmaster room from the twisting hall. Native region access already
// accounts for the required keys and hallway switch in the selected layout.
ENEMY_DROP_WITNESS(WALLMASTER, RR_FOREST_TEMPLE_NW_HALLWAY_STRAIGHTENED, SCENE_FOREST_TEMPLE, RCQUEST_VANILLA, logic->CanKillEnemy(RE_WALLMASTER))
ENEMY_DROP_WITNESS(WALLMASTER, RR_FOREST_TEMPLE_MQ_STRAIGHT_HALLWAY, SCENE_FOREST_TEMPLE, RCQUEST_MQ, logic->CanKillEnemy(RE_WALLMASTER))
// Pinned OoTMM spirit_temple.yml:15-16 explicitly places flying pots in
// the lobby; MQ spirit_temple_mq.yml:105-106 places them in the statue room.
// Native EnTuboTrap_Fly breaks on a shield bounce as well as an AC hit.
ENEMY_DROP_WITNESS(FLYING_POT, RR_SPIRIT_TEMPLE_FOYER, SCENE_SPIRIT_TEMPLE, RCQUEST_VANILLA, logic->CanUseSword() || logic->CanStandingShield())
ENEMY_DROP_WITNESS(FLYING_POT, RR_SPIRIT_TEMPLE_MQ_STATUE_ROOM, SCENE_SPIRIT_TEMPLE, RCQUEST_MQ, logic->CanUseSword() || logic->CanStandingShield())
// Native hyrule_field.cpp's RR_HF_TEKTITE_GROTTO is the physical pond
// encounter, also present in pinned OoTMM hyrule_field.yml:329-336. The real
// EnTite damage table accepts both Slingshot and Normal arrow (1/2 damage).
// Requiring a usable projectile conservatively kills from the entrance ledge;
// no underwater melee or disabled damage trick is assumed.
ENEMY_DROP_WITNESS(TEKTITE, RR_HF_TEKTITE_GROTTO, SCENE_GROTTOS, RCQUEST_BOTH, logic->CanUse(RG_FAIRY_SLINGSHOT) || logic->CanUse(RG_FAIRY_BOW))
// Native sacred_forest_meadow.cpp documents the adult maze's club Moblin
// at the RR_SACRED_FOREST_MEADOW -> RR_SFM_ABOVE_MAZE ledge. Both EnMb
// damage tables accept Normal arrow (2 damage); ranged attacks from the
// maze avoid requiring the disabled club-knockback/hover-boots trick.
ENEMY_DROP_WITNESS(MOBLIN, RR_SACRED_FOREST_MEADOW, SCENE_SACRED_FOREST_MEADOW, RCQUEST_BOTH, logic->IsAdult && logic->CanUse(RG_FAIRY_BOW))
// Native EnEncount1_SpawnStalchildOrWolfos explicitly chooses ACTOR_EN_SKB
// in SCENE_HYRULE_FIELD, aborting by day or with the Bunny Hood equipped.
// Child night access on field ground can always unequip the optional mask.
// Native EnSkb's sword damage table and confirmed-death callback are used.
ENEMY_DROP_WITNESS(STALCHILD, RR_HYRULE_FIELD, SCENE_HYRULE_FIELD, RCQUEST_BOTH, logic->IsChild && logic->AtNight && logic->CanUseSword())
// Native RC_LW_SKULL_KID belongs to RR_THE_LOST_WOODS. EnSkj_Init's
// combat roles initialize health=10 and a sword-vulnerable damage table;
// the additional hostile roles require the adult trade slot >= Poacher's Saw.
// Every later adult-trade item occupies a slot >= Saw too. In particular,
// unshuffled adult trade supplies Claim Check instead of a shuffled Saw.
ENEMY_DROP_WITNESS(SKULL_KID, RR_THE_LOST_WOODS, SCENE_LOST_WOODS, RCQUEST_BOTH, logic->IsAdult && (logic->HasItem(RG_POACHERS_SAW) || logic->HasItem(RG_BROKEN_SWORD) || logic->HasItem(RG_PRESCRIPTION) || logic->HasItem(RG_EYEBALL_FROG) || logic->HasItem(RG_EYEDROPS) || logic->HasItem(RG_CLAIM_CHECK)) && logic->CanUseSword())
// Native lake_hylia.cpp RC_LH_FREESTANDING_POH explicitly models the Guay
// atop the adult laboratory roof. Preserve roof access, climb and airborne
// distance, then require the native soul-aware kill instead of mere avoidance.
ENEMY_DROP_WITNESS(GUAY, RR_LAKE_HYLIA, SCENE_LAKE_HYLIA, RCQUEST_BOTH, logic->IsAdult && (logic->ReachScarecrow() || logic->BeanPlanted(LOGIC_PLANT_LAKE_HYLIA_BEAN)) && logic->HasItem(RG_CLIMB) && logic->CanKillEnemy(RE_GUAY, ED_FAR, false))
// Hyrule Field's child daytime Peahats are native PEAHAT_TYPE_FLYING / GROUNDED
// roles. EnPeehat's flying state explicitly requires IS_DAY, and its real damage
// table accepts Slingshot (1 damage). This also matches pinned OoTMM's
// macros_oot.yml can_kill_peahats_high; no night bomb or recoil trick is assumed.
ENEMY_DROP_WITNESS(PEAHAT, RR_HYRULE_FIELD, SCENE_HYRULE_FIELD, RCQUEST_BOTH, logic->IsChild && logic->AtDay && logic->CanUse(RG_FAIRY_SLINGSHOT))
// The MQ Deku lobby's vine-wall encounter is reachable from 1F independently
// of choosing the ladder. Use the same native regular-walltula kill as the
// explicit Vanilla Spirit big-wall route; Gold Skulltulas are not this type.
ENEMY_DROP_WITNESS(SKULLWALLTULA, RR_DEKU_TREE_MQ_1F, SCENE_DEKU_TREE, RCQUEST_MQ, logic->CanKillEnemy(RE_WALLTULA, ED_FAR))
// Vanilla's central fork contains the burrowed Tailpasaran heads; MQ moves
// them to the explicitly named optional Tailpasaran room. EnTp's native damage
// table accepts Boomerang as insulating damage and emits defeat in SetupDie.
// Never assume Ruto throws, damage trades or a merely removed tail segment.
ENEMY_DROP_WITNESS(TAILPASARN, RR_JABU_JABUS_BELLY_FORKED_CORRIDOR, SCENE_JABU_JABU, RCQUEST_VANILLA, logic->CanUse(RG_BOOMERANG))
ENEMY_DROP_WITNESS(TAILPASARN, RR_JABU_JABUS_BELLY_MQ_FORK_NORTH_WEST, SCENE_JABU_JABU, RCQUEST_MQ, logic->CanUse(RG_BOOMERANG))
// Additional implicit Vanilla encounters: these rooms are reachable in the
// native graph without a kill requirement because their enemies can be avoided.
// First-type rewards add the corresponding native kill/reflect requirement.
ENEMY_DROP_WITNESS(DEKU_SCRUB, RR_DEKU_TREE_BASEMENT_SCRUB_ROOM, SCENE_DEKU_TREE, RCQUEST_VANILLA, logic->CanReflectNuts() && logic->CanKillEnemy(RE_MAD_SCRUB))
ENEMY_DROP_WITNESS(DODONGO, RR_DODONGOS_CAVERN_DODONGO_ROOM, SCENE_DODONGOS_CAVERN, RCQUEST_VANILLA, logic->CanKillEnemy(RE_DODONGO))
ENEMY_DROP_WITNESS(GOHMA_LARVA, RR_DEKU_TREE_BASEMENT_TORCH_ROOM, SCENE_DEKU_TREE, RCQUEST_VANILLA, logic->CanKillEnemy(RE_GOHMA_LARVA))
// EnEncount1's sand spawner supplies ordinary/big Leevers in Desert Colossus.
// The native purple-Leever kill also guarantees the ten prior ordinary kills.
ENEMY_DROP_WITNESS(LEEVER, RR_DESERT_COLOSSUS, SCENE_DESERT_COLOSSUS, RCQUEST_BOTH, logic->CanKillEnemy(RE_PURPLE_LEEVER))
// Pinned OoTMM spirit_temple_mq.yml:81 explicitly includes Baby Dodongos
// in the child upper-ground clear room. Explosives are stated sufficient there.
ENEMY_DROP_WITNESS(BABY_DODONGO, RR_SPIRIT_TEMPLE_MQ_SUN_ON_FLOOR, SCENE_SPIRIT_TEMPLE, RCQUEST_MQ, logic->HasExplosives())
// Pinned OoTMM forest_temple.yml:82: the Vanilla western Floormaster room.
ENEMY_DROP_WITNESS(FLOORMASTER, RR_FOREST_TEMPLE_FLOORMASTER_ROOM, SCENE_FOREST_TEMPLE, RCQUEST_VANILLA, logic->CanKillEnemy(RE_FLOORMASTER))
// Native Deku Tree's map-floor vine wall carries regular walltulas; the lower
// lobby can shoot them before climbing, independently of the Skulltula 3F ring.
ENEMY_DROP_WITNESS(SKULLWALLTULA, RR_DEKU_TREE_LOBBY, SCENE_DEKU_TREE, RCQUEST_VANILLA, logic->CanKillEnemy(RE_WALLTULA, ED_FAR))
// Native Composer's Grave combines the entrance Keese room and the Redead/Sun
// Song chambers in one region. Only one ordinary Keese is needed for this type.
ENEMY_DROP_WITNESS(KEESE, RR_GRAVEYARD_COMPOSERS_GRAVE, SCENE_ROYAL_FAMILYS_TOMB, RCQUEST_BOTH, logic->CanKillEnemy(RE_KEESE))
// Pinned OoTMM dodongo_cavern.yml:112 explicitly requires the Armos soul
// between the Armos room and stairs. Its inert pushable statue is excluded by
// the native actor-role classifier; the live Armos is killed normally.
ENEMY_DROP_WITNESS(ARMOS, RR_DODONGOS_CAVERN_ARMOS_ROOM, SCENE_DODONGOS_CAVERN, RCQUEST_VANILLA, logic->CanKillEnemy(RE_ARMOS))
