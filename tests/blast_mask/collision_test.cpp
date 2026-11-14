#include "global.h"
#include "mods/transformation_masks/mm_mask_wear.h"
#include "mods/transformation_masks/transformation_masks.h"
#include "overlays/actors/ovl_En_Bom/z_en_bom.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>

SaveContext gSaveContext{};
static PlayState play;
static Player player;
static Actor enemy;
PlayState* gPlayState = &play;
static EnBom bomb;
static ColliderCylinder enemyBody;
static s32 sCurrentMmMask = ITEM_MM_MASK_BLAST;
static s32 sBlastMaskCooldown;
static bool sBlastARequested;
static bool sBlastBRequested;
#define BLAST_MASK_COOLDOWN 310
static bool instant;
static bool shieldTwoHanded;
static int explosions;

extern "C" void BlastTest_InitBomb(PlayState*, EnBom*);
extern "C" void BlastTest_InitBody(PlayState*, Actor*, ColliderCylinder*, int);
extern "C" void BlastTest_BombFrame(PlayState*, EnBom*, ColliderCylinder*, ColliderCylinder*);
extern "C" int BlastTest_RaiseShield(PlayState*, Player*, Input*, int);
extern "C" void BlastTest_HoldShield(PlayState*, Player*, Input*);
extern "C" int BlastTest_DamageRecoveryOffersActions(PlayState*, Player*);
extern "C" void Player_Action_8084370C(Player*, PlayState*);
extern "C" s32 func_80834B5C(Player*, PlayState*);
extern "C" int32_t CVarGetInteger(const char* key, int32_t fallback) {
    if (std::strcmp(key, "gMods.BlastMask.Instant") == 0) return instant;
    if (std::strcmp(key, "gCheats.ShieldTwoHanded") == 0) return shieldTwoHanded;
    return fallback;
}
extern "C" s32 MmMaskWear_GetCurrent(void) { return sCurrentMmMask; }
extern "C" MmPlayerTransformation MmForm_GetCurrentForm(void) { return MM_PLAYER_FORM_HUMAN; }
u8 GaroForm_VanillaWantsAButton(Player*) { return 0; }
u8 GerudoMhr_LOwnsB(void) { return 0; }
extern "C" s32 Player_InBlockingCsMode(PlayState*, Player*) { return 0; }
static void LiveActor(Actor*, PlayState*) {}
static void OtherDamageAction(Player*, PlayState*) {}

extern "C" Actor* Actor_Spawn(ActorContext* context, PlayState* current, s16 id, f32 x, f32 y, f32 z,
                              s16 rx, s16 ry, s16 rz, s16 params) {
    assert(context == &play.actorCtx && current == &play && id == ACTOR_EN_BOM);
    assert(rx == 0 && ry == 0 && rz == 0 && params == BOMB_BODY);
    bomb = {};
    bomb.actor.world.pos = { x, y, z };
    BlastTest_InitBomb(&play, &bomb);
    ++explosions;
    return &bomb.actor;
}
extern "C" void Actor_SetScale(Actor* actor, f32 scale) { actor->scale = { scale, scale, scale }; }

/* BLAST_HELPERS */
/* BLAST_UPDATE */

static void Reset(bool child = false) {
    play = {};
    player = {};
    enemy = {};
    bomb = {};
    gSaveContext = {};
    gSaveContext.linkAge = child ? LINK_AGE_CHILD : LINK_AGE_ADULT;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    player.actor.category = ACTORCAT_PLAYER;
    player.actor.update = LiveActor;
    player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    player.heldItemAction = player.itemAction = PLAYER_IA_NONE;
    player.currentShield = child ? PLAYER_SHIELD_DEKU : PLAYER_SHIELD_HYLIAN;
    player.rightHandType = PLAYER_MODELTYPE_RH_OPEN;
    player.sheathType = PLAYER_MODELTYPE_SHEATH_18;
    enemy.category = ACTORCAT_ENEMY;
    enemy.update = LiveActor;
    enemy.world.pos = { 30.0f, 0.0f, 0.0f };
    BlastTest_InitBody(&play, &player.actor, &player.cylinder, 0);
    BlastTest_InitBody(&play, &enemy, &enemyBody, 1);
    sCurrentMmMask = ITEM_MM_MASK_BLAST;
    sBlastMaskCooldown = 0;
    sBlastARequested = sBlastBRequested = false;
    instant = false;
    shieldTwoHanded = false;
    explosions = 0;
}

static Input Press(u16 buttons) {
    Input input{};
    input.cur.button = input.press.button = buttons;
    return input;
}

static void Fire(bool raise, bool targeted = false, bool held = false) {
    Input input = Press(BTN_B | (raise ? BTN_R : 0));
    play.state.input[0] = input;
    TransformMasks_FilterB(&input);
    // This is the real caller order: filter; native action/upper action; mask update.
    if (raise) {
        if (targeted) player.stateFlags1 |= PLAYER_STATE1_FRIENDLY_ACTOR_FOCUS;
        const bool raised = BlastTest_RaiseShield(&play, &player, &input, targeted);
        assert(raised == (player.currentShield != PLAYER_SHIELD_NONE));
        if (held && raised) BlastTest_HoldShield(&play, &player, &input);
    }
    MmMaskWear_UpdateBlast(&play, &player);
    assert(explosions == 1);
}

