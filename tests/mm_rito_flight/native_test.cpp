#include "PR/os.h"
// libultra also exports BTN_*; let the real MM controller header own these names.
#undef BTN_A
#undef BTN_B
#undef BTN_Z
#undef BTN_START
#undef BTN_DUP
#undef BTN_DDOWN
#undef BTN_DLEFT
#undef BTN_DRIGHT
#undef BTN_L
#undef BTN_R
#undef BTN_CUP
#undef BTN_CDOWN
#undef BTN_CLEFT
#undef BTN_CRIGHT
#include "global.h"
#include "mods/nei_oot_compat.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <initializer_list>

#define PLAYER_STATE3_PAUSE_ACTION_FUNC PLAYER_STATE3_4
#define FORMS_STATE1_DAMAGED PLAYER_STATE1_4000000
#define FORMS_STATE1_CLIMBING_LEDGE PLAYER_STATE1_4
#define FORMS_STATE1_GETTING_ITEM PLAYER_STATE1_400
#define FORMS_STATE1_LOADING PLAYER_STATE1_200
#define CUSTOM_FORM_RITO 4

SaveContext gSaveContext;
PlayState* gPlayState;
s32 D_801ED8B4;
static Input raw;
static int active = CUSTOM_FORM_RITO, errors, animationChanges, animationTicks, lands, actionChanges;
static int bowOwns, bowTicks, bowEnds, restores, windTicks, windStarts, windClears, windStops;
static f32 raycastHeight;
static CollisionPoly floorPoly;
static PlayerAnimationHeader chargeClip{}, launchClip{}, flyClip{}, landClip{}, hopClip{};
static PlayerAnimationHeader* lastAnimation;
static f32 lastSpeed, lastStart, lastEnd;
static u8 lastMode;
static struct { s16 lastHealth; u8 hurt; } sRitoDamage;
static struct { u8 state; PlayerActionFunc action; } sRitoBow;
#define RITO_BOW_OFF 0
static u8 sRitoWindOn;
static PlayState* sRitoPlay;
static Player* sRitoPlayer;
static u32 sRitoLastFrame;

int CustomForms_ActiveForm() { return active; }
const Input* Forms_RawInput() { return &raw; }
u8 Forms_IsAdultAge() { return 0; }
f32 Forms_StickMagnitude(const Input* input) {
    return std::hypot(input->rel.stick_x, input->rel.stick_y);
}
extern "C" f32 BgCheck_EntityRaycastFloor5_2(PlayState*, CollisionContext*, CollisionPoly** out,
                                            s32* bg, Actor*, Vec3f*) {
    *out = raycastHeight > BGCHECK_Y_MIN ? &floorPoly : nullptr;
    *bg = BGCHECK_SCENE;
    return raycastHeight;
}
extern "C" void DynaPolyActor_AttachCarriedActor(CollisionContext*, Actor*, s32) {}
extern "C" void Player_Action_Idle(Player*, PlayState*) {}
extern "C" void Player_SetAction(PlayState*, Player* p, PlayerActionFunc action, s32) {
    ++actionChanges;
    p->actionFunc = action;
}
extern "C" s32 Camera_ChangeMode(Camera*, s16 mode) { assert(mode == CAM_MODE_JUMP); return 0; }
extern "C" void Player_PlaySfx(Player*, u16 id) { if (id == NA_SE_PL_LAND) ++lands; }
extern "C" void Audio_PlaySfx(u16) { ++errors; }
extern "C" s16 Animation_GetLastFrame(void*) { return 12; }
extern "C" void PlayerAnimation_Change(PlayState*, SkelAnime* skel, PlayerAnimationHeader* anim,
                                         f32 speed, f32 start, f32 end, u8 mode, f32 morph) {
    ++animationChanges;
    lastAnimation = anim;
    skel->animation = anim;
    lastSpeed = speed; lastStart = start; lastEnd = end; lastMode = mode;
    assert(morph == -4.0f);
}
extern "C" s32 PlayerAnimation_Update(PlayState*, SkelAnime*) { ++animationTicks; return 0; }
static void MmRitoLoadClips() {} // actual resource loading is a separate engine boundary
static void MmRitoRestoreLanding() { ++restores; }
static void MmRitoWindClear() { sRitoWindOn = 0; ++windClears; }
static void MmRitoWindTrailsOff(PlayState*) { sRitoWindOn = 0; ++windStops; }
static void MmRitoWindTrailsOn(PlayState*, Player*) { sRitoWindOn = 1; ++windStarts; }
static void MmRitoWindTick(PlayState*) { ++windTicks; }
static void MmRitoBowReset() { sRitoBow = {}; }
static void MmRitoBowEnd(Player*) { ++bowEnds; sRitoBow.state = RITO_BOW_OFF; }
static u8 MmRitoBowIsOut() { return sRitoBow.state != RITO_BOW_OFF; }
// Exercise the production dispatch boundary. Actual bow state transitions/volley
// decisions remain covered by tests/rito_mask_mm/run_controller_tests.py.
static u8 MmRitoBowUpdate(Player*, PlayState*) { ++bowTicks; return bowOwns; }
static u8 RitoForm_OwnsAction();
#include "production.inc"

