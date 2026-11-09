#include "Rando/SoulShuffleRuntime.cpp"
#include "tests/test_require.h"
#include <cstdio>
extern "C" {
SaveContext gSaveContext{};
s32 Flags_GetRandoInf(s32 f) { return (gSaveContext.save.shipSaveInfo.rando.randoInf[f/16] >> (f%16)) & 1; }
}
static void Own(RandoItemId item) {
    auto*s=Rando::SoulShuffle::Find(item); REQUIRE(s);
    gSaveContext.save.shipSaveInfo.rando.randoInf[s->flag/16]|=1u<<(s->flag%16);
}
static int spawnReachesAllocation=0;
#include "mm_spawn.inc"
int main(){
    PlayState play{};
    gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_RANDO;

    struct Shop { int scene; unsigned params; RandoItemId soul; };
    Shop shops[]={
#define SOUL_SHOP(scene,params,soul) {scene,params,soul},
#include "Rando/SoulShuffleShops.inc"
#undef SOUL_SHOP
    };
    for(const auto& shop:shops) {
        play.sceneId=shop.scene;
        RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS]=0;
        REQUIRE(MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_SOB1,shop.params));
        RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS]=1;
        REQUIRE(!MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_SOB1,shop.params));
        REQUIRE(MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_SOB1,15));
        play.sceneId=0x7FFF;
        REQUIRE(MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_SOB1,shop.params));
        play.sceneId=shop.scene;
        Own(shop.soul);
        REQUIRE(MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_SOB1,shop.params));
        const auto* soul=Rando::SoulShuffle::Find(shop.soul);
        gSaveContext.save.shipSaveInfo.rando.randoInf[soul->flag/16]&=~(1u<<(soul->flag%16));
    }
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS]=0;
    play.sceneId=0;
    REQUIRE(MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_COW,0));
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_ANIMAL_SOULS]=1;
    for(int id:{ACTOR_EN_NIW,ACTOR_EN_NWC,ACTOR_EN_COW,ACTOR_EN_DG,ACTOR_EN_RACEDOG,ACTOR_EN_BUTTE})
        REQUIRE(!MM_SoulCanSpawnNonEnemy(&play,id,0));
    REQUIRE(MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_OSSAN,0)); // custom black-market shop
    REQUIRE(MM_SoulCanSpawnNonEnemy(&play,ACTOR_DM_CHAR00,0)); // generic cutscene proxy
    play.sceneId=SCENE_F01C;
    REQUIRE(!MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_HS,0));
    play.sceneId=SCENE_F01_B;
    REQUIRE(!MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_AOB_01,0));
    Own(RI_SOUL_ANIMAL_CUCCO);Own(RI_SOUL_ANIMAL_DOG);
    REQUIRE(MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_AOB_01,0));
    play.sceneId=SCENE_F01C;
    REQUIRE(MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_HS,0));
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_MISC_SOULS]=1;
    for(int id:{ACTOR_EN_SELLNUTS,ACTOR_EN_SCOPENUTS,ACTOR_EN_AKINDONUTS})
        REQUIRE(!MM_SoulCanSpawnNonEnemy(&play,id,0));
    REQUIRE(!MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_SW,1));
    REQUIRE(MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_SW,0)); // existing ordinary enemy semantics
    Own(RI_SOUL_MISC_GS);
    REQUIRE(MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_SW,1));
    Actor_SpawnAsChildAndCutscene(&play.actorCtx,&play,ACTOR_EN_COW,0,0,0,0,0,0,0,0,0,nullptr);
    REQUIRE(spawnReachesAllocation==0);
    Own(RI_SOUL_ANIMAL_COW);
    Actor_SpawnAsChildAndCutscene(&play.actorCtx,&play,ACTOR_EN_COW,0,0,0,0,0,0,0,0,0,nullptr);
    REQUIRE(spawnReachesAllocation==1);
    gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_VANILLA;
    REQUIRE(MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_BUTTE,0));
    gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_RANDO;gSaveContext.gameMode=GAMEMODE_END_CREDITS;
    REQUIRE(MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_BUTTE,0));
    puts("PASS real MM optional-soul bridge and native preallocation spawn gate, host/custom/legacy controls");
}