static void CheckExplosion(bool selfDamage) {
    bool hitPlayer = false, hitEnemy = false;
    for (int frame = 0; frame < 12; ++frame) {
        BlastTest_BombFrame(&play, &bomb, &player.cylinder, &enemyBody);
        if (player.actor.colChkInfo.damage != 0) {
            assert(player.actor.colChkInfo.damage == 8);
            hitPlayer = true;
        }
        if (enemy.colChkInfo.damage != 0) {
            assert(enemy.colChkInfo.damage == 8 && "shielding must preserve enemy damage");
            hitEnemy = true;
        }
    }
    assert(hitPlayer == selfDamage && "only a raised shield may suppress the mask's own damage");
    assert(hitEnemy && "the shield-safe mask must still hit nearby enemies");
    assert(player.invincibilityTimer == 0 && "Blast protection must not grant blanket invincibility");
}

int main() {
    for (bool child : { false, true }) {
        for (s8 weapon : { s8(PLAYER_IA_SWORD_BIGGORON), s8(PLAYER_IA_HAMMER), s8(PLAYER_IA_DEKU_STICK) }) {
            for (bool cheat : { false, true }) {
                for (bool targeted : { false, true }) {
                    Reset(child);
                    player.heldItemAction = player.itemAction = weapon;
                    shieldTwoHanded = cheat;
                    Fire(true, targeted);
                    const bool usableShield = cheat && weapon != PLAYER_IA_DEKU_STICK;
                    // Targeted first-frame guarding installs its upper action;
                    // standing guard executes the real native model decision now.
                    if (!targeted) {
                        assert((player.rightHandType == PLAYER_MODELTYPE_RH_SHIELD) == usableShield);
                    }
                    CheckExplosion(!usableShield);
                }
            }
        }
        if (child) {
            for (s8 held : { s8(PLAYER_IA_NONE), s8(PLAYER_IA_DEKU_STICK) }) {
                Reset(true);
                player.currentShield = PLAYER_SHIELD_HYLIAN;
                player.heldItemAction = player.itemAction = held;
                Fire(true);
                assert(player.rightHandType != PLAYER_MODELTYPE_RH_SHIELD);
                CheckExplosion(false);
            }
        }
        for (f32 frame : { 23.5f, 24.0f, 31.0f }) {
            Reset(child);
            player.stateFlags1 = PLAYER_STATE1_DAMAGED;
            player.actionFunc = Player_Action_8084370C;
            player.skelAnime.endFrame = 40.0f;
            player.skelAnime.curFrame = frame;
            Input recovering = Press(BTN_B);
            play.state.input[0] = recovering;
            TransformMasks_FilterB(&recovering);
            const bool nativeOffersActions = BlastTest_DamageRecoveryOffersActions(&play, &player);
            assert(nativeOffersActions == (frame >= 24.0f));
            MmMaskWear_UpdateBlast(&play, &player);
            assert(explosions == nativeOffersActions &&
                   "Blast must respect the native damage action's recovery window");
        }
        Reset(child);
        player.stateFlags1 = PLAYER_STATE1_DAMAGED;
        player.actionFunc = OtherDamageAction;
        player.skelAnime.curFrame = player.skelAnime.endFrame = 40.0f;
        Input damaged = Press(BTN_B);
        play.state.input[0] = damaged;
        TransformMasks_FilterB(&damaged);
        MmMaskWear_UpdateBlast(&play, &player);
        assert(explosions == 0 && "Other active damage actions must not grant a recovery window");
        Reset(child);
        player.invincibilityTimer = 20;
        Fire(false);
        assert(player.invincibilityTimer == 20 && "Healthy remaining invincibility is not hitstun");

        Reset(child);
        Fire(true);
        CheckExplosion(false);
        Reset(child);
        Fire(true, true);
        CheckExplosion(false);
        Reset(child);
        Fire(true, true, true);
        CheckExplosion(false);
        Reset(child);
        Fire(false);
        CheckExplosion(true);
        Reset(child);
        player.currentShield = PLAYER_SHIELD_NONE;
        Fire(true);
        CheckExplosion(true);
        Reset(child);
        Fire(true);
        // A second ordinary bomb retains its native collision categories even
        // while the shield-safe mask explosion exists and the shield is held.
        bomb = {};
        BlastTest_InitBomb(&play, &bomb);
        bomb.timer = 1;
        bomb.actor.scale = { 0.01f, 0.01f, 0.01f };
        CheckExplosion(true);

        Reset(child);
        player.stateFlags1 |= PLAYER_STATE1_FRIENDLY_ACTOR_FOCUS;
        Input held = Press(BTN_R);
        assert(BlastTest_RaiseShield(&play, &player, &held, true));
        Input released = Press(BTN_B);
        play.state.input[0] = released;
        TransformMasks_FilterB(&released);
        BlastTest_HoldShield(&play, &player, &released);
        MmMaskWear_UpdateBlast(&play, &player);
        assert(explosions == 1);
        CheckExplosion(true);
    }
    puts("PASS native OoT bomb lifetime/collision/damage: same-frame and held shield+B, "
         "adult/child, unshielded/no-shield harm, ordinary bombs, enemy damage and native hitstun window");
}