static PlayState play;
static Player player;
static Camera camera;
static void fresh() {
    play = {}; player = {}; raw = {}; sRito = {}; sRitoBow = {}; sRitoDamage = {};
    sRitoPlay = &play; sRitoPlayer = &player; sRitoLastFrame = 0;
    sRitoWindOn = 0; bowOwns = 0; active = CUSTOM_FORM_RITO;
    gPlayState = &play;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    play.cameraPtrs[0] = &camera;
    player.transformation = PLAYER_FORM_HUMAN;
    player.actor.world.pos.y = 100.0f;
    player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    player.actor.gravity = -1.2f;
    player.actionFunc = Player_Action_Idle;
    sRito.charge_ = &chargeClip; sRito.launch = &launchClip;
    sRito.fly = &flyClip; sRito.land = &landClip;
    sRito.hopSettle = &hopClip;
    for (auto& clip : sRito.hopClips) clip = &hopClip;
    gSaveContext.magicCapacity = 48;
    gSaveContext.save.saveInfo.playerData.magic = 40;
    animationChanges = animationTicks = actionChanges = lands = errors = 0;
}
static void floorAt(f32 height) {
    raycastHeight = height;
    Vec3f ray = player.actor.world.pos;
    func_800B7678(&play, &player.actor, &ray, UPDBGCHECKINFO_FLAG_4 | UPDBGCHECKINFO_FLAG_10);
}
static void tick() {
    Input filtered = raw;
    RitoForm_FilterInput(&player, &filtered);
    RitoForm_Update(&player, &play);
    ++play.gameplayFrames;
    raw.press.button = 0;
}
static void heldAir(f32 height) {
    fresh();
    raw.cur.button = BTN_A; // the A press happened while still standing/walking
    floorAt(100.0f); tick();
    assert(sRito.state == RITO_FLY_OFF && animationChanges == 0);
    player.actor.velocity.y = -3.0f;
    floorAt(height);
    assert(!(player.actor.bgCheckFlags & BGCHECKFLAG_GROUND));
    assert(player.actor.bgCheckFlags & BGCHECKFLAG_GROUND_LEAVE);
    assert(player.actor.floorHeight == height);
    tick();
    if (sRito.state != RITO_FLY_GLIDE) {
        std::fprintf(stderr, "FAIL held A after leaving floor (%g): expected GLIDE, got %u\n", height, sRito.state);
        std::exit(1);
    }
    assert(lastAnimation == &flyClip && lastMode == ANIMMODE_LOOP);
    assert(lastSpeed == 1.0f && lastStart == 0.0f && lastEnd == 12.0f);
    tick();
    assert(player.speedXZ == 4.5f && player.actor.speed == player.speedXZ);
    assert(player.actor.gravity == 0 && player.actor.velocity.y == -0.35f);
    assert(player.stateFlags3 & PLAYER_STATE3_PAUSE_ACTION_FUNC);
    assert(gSaveContext.save.saveInfo.playerData.magic == 40);
    raw.cur.button = 0; tick();
    assert(sRito.state == RITO_FLY_OFF && player.actor.gravity == -1.2f);
    assert(!(player.stateFlags3 & (PLAYER_STATE3_MIDAIR | PLAYER_STATE3_PAUSE_ACTION_FUNC)));
    assert(!(player.stateFlags1 & PLAYER_STATE1_JUMPING));
}
static void poisonFlight() {
    sRito.state = RITO_FLY_GLIDE; sRito.charge = 19; sRito.boost = 5;
    sRito.windT = 12; sRito.hopArmed = 1; sRito.magicTimer = 8; sRito.timer = 22;
    sRito.yaw = 333; sRito.flyAction = Player_Action_Idle; sRitoDamage = { 80, 1 };
    sRito.loaded = 1;
}
static void assertForgotten() {
    if (sRito.state || sRito.charge || sRito.boost || sRito.windT || sRito.hopArmed ||
        sRito.magicTimer || sRito.timer || sRito.yaw || sRito.flyAction ||
        sRitoDamage.lastHealth || sRitoDamage.hurt) {
        std::fputs("FAIL session reset retained flight counters/damage from departed session\n", stderr);
        std::exit(1);
    }
    assert(sRito.loaded && sRito.fly == &flyClip && sRito.launch == &launchClip && sRito.land == &landClip);
}
static void resets() {
    fresh(); poisonFlight(); RitoForm_ForgetContext(); assertForgotten();
    fresh(); poisonFlight(); RitoForm_Reset(&player, &play); assertForgotten();
    assert(player.actor.gravity == -1.2f);
    fresh(); poisonFlight();
    PlayState otherPlay{}; Player otherPlayer{};
    const int beforeStops = windStops;
    RitoForm_SyncContext(&otherPlayer, &otherPlay); assertForgotten();
    assert(windStops == beforeStops); // replacement forgets an old arena without killing it
    fresh(); poisonFlight(); sRitoLastFrame = 100; play.gameplayFrames = 0;
    RitoForm_SyncContext(&player, &play); assertForgotten();
    fresh(); poisonFlight(); RitoForm_SyncContext(&otherPlayer, &play); assertForgotten();
    std::puts("PASS MM native flight reset: form exit, scene/player replacement and frame rollback; clips retained, departed arena untouched");
}
int main(int argc, char** argv) {
    if (argc > 1 && std::strcmp(argv[1], "reset") == 0) { resets(); return 0; }
    heldAir(BGCHECK_Y_MIN);
    heldAir(-1000.0f); // a distant raycast hit is still airborne
    fresh(); floorAt(BGCHECK_Y_MIN); tick();
    assert(!RitoForm_OwnsAction() && animationChanges == 0); // no A: ordinary fall
    raw.cur.button = raw.press.button = BTN_A; tick();
    assert(sRito.state == RITO_FLY_GLIDE); // fresh A midair also works
    floorAt(100.0f); tick();
    assert(sRito.state == RITO_FLY_OFF && lands == 1 && player.actor.gravity == -1.2f);

    fresh(); raw.cur.button = raw.press.button = BTN_A; tick();
    assert(sRito.state == RITO_FLY_CHARGE && lastAnimation == &chargeClip);
    for (int i = 0; i < 8; ++i) tick();
    raw.cur.button = 0; tick();
    assert(sRito.state == RITO_FLY_LAUNCH && lastAnimation == &launchClip);
    assert(gSaveContext.save.saveInfo.playerData.magic == 28 && player.actor.velocity.y > 8);
    raw.cur.button = BTN_A;
    for (int i = 0; i < 30; ++i) tick();
    assert(sRito.state == RITO_FLY_GLIDE && lastAnimation == &flyClip);
    assert(gSaveContext.save.saveInfo.playerData.magic == 28);

    fresh(); raw.cur.button = raw.press.button = BTN_A; tick();
    raw.cur.button = 0; tick();
    assert(sRito.state == RITO_FLY_OFF && gSaveContext.save.saveInfo.playerData.magic == 40);
    fresh(); raw.cur.button = raw.press.button = BTN_A; tick();
    for (int i = 0; i < 8; ++i) tick();
    gSaveContext.save.saveInfo.playerData.magic = 11; raw.cur.button = 0; tick();
    assert(sRito.state == RITO_FLY_OFF && errors == 1 && gSaveContext.save.saveInfo.playerData.magic == 11);

    fresh(); raw.cur.button = raw.press.button = BTN_A; raw.rel.stick_x = 20; tick();
    assert(sRito.state == RITO_FLY_HOP && !sRito.hopArmed && lastAnimation == &hopClip);
    tick(); assert(sRito.state == RITO_FLY_HOP); // held takeoff A cannot instantly glide
    raw.cur.button = 0; tick(); assert(sRito.hopArmed);
    raw.cur.button = raw.press.button = BTN_A; tick(); assert(sRito.state == RITO_FLY_GLIDE);

    fresh(); floorAt(BGCHECK_Y_MIN); raw.cur.button = raw.press.button = BTN_A | BTN_B;
    bowOwns = 1; tick(); assert(sRito.state == RITO_FLY_OFF && animationChanges == 0);
    bowOwns = 0; raw.cur.button = BTN_A; tick(); assert(sRito.state == RITO_FLY_GLIDE);
    fresh(); raw.cur.button = raw.press.button = BTN_A | BTN_B | BTN_R;
    Input filtered = raw; RitoForm_FilterInput(&player, &filtered);
    assert(!(filtered.cur.button & (BTN_A | BTN_B | BTN_R)));
    assert(raw.cur.button == (BTN_A | BTN_B | BTN_R));

    fresh(); active = 0; floorAt(BGCHECK_Y_MIN); raw.cur.button = raw.press.button = BTN_A;
    assert(MmRitoFlightUpdate(&player, &play) == 0 && animationChanges == 0);
    for (u32 mask : { PLAYER_STATE1_IN_WATER, PLAYER_STATE1_IN_CUTSCENE, PLAYER_STATE1_DEAD,
                      PLAYER_STATE1_TALKING, PLAYER_STATE1_CARRYING_ACTOR, PLAYER_STATE1_INPUT_DISABLED,
                      PLAYER_STATE1_CLIMBING_LADDER, FORMS_STATE1_CLIMBING_LEDGE, FORMS_STATE1_DAMAGED }) {
        fresh(); floorAt(BGCHECK_Y_MIN); raw.cur.button = BTN_A; player.stateFlags1 |= mask;
        tick(); assert(sRito.state == RITO_FLY_OFF && animationChanges == 0);
    }
    fresh(); floorAt(BGCHECK_Y_MIN); raw.cur.button = BTN_A; play.pauseCtx.state = PAUSE_STATE_MAIN;
    tick(); assert(!RitoForm_OwnsAction());
    fresh(); floorAt(BGCHECK_Y_MIN); raw.cur.button = BTN_A; play.msgCtx.msgMode = MSGMODE_TEXT_DISPLAYING;
    tick(); assert(!RitoForm_OwnsAction());
    fresh(); floorAt(BGCHECK_Y_MIN); raw.cur.button = BTN_A; player.csAction = PLAYER_CSACTION_5;
    tick(); assert(!RitoForm_OwnsAction());
    fresh(); floorAt(BGCHECK_Y_MIN); raw.cur.button = BTN_A; player.transformation = PLAYER_FORM_DEKU;
    tick(); assert(!RitoForm_OwnsAction());
    fresh(); floorAt(BGCHECK_Y_MIN); raw.cur.button = BTN_A; tick();
    play.pauseCtx.state = PAUSE_STATE_MAIN; tick();
    assert(!RitoForm_OwnsAction() && player.actor.gravity == -1.2f);

    std::puts("PASS MM native flight: floor sentinel/ground leave, held/fresh A, charge/magic, flap clip, release/landing, hop guard, bow priority, safety masks");
    if (argc < 2 || std::strcmp(argv[1], "all") == 0) resets();
}
