#include "soh/Enhancements/randomizer/SoulShuffleRuntime.cpp"
#include "tests/test_require.h"
#include <cstring>
extern "C" {
SaveContext gSaveContext;
u8 optionValues[RSK_MAX]{};
u8 Randomizer_GetSettingValue(RandomizerSettingKey k) { return optionValues[k]; }
s32 Flags_GetRandomizerInf(RandomizerInf f) { return (gSaveContext.ship.randomizerInf[f/16] >> (f%16)) & 1; }
}
static void Own(RandomizerGet item, bool on) {
    const auto* s=Rando::SoulShuffle::Find(item);
    REQUIRE(s);
    if(on) gSaveContext.ship.randomizerInf[s->flag/16]|=1u<<(s->flag%16);
    else gSaveContext.ship.randomizerInf[s->flag/16]&=~(1u<<(s->flag%16));
}
static int collisionCount=0;
static void Collision(PlayState*, CollisionCheckContext*, Collider*, Collider*) { ++collisionCount; }
static void Update(Actor*,PlayState*) {}
static void (*sACVsFuncs[4][4])(PlayState*, CollisionCheckContext*, Collider*, Collider*) = {{Collision}};
#include "collision.inc"
static int spawnReachesAllocation=0;
#include "spawn.inc"
int main() {
    PlayState play{}; Actor actor{};
    gSaveContext.ship.quest.id=QUEST_RANDOMIZER;

    // Literal independent native role expectations, not generated from the runtime map.
    struct ExpectedNpc { int actor; int scene; unsigned params; RandomizerGet soul; };
    const ExpectedNpc expectedNpcs[] = {
#include "oot_npc_roles.inc"
    };
    for (const auto& role : expectedNpcs) {
        play.sceneNum = role.scene;
        optionValues[RSK_SHUFFLE_NPC_SOULS] = 1;
        REQUIRE(!Randomizer_SoulCanSpawnNonEnemy(&play, role.actor, role.params));
        Own(role.soul, true);
        REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play, role.actor, role.params));
        Own(role.soul, false);
        optionValues[RSK_SHUFFLE_NPC_SOULS] = 0;
        REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play, role.actor, role.params));
        play.sceneNum = 0x7FFF;
        optionValues[RSK_SHUFFLE_NPC_SOULS] = 1;
        REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play, role.actor, role.params));
        optionValues[RSK_SHUFFLE_NPC_SOULS] = 0;
    }

    struct Shop { int scene; unsigned params; RandomizerGet soul; };
    Shop shops[]={
#define SOUL_SHOP(scene,params,soul) {scene,params,soul},
#include "soh/Enhancements/randomizer/SoulShuffleShops.inc"
#undef SOUL_SHOP
    };
    for(const auto& shop:shops) {
        play.sceneNum=shop.scene;
        optionValues[RSK_SHUFFLE_NPC_SOULS]=0;
        REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_OSSAN,shop.params));
        optionValues[RSK_SHUFFLE_NPC_SOULS]=1;
        REQUIRE(!Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_OSSAN,shop.params));
        REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_OSSAN,15));
        play.sceneNum=0x7FFF;
        REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_OSSAN,shop.params));
        play.sceneNum=shop.scene;
        Own(shop.soul,true);
        REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_OSSAN,shop.params));
        const auto* soul=Rando::SoulShuffle::Find(shop.soul);
        gSaveContext.ship.randomizerInf[soul->flag/16]&=~(1u<<(soul->flag%16));
    }
    // Literal native role cases: deleting a gate, using the donor's 0x3F
    // mask, or matching a different EnHy role must fail these controls.
    struct NpcRole { int actor; int scene; unsigned params; RandomizerGet soul; };
    const NpcRole roles[] = {
        {ACTOR_EN_SYATEKI_ITM, SCENE_SHOOTING_GALLERY, 0, RG_SOUL_NPC_SHOOTING_GALLERY_OWNER},
        {ACTOR_EN_SYATEKI_MAN, SCENE_SHOOTING_GALLERY, 0, RG_SOUL_NPC_SHOOTING_GALLERY_OWNER},
        {ACTOR_EN_BOM_BOWL_MAN, SCENE_BOMBCHU_BOWLING_ALLEY, 0xFFFF, RG_SOUL_NPC_BOMBCHU_BOWLING_LADY},
        {ACTOR_EN_DS, SCENE_POTION_SHOP_GRANNY, 0xFFFF, RG_SOUL_NPC_OLD_HAG},
        {ACTOR_EN_MK, SCENE_LAKESIDE_LABORATORY, 0xFFFF, RG_SOUL_NPC_SCIENTIST},
        {ACTOR_EN_MS, SCENE_ZORAS_RIVER, 0xFFFF, RG_SOUL_NPC_BEAN_SALESMAN},
        {ACTOR_EN_JS, SCENE_HAUNTED_WASTELAND, 0xFFFF, RG_SOUL_NPC_CARPET_MAN},
        {ACTOR_EN_FU, SCENE_WINDMILL_AND_DAMPES_GRAVE, 0xFFFF, RG_SOUL_NPC_GURU_GURU},
        {ACTOR_EN_HS, SCENE_LOST_WOODS, 0xFFFF, RG_SOUL_NPC_GROG},
        {ACTOR_EN_HS2, SCENE_KAKARIKO_VILLAGE, 0xFFFF, RG_SOUL_NPC_GROG},
        {ACTOR_EN_HY, SCENE_DOG_LADY_HOUSE, 0, RG_SOUL_NPC_DOG_LADY},
        {ACTOR_EN_HY, SCENE_MARKET_DAY, 0x780, RG_SOUL_NPC_DOG_LADY},
        {ACTOR_EN_HY, SCENE_KAKARIKO_CENTER_GUEST_HOUSE, 0x8000, RG_SOUL_NPC_DOG_LADY},
        {ACTOR_EN_HY, SCENE_MARKET_DAY, 5, RG_SOUL_NPC_BANKER},
        {ACTOR_EN_HY, SCENE_KAKARIKO_VILLAGE, 0x785, RG_SOUL_NPC_BANKER},
    };
    for (const auto& role : roles) {
        play.sceneNum = role.scene;
        optionValues[RSK_SHUFFLE_NPC_SOULS] = 0;
        REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play, role.actor, role.params));
        optionValues[RSK_SHUFFLE_NPC_SOULS] = 1;
        REQUIRE(!Randomizer_SoulCanSpawnNonEnemy(&play, role.actor, role.params));
        gSaveContext.ship.quest.id = QUEST_NORMAL;
        REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play, role.actor, role.params));
        gSaveContext.ship.quest.id = QUEST_RANDOMIZER;
        gSaveContext.gameMode = GAMEMODE_END_CREDITS;
        REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play, role.actor, role.params));
        gSaveContext.gameMode = GAMEMODE_NORMAL;
        Actor_Spawn(&play.actorCtx, &play, role.actor, 0, 0, 0, 0, 0, 0, role.params);
        REQUIRE(spawnReachesAllocation == 0);
        Own(role.soul, true);
        REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play, role.actor, role.params));
        Actor_Spawn(&play.actorCtx, &play, role.actor, 0, 0, 0, 0, 0, 0, role.params);
        REQUIRE(spawnReachesAllocation == 1);
        spawnReachesAllocation = 0;
        Own(role.soul, false);
        if (role.actor == ACTOR_EN_HY) {
            // EnHy role 7 now has its own Astronomer soul.
            if (role.scene == SCENE_MARKET_DAY || role.scene == SCENE_KAKARIKO_VILLAGE)
                REQUIRE(!Randomizer_SoulCanSpawnNonEnemy(&play, role.actor, 7));
            REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play, role.actor, 0x40));
            REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play, role.actor, 0x45));
        }
        play.sceneNum = SCENE_HYRULE_CASTLE;
        REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play, role.actor, role.params));
    }
    optionValues[RSK_SHUFFLE_NPC_SOULS]=0;
    play.sceneNum=0;
    REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_COW,0));
    optionValues[RSK_SHUFFLE_NPC_SOULS] = 1;
    for (int scene : {SCENE_KAKARIKO_VILLAGE, SCENE_IMPAS_HOUSE}) {
      play.sceneNum = scene;
      optionValues[RSK_SHUFFLE_NPC_SOULS] = 0;
      REQUIRE(
          Randomizer_SoulCanSpawnNonEnemy(&play, ACTOR_EN_NIW_LADY, 0xFFFF));
      optionValues[RSK_SHUFFLE_NPC_SOULS] = 1;
      REQUIRE(
          !Randomizer_SoulCanSpawnNonEnemy(&play, ACTOR_EN_NIW_LADY, 0xFFFF));
      Actor_Spawn(&play.actorCtx, &play, ACTOR_EN_NIW_LADY, 0, 0, 0, 0, 0, 0,
                  -1);
      REQUIRE(spawnReachesAllocation == 0);
      Own(RG_SOUL_NPC_ANJU, true);
      REQUIRE(
          Randomizer_SoulCanSpawnNonEnemy(&play, ACTOR_EN_NIW_LADY, 0xFFFF));
      Actor_Spawn(&play.actorCtx, &play, ACTOR_EN_NIW_LADY, 0, 0, 0, 0, 0, 0,
                  -1);
      REQUIRE(spawnReachesAllocation == 1);
      spawnReachesAllocation = 0;
      Own(RG_SOUL_NPC_ANJU, false);
    }
    play.sceneNum = SCENE_HYRULE_CASTLE;
    REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play, ACTOR_EN_NIW_LADY, 0xFFFF));
    REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play, ACTOR_EN_VIEWER, 0));
    optionValues[RSK_SHUFFLE_NPC_SOULS] = 0;
    optionValues[RSK_SHUFFLE_ANIMAL_SOULS]=1;
    REQUIRE(!Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_COW,0));
    REQUIRE(!Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_NIW,0));
    REQUIRE(!Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_DOG,0));
    REQUIRE(!Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_BUTTE,0));
    REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_VIEWER,0));
    REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_OSSAN,0));
    play.sceneNum=SCENE_LON_LON_BUILDINGS;
    REQUIRE(!Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_TA,0));
    REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_TA,1));
    play.sceneNum=SCENE_HYRULE_CASTLE;
    REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_TA,0));
    Own(RG_SOUL_ANIMAL_CUCCO,true);
    REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_NIW,0));
    play.sceneNum=SCENE_LON_LON_BUILDINGS;
    REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_TA,0));
    optionValues[RSK_SHUFFLE_MISC_SOULS]=1;
    REQUIRE(!Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_SHOPNUTS,0));
    REQUIRE(!Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_SW,0x2000));
    REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_SW,0));
    Own(RG_SOUL_MISC_BUSINESS_SCRUB,true);
    REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_SHOPNUTS,0));
    optionValues[RSK_SHUFFLE_MISC_SOULS]=0;
    Actor_Spawn(&play.actorCtx,&play,ACTOR_EN_COW,0,0,0,0,0,0,0);
    REQUIRE(spawnReachesAllocation==0);
    Own(RG_SOUL_ANIMAL_COW,true);
    Actor_Spawn(&play.actorCtx,&play,ACTOR_EN_COW,0,0,0,0,0,0,0);
    REQUIRE(spawnReachesAllocation==1);

    gSaveContext.ship.quest.id=QUEST_RANDOMIZER;
    actor.id=ACTOR_EN_TEST;
    REQUIRE(Randomizer_SoulCanDamageActor(&play,&actor));
    optionValues[RSK_SHUFFLE_ENEMY_SOULS]=1;
    REQUIRE(!Randomizer_SoulCanDamageActor(&play,&actor));
    REQUIRE(Randomizer_SoulShouldUpdateActor(&play,&actor)); // ordinary immunity preserves presence/update
    Own(RG_SOUL_ENEMY_STALFOS,true);
    REQUIRE(Randomizer_SoulCanDamageActor(&play,&actor));
    Own(RG_SOUL_ENEMY_STALFOS,false);
    actor.id=ACTOR_EN_TUBO_TRAP;
    REQUIRE(!Randomizer_SoulShouldUpdateActor(&play,&actor)); // wall/AT scripted death
    Own(RG_SOUL_ENEMY_FLYING_POT,true);
    REQUIRE(Randomizer_SoulShouldUpdateActor(&play,&actor));
    actor.id=ACTOR_EN_ANUBICE;
    REQUIRE(!Randomizer_SoulShouldUpdateActor(&play,&actor)); // flame-circle scripted death
    Own(RG_SOUL_ENEMY_ANUBIS,true);
    REQUIRE(Randomizer_SoulShouldUpdateActor(&play,&actor));
    actor.id=ACTOR_EN_OSSAN;
    REQUIRE(Randomizer_SoulShouldUpdateActor(&play,&actor));
    REQUIRE(Randomizer_SoulCanDamageActor(&play,&actor));
    actor.id=ACTOR_EN_SW;actor.params=0x2000;
    REQUIRE(Randomizer_SoulCanDamageActor(&play,&actor)); // independent misc option
    optionValues[RSK_SHUFFLE_MISC_SOULS]=1;
    REQUIRE(!Randomizer_SoulCanDamageActor(&play,&actor));
    Own(RG_SOUL_MISC_GS,true);
    REQUIRE(Randomizer_SoulCanDamageActor(&play,&actor));
    actor.id=ACTOR_EN_TEST;actor.params=0;
    gSaveContext.ship.quest.id=QUEST_NORMAL;
    REQUIRE(Randomizer_SoulCanDamageActor(&play,&actor));
    gSaveContext.ship.quest.id=QUEST_RANDOMIZER;gSaveContext.gameMode=GAMEMODE_END_CREDITS;
    REQUIRE(Randomizer_SoulCanDamageActor(&play,&actor));
    REQUIRE(Randomizer_SoulCanDamageActor(nullptr,&actor));
    REQUIRE(Randomizer_SoulCanDamageActor(&play,nullptr));
    gSaveContext.gameMode=GAMEMODE_NORMAL;
    actor.update=Update;
    actor.id=ACTOR_EN_TEST;
    Collider at{}, ac{};
    at.atFlags=AT_ON|AT_TYPE_PLAYER;ac.acFlags=AC_ON|AC_TYPE_PLAYER;ac.actor=&actor;
    CollisionCheckContext context{};
    Collider* entries[]={&ac};
    context.colAC=entries;context.colACCount=1;
    Actor before=actor;
    CollisionCheck_AC(&play,&context,&at);
    REQUIRE(collisionCount==0);
    REQUIRE(memcmp(&before,&actor,sizeof(actor))==0);
    Own(RG_SOUL_ENEMY_STALFOS,true);
    CollisionCheck_AC(&play,&context,&at);
    REQUIRE(collisionCount==1);
    REQUIRE(memcmp(&before,&actor,sizeof(actor))==0);
    puts("PASS actual native actor bridge: immunity, special death/update, independent misc, vanilla/credits controls");
    return 0;
}
