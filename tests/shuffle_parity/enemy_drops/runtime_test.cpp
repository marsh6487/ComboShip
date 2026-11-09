// Engine allocation, item lookup and hook registration are the deliberate
// boundaries. Actor, PlayState, SaveContext and all production callback bodies
// below are the real native types/code.
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#include "z64.h"
#include "functions.h"
#include "variables.h"
#include "overlays/actors/ovl_En_Goma/z_en_goma.h"
#include "overlays/actors/ovl_En_GeldB/z_en_geldb.h"
#include <cstdio>
#include <cstdlib>
#define REQUIRE(expr) do {if(!(expr)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#expr);abort();}}while(0)
#include <functional>
#include <cstring>
static std::function<void(void*)> defeat;
static std::function<void()> playerUpdate;
static std::function<void(void*)> destroyed;
static std::function<void(int16_t)> scene;
template<class F> void Register_OnActorDestroy(bool enabled,F callback) {destroyed=enabled?std::function<void(void*)>(callback):nullptr;}
template<class F> void Register_OnPlayerUpdate(bool enabled,F callback) {playerUpdate=enabled?std::function<void()>(callback):nullptr;}
template<class F> void Register_OnEnemyDefeat(bool enabled,F callback) {defeat=enabled?std::function<void(void*)>(callback):nullptr;}
template<class F> void Register_OnSceneInit(bool enabled,F callback) {scene=enabled?std::function<void(int16_t)>(callback):nullptr;}
#undef COND_HOOK
#define COND_HOOK(hook, enabled, callback) Register_##hook(enabled, callback)
#include "soh/soh/Enhancements/randomizer/EnemyFirstDropsRuntime.cpp"
extern "C" {
void EnGoma_SetupDead(EnGoma*);
void EnGeldB_SetupDefeated(EnGeldB*);
void EnGoma_Dead(EnGoma*,PlayState*) {}
void EnGeldB_Defeated(EnGeldB*,PlayState*) {}
void EnGeldB_SetupAction(EnGeldB*, EnGeldBActionFunc) {}
void DinFireSword_FinishFatal(PlayState*, Actor*) {}
void GameInteractor_ExecuteOnActorKill(void*) {}
void GameInteractor_ExecuteOnEnemyDefeat(void* actor) {if(defeat)defeat(actor);}
void Animation_Change(SkelAnime*,AnimationHeader*,f32,f32,f32,u8,f32) {}
s16 Animation_GetLastFrame(void*) {return 20;}
void Animation_MorphToPlayOnce(SkelAnime*,AnimationHeader*,f32) {}
void Audio_PlayActorSound2(Actor*,u16) {}
SaveContext gSaveContext;
PlayState play;
PlayState* gPlayState=&play;
static bool enabled;
static bool allocate=true;
static EnItem00 item;
static int spawned;
u8 Randomizer_GetSettingValue(RandomizerSettingKey key) {return key==RSK_SHUFFLE_ENEMY_DROPS&&enabled;}
GetItemEntry Randomizer_GetItemFromKnownCheck(RandomizerCheck check, GetItemID original) {
    REQUIRE(original==GI_NONE); GetItemEntry result={};result.itemId=check;return result;
}
EnItem00* Item_DropCollectible2(PlayState*,Vec3f* position,s16 params) {
    REQUIRE(params==ITEM00_SOH_DUMMY);if(!allocate)return nullptr;
    ++spawned;item={};item.actor.world.pos=*position;return &item;
}
void lusprintf(const char*,int32_t,int32_t,const char*,...) {}
void GameInteractor_ExecuteOnFlagSet(FlagType, uint32_t) {}
#include "native_save_flags.inc"
}
void EnItem00_DrawRandomizedItem(EnItem00*,PlayState*) {}
int main() {
    using namespace Rando::EnemyFirstDrops;
    gSaveContext.ship.quest.id=QUEST_RANDOMIZER;gSaveContext.linkAge=LINK_AGE_ADULT;
    Actor actor={};actor.id=ACTOR_EN_TEST;actor.world.pos={4,5,6};
    enabled=false; RegisterRuntime();REQUIRE(!defeat);REQUIRE(!OnDefeat(&actor));
    enabled=true;RegisterRuntime();REQUIRE(defeat);REQUIRE(scene);
    defeat(&actor);REQUIRE(spawned==1);REQUIRE(item.randoInf==RAND_INF_ENEMY_DROP_STALFOS);
    REQUIRE(item.itemEntry.itemId==RC_ENEMY_DROP_STALFOS);REQUIRE(item.actor.draw==(ActorFunc)EnItem00_DrawRandomizedItem);
    REQUIRE(item.actor.world.pos.x==4&&item.actor.world.pos.y==5&&item.actor.world.pos.z==6);
    // Repeated callbacks and a second actor of the same type share one pending check.
    defeat(&actor);actor.params=1;defeat(&actor);REQUIRE(spawned==1);
    // The actual existing receipt flag survives both a scene reset and reload.
    Flags_SetRandomizerInf(item.randoInf);scene(0);defeat(&actor);REQUIRE(spawned==1);
    auto saved=gSaveContext.ship.randomizerInf[RAND_INF_ENEMY_DROP_STALFOS>>4];
    memset(gSaveContext.ship.randomizerInf,0,sizeof(gSaveContext.ship.randomizerInf));
    gSaveContext.ship.randomizerInf[RAND_INF_ENEMY_DROP_STALFOS>>4]=saved;
    RegisterRuntime();defeat(&actor);REQUIRE(spawned==1);
    // Uncollected drops may be tried again after leaving; allocation failures also retry.
    actor.id=ACTOR_EN_FIREFLY;scene(0);allocate=false;defeat(&actor);REQUIRE(spawned==1);
    allocate=true;defeat(&actor);REQUIRE(spawned==2);scene(0);defeat(&actor);REQUIRE(spawned==3);
    // Room cleanup never creates a reward; a lost uncollected drop can retry.
    int countBeforeCleanup=spawned;destroyed(&actor);REQUIRE(spawned==countBeforeCleanup);
    destroyed(&item.actor);defeat(&actor);REQUIRE(spawned==countBeforeCleanup+1);
    // Native special victories (Dark Link and Gerudo) use the same confirmed callback.
    actor.id=ACTOR_EN_TORCH2;defeat(&actor);REQUIRE(item.randoInf==RAND_INF_ENEMY_DROP_DARK_LINK);
    actor.id=ACTOR_EN_GELDB;defeat(&actor);REQUIRE(item.randoInf==RAND_INF_ENEMY_DROP_THIEVES);
    // Execute the actual native ordinary-death and special-victory functions.
    EnGoma goma={};goma.actor.id=ACTOR_EN_GOMA;play.sceneNum=SCENE_DEKU_TREE;
    EnGoma_SetupDead(&goma);REQUIRE(item.randoInf==RAND_INF_ENEMY_DROP_GOHMA_LARVA);
    scene(0);EnGeldB gerudo={};gerudo.actor.id=ACTOR_EN_GELDB;
    EnGeldB_SetupDefeated(&gerudo);REQUIRE(item.randoInf==RAND_INF_ENEMY_DROP_THIEVES);
    int before=spawned;
    Actor unloading={};unloading.id=ACTOR_EN_TITE;Actor_Kill(&unloading);REQUIRE(spawned==before);
    actor.id=ACTOR_EN_SW;actor.params=0x2000;defeat(&actor);REQUIRE(spawned==before); // gold skull
    actor.id=ACTOR_EN_AM;actor.params=0;defeat(&actor);REQUIRE(spawned==before); // inert statue
    actor.id=ACTOR_EN_POH;actor.params=2;defeat(&actor);REQUIRE(spawned==before); // composer
    actor.id=ACTOR_EN_GOMA;play.sceneNum=SCENE_DEKU_TREE_BOSS;defeat(&actor);REQUIRE(spawned==before);
    actor.id=ACTOR_EN_OSSAN;defeat(&actor);REQUIRE(spawned==before); // custom viewer/shop roles
    enabled=false;actor.id=ACTOR_EN_TITE;REQUIRE(!OnDefeat(&actor));REQUIRE(spawned==before);
    enabled=true;gSaveContext.ship.quest.id=QUEST_NORMAL;
    REQUIRE(!OnDefeat(&actor));REQUIRE(spawned==before);
    // A unique special victory must remain deliverable after scene/save reload
    // without defeating its now-absent actor a second time.
    gSaveContext.ship.quest.id=QUEST_RANDOMIZER;RegisterRuntime();
    Actor playerActor={};playerActor.world.pos={90,80,70};
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head=&playerActor;
    u16 savedEligibility[ARRAY_COUNT(gSaveContext.ship.randomizerInf)];
    memcpy(savedEligibility,gSaveContext.ship.randomizerInf,sizeof(savedEligibility));
    memset(gSaveContext.ship.randomizerInf,0,sizeof(gSaveContext.ship.randomizerInf));
    int beforeRecovery=spawned;if(playerUpdate)playerUpdate();REQUIRE(spawned==beforeRecovery);
    memcpy(gSaveContext.ship.randomizerInf,savedEligibility,sizeof(savedEligibility));
    if(playerUpdate)playerUpdate();
    REQUIRE(spawned>beforeRecovery);
    int afterRecovery=spawned;playerUpdate();REQUIRE(spawned==afterRecovery);
    REQUIRE(item.actor.world.pos.x==90&&item.actor.world.pos.y==80&&item.actor.world.pos.z==70);
    // Every delivered type has exactly one durable receipt; an obtained flag
    // ends redelivery even when the corresponding actor no longer exists.
    for(const auto& entry:Catalog)Flags_SetRandomizerInf(entry.flag);
    ResetPending();playerUpdate();REQUIRE(spawned==afterRecovery);
    puts("PASS native first-drop callback/receipt/role/allocation/default-off regression");
}
