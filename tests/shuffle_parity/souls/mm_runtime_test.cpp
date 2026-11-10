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
static int bottleSpawns=0,bottleConsumed=0;static bool releaseAllocationFails=false;static Actor releasedActor{};
extern "C" {
f32 Math_SinS(s16){return 0;} f32 Math_CosS(s16){return 1;}
Actor* Actor_Spawn(ActorContext*,PlayState*,s16,f32,f32,f32,s16,s16,s16,s32){++bottleSpawns;return releaseAllocationFails?nullptr:&releasedActor;}
void Player_UpdateBottleHeld(PlayState*,Player*,ItemId,PlayerItemAction){++bottleConsumed;}
}
static void ReleaseBottle(PlayState*play,Player*self,s16 actorId,s16 params){
 struct {s16 actorId,params;} data{actorId,params};auto*sp4C=&data;
#include "mm_bottle.inc"
}
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

    // Actual native bottle-release statements: both ownership gates precede consume.
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS]=1;play.sceneId=SCENE_22DEKUCITY;
    ReleaseBottle(&play,&player,ACTOR_EN_DNP,1);REQUIRE(bottleSpawns==0&&bottleConsumed==0);
    Own(RI_SOUL_NPC_DEKU_PRINCESS);ReleaseBottle(&play,&player,ACTOR_EN_DNP,1);
    REQUIRE(bottleSpawns==0&&bottleConsumed==0);
    Own(RI_SOUL_NPC_DEKU_KING);releaseAllocationFails=true;ReleaseBottle(&play,&player,ACTOR_EN_DNP,1);
    REQUIRE(bottleSpawns==1&&bottleConsumed==0);
    releaseAllocationFails=false;ReleaseBottle(&play,&player,ACTOR_EN_DNP,1);
    REQUIRE(bottleSpawns==2&&bottleConsumed==1);
    Own(RI_SOUL_NPC_DEKU_PRINCESS,false);play.sceneId=SCENE_00KEIKOKU;
    ReleaseBottle(&play,&player,ACTOR_EN_DNP,1);REQUIRE(bottleConsumed==1);
    ReleaseBottle(&play,&player,ACTOR_EN_DNP,3);REQUIRE(bottleConsumed==2); // unknown/custom role
    ReleaseBottle(&play,&player,ACTOR_EN_FISH,0);REQUIRE(bottleConsumed==3);
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS]=0;
    ReleaseBottle(&play,&player,ACTOR_EN_DNP,1);REQUIRE(bottleConsumed==4);
    Own(RI_SOUL_NPC_DEKU_KING,false);
    // Native Moon child roles include running copies through type 8; type 9 is not a child role.
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS]=1;play.sceneId=SCENE_SOUGEN;
    REQUIRE(!MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_JS,8));
    REQUIRE(MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_JS,9));
    Own(RI_SOUL_NPC_MOON_CHILDREN);REQUIRE(MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_JS,8));Own(RI_SOUL_NPC_MOON_CHILDREN,false);
    // Ranch quest roles require Romani/Cremia; aliens and UFO keep their enemy path.
    play.sceneId=SCENE_F01;
    for(int type:{0,5,7,9,11,12}) REQUIRE(!MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_INVADEPOH,type<<4));
    for(int type:{1,2,3,4,6,8,10,13}) REQUIRE(MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_INVADEPOH,type<<4));
    Own(RI_SOUL_NPC_MALON);REQUIRE(MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_INVADEPOH,0));Own(RI_SOUL_NPC_MALON,false);
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS]=0;
    // Literal independent native role expectations, not generated from the runtime map.
    struct ExpectedNpc { int actor; int scene; unsigned params; RandoItemId soul; };
    const ExpectedNpc expectedNpcs[] = {
#include "mm_npc_roles.inc"
    };
    for (const auto& role : expectedNpcs) {
        play.sceneId = role.scene;
        RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] = 1;
        REQUIRE(!MM_SoulCanInteract(&play, role.actor, role.params));
        Own(role.soul, true);
        REQUIRE(MM_SoulCanInteract(&play, role.actor, role.params));
        Own(role.soul, false);
        RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] = 0;
        REQUIRE(MM_SoulCanInteract(&play, role.actor, role.params));
        play.sceneId = 0x7FFF;
        RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] = 1;
        REQUIRE(MM_SoulCanInteract(&play, role.actor, role.params));
        RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] = 0;
    }

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
    struct NpcRole { int actor; int scene; unsigned params; RandoItemId soul; };
    const NpcRole roles[] = {
        {ACTOR_EN_SYATEKI_MAN, SCENE_SYATEKI_MORI, 0xFE00, RI_SOUL_NPC_BAZAAR_SHOPKEEPER},
        {ACTOR_EN_SYATEKI_MAN, SCENE_SYATEKI_MIZU, 0xFE00, RI_SOUL_NPC_SHOOTING_GALLERY_OWNER},
        {ACTOR_EN_GINKO_MAN, SCENE_ICHIBA, 0xFFFF, RI_SOUL_NPC_BANKER},
        {ACTOR_EN_GURUGURU, SCENE_ALLEY, 1, RI_SOUL_NPC_GURU_GURU},
        {ACTOR_EN_GURUGURU, SCENE_YADOYA, 0, RI_SOUL_NPC_GURU_GURU},
        {ACTOR_EN_NB, SCENE_YADOYA, 0xFFFF, RI_SOUL_NPC_OLD_HAG},
        {ACTOR_EN_NB, SCENE_OMOYA, 0xFFFF, RI_SOUL_NPC_OLD_HAG},
        {ACTOR_EN_KENDO_JS, SCENE_DOUJOU, 0xFE00, RI_SOUL_NPC_CARPET_MAN},
        {ACTOR_EN_KENDO_JS, SCENE_DOUJOU, 0xFE01, RI_SOUL_NPC_CARPET_MAN},
    };
    for (const auto& role : roles) {
        play.sceneId = role.scene;
        RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] = 0;
        REQUIRE(MM_SoulCanSpawnNonEnemy(&play, role.actor, role.params));
        RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] = 1;
        REQUIRE(!MM_SoulCanSpawnNonEnemy(&play, role.actor, role.params));
        Actor_SpawnAsChildAndCutscene(&play.actorCtx, &play, role.actor, 0, 0, 0,
                                     0, 0, 0, (s16)role.params, 0, 0, nullptr);
        REQUIRE(spawnReachesAllocation == 0);
        gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_VANILLA;
        REQUIRE(MM_SoulCanSpawnNonEnemy(&play, role.actor, role.params));
        gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
        gSaveContext.gameMode = GAMEMODE_END_CREDITS;
        REQUIRE(MM_SoulCanSpawnNonEnemy(&play, role.actor, role.params));
        gSaveContext.gameMode = GAMEMODE_NORMAL;
        Own(role.soul);
        REQUIRE(MM_SoulCanSpawnNonEnemy(&play, role.actor, role.params));
        Actor_SpawnAsChildAndCutscene(&play.actorCtx, &play, role.actor, 0, 0, 0,
                                     0, 0, 0, (s16)role.params, 0, 0, nullptr);
        REQUIRE(spawnReachesAllocation == 1);
        spawnReachesAllocation = 0;
        Own(role.soul, false);
        if (role.actor == ACTOR_EN_GURUGURU || role.actor == ACTOR_EN_KENDO_JS) {
            REQUIRE(MM_SoulCanSpawnNonEnemy(&play, role.actor, 2));
            REQUIRE(MM_SoulCanSpawnNonEnemy(&play, role.actor, 0xFF));
        }
        play.sceneId = 0x7FFF;
        REQUIRE(MM_SoulCanSpawnNonEnemy(&play, role.actor, role.params));
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
    return 0;
}
