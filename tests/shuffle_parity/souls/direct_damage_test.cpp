#define main collision_suite_main
#include "runtime_test.cpp"
#undef main
static int kills, finishers, heals, broadcasts, drops, sounds;
static bool enabled = true;
static struct { int currentExtShield = 3; } gExtEquipState;
static bool ExtEquip_IsEnabled() { return enabled; }
static int sIkanaGuardTimer=0, sIkanaGuardActive=1;
#define IKANA_PERFECT_GUARD_WINDOW 12
#define IKANA_SOUL_DRAIN_DAMAGE 4
#define SHOVEL_HOLE_RADIUS 40.0f
struct ShovelHole { Actor actor; s16 lifetime; f32 scale; };
PlayState* gPlayState;
static void DinFireSword_FinishFatal(PlayState*, Actor*) { ++finishers; }
extern "C" {
f32 gSfxDefaultFreqAndVolScale; s8 gSfxDefaultReverb;
void Actor_Kill(Actor*) { ++kills; }
void Enemy_StartFinishingBlow(PlayState*, Actor*) { ++finishers; }
void Item_DropCollectibleRandom(PlayState*, Actor*, Vec3f*, s16) { ++drops; }
void Audio_PlaySfxGeneral(u16, Vec3f*, u8, f32*, f32*, s8*) { ++sounds; }
void Audio_PlayActorSound2(Actor*,u16) { ++sounds; }
s32 Health_ChangeBy(PlayState*,s16) { ++heals; return 1; }
f32 Math_Vec3f_DistXYZ(Vec3f*,Vec3f*) { return 0; }
}
void HarpoonCombat_BroadcastShieldParry_C(int,int) { ++broadcasts; }
#include "damage.inc"
int main() {
    PlayState play{}; gPlayState=&play;
    Player player{}; Actor enemy{};enemy.id=ACTOR_EN_TEST;enemy.category=ACTORCAT_ENEMY;
    enemy.update=Update;enemy.colChkInfo.health=3;enemy.colChkInfo.damage=4;
    gSaveContext.ship.quest.id=QUEST_RANDOMIZER;
    optionValues[RSK_SHUFFLE_ENEMY_SOULS]=1;
    REQUIRE(Actor_ApplyDamage(&enemy)==3);
    REQUIRE(enemy.colChkInfo.health==3 && finishers==0);
    player.shieldQuad.base.ac=&enemy;
    Ikana_OnShieldBlock(&player,&play);
    REQUIRE(enemy.colChkInfo.health==3 && kills==0 && heals==0 && broadcasts==0);
    Own(RG_SOUL_ENEMY_STALFOS,true);
    REQUIRE(Actor_ApplyDamage(&enemy)==0 && finishers==1);
    enemy.colChkInfo.health=3;
    Ikana_OnShieldBlock(&player,&play);
    REQUIRE(enemy.colChkInfo.health==0 && kills==1 && drops==1 && heals==1 && broadcasts==1);
    enemy.colChkInfo.health=10;
    sIkanaGuardTimer=IKANA_PERFECT_GUARD_WINDOW+1;
    Ikana_OnShieldBlock(&player,&play);
    REQUIRE(enemy.colChkInfo.health==10 && heals==1);
    sIkanaGuardTimer=0;
    enemy.id=ACTOR_EN_REEBA;enemy.colChkInfo.health=32;
    play.actorCtx.actorLists[ACTORCAT_ENEMY].head=&enemy;
    ShovelHole hole{};hole.lifetime=5;
    ShovelHole_Update(&hole.actor,&play);
    REQUIRE(enemy.colChkInfo.health==32 && hole.lifetime==4);
    Own(RG_SOUL_ENEMY_LEEVER,true);
    ShovelHole_Update(&hole.actor,&play);
    REQUIRE(enemy.colChkInfo.health==16 && hole.lifetime==3);
    for (int actorId : { ACTOR_EN_DEKUNUTS, ACTOR_EN_HINTNUTS }) {
        enemy.id=actorId;enemy.colChkInfo.health=32;hole.lifetime=10;
        ShovelHole_Update(&hole.actor,&play);
        REQUIRE(enemy.colChkInfo.health==32);
        Own(RG_SOUL_ENEMY_DEKU_SCRUB,true);
        ShovelHole_Update(&hole.actor,&play);
        REQUIRE(enemy.colChkInfo.health==16);
        Own(RG_SOUL_ENEMY_DEKU_SCRUB,false);
    }
    enemy.id=ACTOR_EN_REEBA;
    Own(RG_SOUL_ENEMY_LEEVER,false);
    optionValues[RSK_SHUFFLE_ENEMY_SOULS]=0;
    enemy.colChkInfo.health=32;
    ShovelHole_Update(&hole.actor,&play);
    REQUIRE(enemy.colChkInfo.health==16);
    enemy.id=ACTOR_EN_TEST;enemy.colChkInfo.health=10;
    Ikana_OnShieldBlock(&player,&play);
    REQUIRE(enemy.colChkInfo.health==6);
    optionValues[RSK_SHUFFLE_ENEMY_SOULS]=1;Own(RG_SOUL_ENEMY_STALFOS,false);
    gSaveContext.ship.quest.id=QUEST_NORMAL;
    REQUIRE(Actor_ApplyDamage(&enemy)==2);
    puts("PASS actual direct damage bodies: ApplyDamage, Ikana shield and Shovel, owned/off/vanilla controls");
}
