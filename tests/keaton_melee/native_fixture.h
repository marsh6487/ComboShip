#include <cassert>
#include <cstdint>
using u8 = uint8_t; using u16 = uint16_t; using u32 = uint32_t;
using s16 = int16_t; using s32 = int32_t; using f32 = float;
struct Vec3f { f32 x{}, y{}, z{}; };
struct Vec3s { s16 x{}, y{}, z{}; };
struct Actor { int bgCheckFlags{}; struct { Vec3s rot; } shape; Vec3f velocity; };
struct LinkAnimationHeader {};
struct Player {
    Actor actor;
    u32 stateFlags1{}, stateFlags3{};
    s16 yaw{};
    f32 linearVelocity{};
    Vec3f bodyPartsPos[1];
    struct { f32 curFrame{}; } skelAnime;
    int meleeWeaponState{};
    struct { struct { struct { u32 dmgFlags{}; } toucher; u8 toucherFlags{}; } info; } meleeWeaponQuads[2];
};
struct Input { struct { u16 button{}; } cur, press; };
struct PlayState { struct { Input input[1]; } state; };
enum {
    STATE_IDLE, STATE_COMBO, STATE_CHARGE, STATE_THROW, STATE_LONGJUMP, STATE_AIRKICK, STATE_RECOIL
};
enum {
    PLAYER_STATE1_IN_ITEM_CS = 1, PLAYER_STATE1_IN_CUTSCENE = 2, PLAYER_STATE1_DAMAGED = 4,
    PLAYER_STATE1_CLIMBING_LADDER = 8, PLAYER_STATE1_CLIMBING_LEDGE = 16,
    PLAYER_STATE1_HANGING_OFF_LEDGE = 32, PLAYER_STATE1_JUMPING = 64,
    PLAYER_STATE3_PAUSE_ACTION_FUNC = 1, BGCHECKFLAG_GROUND = 1, BGCHECKFLAG_WALL = 2,
    BTN_A = 1, BTN_B = 2, NA_SE_VO_LI_AUTO_JUMP = 1, NA_SE_IT_SWORD_SWING_HARD = 2,
    NA_SE_IT_SWORD_CHARGE = 3, TOUCH_ON = 1, TOUCH_NEAREST = 4, TOUCH_SFX_WOOD = 16,
    DMG_HAMMER_SWING = 64, LONG_JUMP_ANIM_DELAY = 2
};
#define CHECK_BTN_ALL(value, bits) (((value) & (bits)) == (bits))
static LinkAnimationHeader clip;
static LinkAnimationHeader *sCombo[3] = { &clip, &clip, &clip }, *sAirKick = &clip;
static LinkAnimationHeader *sLongJump = &clip, *sCharge = &clip;
static int sState = STATE_IDLE, sStep = 0, sJumpDelay = 0, sChargeTimer = 0, sChargeLevel = 0;
struct Move { f32 hitStart, hitEnd; };
static const Move kCombo[3] = { {4,9}, {6,13}, {14,26} };
static constexpr f32 LONG_JUMP_SPEED = 16, LONG_JUMP_LIFT = 8, LOOP_START = 13, LOOP_END = 17;
static constexpr f32 kChainOpens = .55f;
static constexpr int HADOUKEN_MAGIC_LEVEL1 = 8, HADOUKEN_MAGIC_LEVEL2 = 16;
inline PlayState* expectedPlay;
inline Player* expectedPlayer;
inline int damageCalls, animationCalls;
inline bool giantMask;
inline bool elementalOverride;
extern "C" inline s32 MmMaskWear_IsGiantMaskActive() { return giantMask; }
static u32 DinFireSword_SetDamageFlags(PlayState* play, Player* player, s32 quad, u32 flags) {
    assert(play == expectedPlay && player == expectedPlayer);
    assert(quad == damageCalls && quad < 2);
    ++damageCalls;
    return elementalOverride ? 0x400000u : flags;
}
static void Load() {}
static void UpdateShot(PlayState*, Player*) {}
static void Play(PlayState* play, Player* player, LinkAnimationHeader*, f32, f32) {
    assert(play == expectedPlay && player == expectedPlayer); ++animationCalls;
}
static f32 Animation_GetLastFrame(LinkAnimationHeader*) { return 40; }
static bool LinkAnimation_Update(PlayState*, void*) { return false; }
static void Audio_PlayActorSound2(Actor*, int) {}
static void Player_PlayVoiceSfx(Player*, int) {}
static bool GaroForm_VanillaWantsAButton(Player*) { return false; }
static bool WasBlocked() { return false; }
static void StartRecoil(PlayState*, Player*) {}
static void ClearState() { sState = STATE_IDLE; }
static void EnterCharge(PlayState*, Player*) {}
static u8 LevelForTimer(int) { return 0; }
static void SpawnFlame(PlayState*, void*, int) {}
static void PlaceFlame(PlayState*, void*, int) {}
static void KillFlame(PlayState*) {}
static bool ItemMagic_HasEnough(PlayState*, int) { return false; }
static void ItemMagic_Consume(PlayState*, int) {}
static void Launch(PlayState*, Player*, int) {}
static void TickChargeLoop(PlayState*, Player*) {}
