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
    optionValues[RSK_SHUFFLE_NPC_SOULS]=0;
    play.sceneNum=0;
    REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_COW,0));
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
