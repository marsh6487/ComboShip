#include "Rando/SoulShuffleRuntime.cpp"
#include "tests/test_require.h"
#include <cstdio>
extern "C" {
SaveContext gSaveContext{};
s32 Flags_GetRandoInf(s32 f) { return (gSaveContext.save.shipSaveInfo.rando.randoInf[f/16] >> (f%16)) & 1; }
}
static void Own(RandoItemId item, bool on = true) {
  auto *s = Rando::SoulShuffle::Find(item);
  REQUIRE(s);
  if (on)
    gSaveContext.save.shipSaveInfo.rando.randoInf[s->flag / 16] |=
        1u << (s->flag % 16);
  else
    gSaveContext.save.shipSaveInfo.rando.randoInf[s->flag / 16] &=
        ~(1u << (s->flag % 16));
}
#include "src/overlays/actors/ovl_En_Nwc/z_en_nwc.h"
#include <cstring>
static int followCount = 0, chimeCount = 0;
extern "C" {
Vec3f gSfxDefaultPos{};
void Audio_PlaySfx_AtPosWithAllChannelsIO(Vec3f *, u16, u8) { ++chimeCount; }
}
static void EnNwc_ChangeState(EnNwc *self, s16 state) {
  self->state = state;
  ++followCount;
}
#include "mm_chicks.inc"
static int spawnReachesAllocation=0;
#include "mm_spawn.inc"
int main(){
    PlayState play{};
    // Existing native chicks remain safe and cannot start the march without
    // Grog.
    Player player{};
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    player.stateFlags3 = PLAYER_STATE3_20000000;
    EnNwc chick{};
    chick.grog = EnNwc_FindGrog(&play);
    REQUIRE(chick.grog == nullptr);
    EnNwc before = chick;
    EnNwc_CheckFound(&chick, &play);
    REQUIRE(followCount == 0 && chimeCount == 0 &&
            std::memcmp(&chick, &before, sizeof(chick)) == 0);
    EnHs grog{};
    grog.actor.id = ACTOR_EN_HS;
    play.actorCtx.actorLists[ACTORCAT_NPC].first = &grog.actor;
    chick.grog = EnNwc_FindGrog(&play);
    REQUIRE(chick.grog == &grog);
    player.stateFlags3 = 0;
    EnNwc_CheckFound(&chick, &play);
    REQUIRE(followCount == 0);
    player.stateFlags3 = PLAYER_STATE3_20000000;
    chick.actor.xzDistToPlayer = 100;
    EnNwc_CheckFound(&chick, &play);
    REQUIRE(followCount == 0);
    chick.actor.xzDistToPlayer = 0;
    EnNwc_CheckFound(&chick, &play);
    REQUIRE(followCount == 1 && chimeCount == 1 && grog.actor.home.rot.z == 2);
    play.actorCtx.actorLists[ACTORCAT_NPC].first = nullptr;

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
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] = 1;
    play.sceneId = SCENE_F01C;
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] = 0;
    REQUIRE(MM_SoulCanSpawnNonEnemy(&play, ACTOR_EN_HS, 0xFE01));
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] = 1;
    REQUIRE(!MM_SoulCanSpawnNonEnemy(&play, ACTOR_EN_HS, 0xFE01));
    Actor_SpawnAsChildAndCutscene(&play.actorCtx, &play, ACTOR_EN_HS, 0, 0, 0,
                                  0, 0, 0, (s16)0xFE01, 0, 0, nullptr);
    REQUIRE(spawnReachesAllocation == 0);
    REQUIRE(MM_SoulCanSpawnNonEnemy(&play, ACTOR_EN_HS,
                                    1)); // unused alternate role
    Own(RI_SOUL_NPC_GROG);
    REQUIRE(MM_SoulCanSpawnNonEnemy(&play, ACTOR_EN_HS, 0xFE01));
    Actor_SpawnAsChildAndCutscene(&play.actorCtx, &play, ACTOR_EN_HS, 0, 0, 0,
                                  0, 0, 0, (s16)0xFE01, 0, 0, nullptr);
    REQUIRE(spawnReachesAllocation == 1);
    spawnReachesAllocation = 0;
    play.sceneId = SCENE_F01_B;
    REQUIRE(!MM_SoulCanSpawnNonEnemy(&play, ACTOR_EN_AOB_01, 0xFFFF));
    Own(RI_SOUL_NPC_DOG_LADY);
    REQUIRE(MM_SoulCanSpawnNonEnemy(&play, ACTOR_EN_AOB_01, 0xFFFF));
    play.sceneId = SCENE_8ITEMSHOP;
    REQUIRE(MM_SoulCanSpawnNonEnemy(&play, ACTOR_EN_HS, 0xFE01));
    REQUIRE(MM_SoulCanSpawnNonEnemy(&play, ACTOR_EN_AOB_01, 0xFFFF));
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] = 0;
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
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] = 1;
    Own(RI_SOUL_NPC_DOG_LADY, false);
    REQUIRE(!MM_SoulCanSpawnNonEnemy(&play, ACTOR_EN_AOB_01, 0xFFFF));
    Own(RI_SOUL_NPC_DOG_LADY);
    REQUIRE(MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_AOB_01,0));
    play.sceneId=SCENE_F01C;
    Own(RI_SOUL_NPC_GROG, false);
    REQUIRE(!MM_SoulCanSpawnNonEnemy(&play, ACTOR_EN_HS, 0xFE01));
    Own(RI_SOUL_NPC_GROG);
    REQUIRE(MM_SoulCanSpawnNonEnemy(&play, ACTOR_EN_HS, 0xFE01));
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] = 0;
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
