#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef int32_t s32;
typedef int8_t s8;
typedef float f32;
typedef struct { f32 x, y, z; } Vec3f;

/* PRODUCTION_FORM_ENUM */
/* PRODUCTION_STEP_DECLARATIONS */

enum {
    BGCHECKFLAG_GROUND = 1, BGCHECKFLAG_WATER = 0x20, BGCHECK_SCENE = 0x32,
    SEASON_SUMMER = 1, SEASON_WINTER = 3, PLAYER_BOOTS_KOKIRI = 0, PLAYER_BOOTS_IRON = 1,
    PLAYER_STATE2_IDLE_FIDGET = 1, PLAYER_STATE2_CRAWLING = 2, PLAYER_STATE2_FOOTSTEP = 4,
    NA_SE_PL_WALK_GROUND = 0x0800, NA_SE_PL_WALK_SAND = 0x0801,
    NA_SE_PL_WALK_HEAVYBOOTS = 0x08B0, NA_SE_PL_JUMP = 0x0810, NA_SE_PL_LAND = 0x0820,
    NA_SE_PL_JUMP_HEAVYBOOTS = 0x08B1, NA_SE_PL_LAND_HEAVYBOOTS = 0x08B2,
    NA_SE_PL_HOBBERBOOTS_LV = 0x08C9, SFX_FLAG = 0x0800, COUNT_STEPS = 0, R_UPDATE_RATE = 3,
};
typedef struct { u16 unk_94; } AgeProperties;
typedef struct { Vec3f projectedPos; s32 bgCheckFlags, floorBgId; } Actor;
typedef struct {
    Actor actor;
    u16 floorSfxOffset;
    AgeProperties* ageProperties;
    s32 currentBoots, stateFlags2, hoverBootsTimer;
    f32 linearVelocity, unk_868;
} Player;
static struct { struct { struct { s32 gameComplete, count[1]; } stats; } ship; } gSaveContext;
static void* gPlayState;
static AgeProperties age;
static Player player;
static s32 currentForm = MM_PLAYER_FORM_DEKU, rolling, mmAvailable = 1, acceptMm = 1, winter;
static s32 mmCalls, exCalls, nativeCalls, hoverCalls, acceptedMm;
static u16 lastMmId, lastNativeId;
static f32 lastFreq, lastVol, lastNativeParam;
static f32* queuedFreq;
static f32* queuedVol;
static Vec3f* lastPos;
static u8 lastToken;

static void require(s32 condition, const char* message) {
    if (!condition) { fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}
static void near(f32 actual, f32 expected, const char* message) {
    if (fabsf(actual - expected) > 0.00001f) {
        fprintf(stderr, "FAIL: %s (got %.7f, expected %.7f)\n", message, actual, expected);
        exit(1);
    }
}
s32 MmSfx_IsAvailable(void) { return mmAvailable; }
s32 MmForm_IsGoronRolling(void) { return currentForm == MM_PLAYER_FORM_GORON && rolling; }
MmPlayerTransformation MmForm_GetCurrentForm(void) { return currentForm; }
static void captureMm(u16 id, Vec3f* pos, f32* freq, f32* vol) {
    ++mmCalls;
    lastMmId = id;
    lastPos = pos;
    queuedFreq = freq;
    queuedVol = vol;
    lastFreq = freq ? *freq : 1.0f;
    lastVol = vol ? *vol : 1.0f;
    acceptedMm += mmAvailable && acceptMm;
}
s32 MmSfx_PlayAtPos(u16 id, Vec3f* pos) {
    captureMm(id, pos, NULL, NULL);
    return mmAvailable && acceptMm;
}
s32 MmSfx_PlayEx(u16 id, Vec3f* pos, u8 token, f32* freq, f32* vol, s8* reverb) {
    ++exCalls;
    lastToken = token;
    captureMm(id, pos, freq, vol);
    return mmAvailable && acceptMm;
}
s32 RocBoots_OnWater(void) { return 0; }
s32 SeasonWeather_ParticleSeasonForPlay(void* play) { return winter ? SEASON_WINTER : SEASON_SUMMER; }
void func_800F4010(Vec3f* pos, u16 id, f32 param) {
    ++nativeCalls;
    lastNativeId = id;
    lastNativeParam = param;
}
void Player_PlaySfx(Player* self, u16 id) { ++nativeCalls; lastNativeId = id; }
f32 GerudoMhr_RunAnimRateMul(void) { return 1.0f; }
f32 KafeiForm_RunAnimRateMul(void) { return 1.0f; }
f32 ExtEquip_LegCycleRateMul(void) { return 1.0f; }
s32 Player_IsHovering(Player* self) { return 0; }
s32 Player_IsFloatedWithoutBoots(Player* self) { return 0; }
void Actor_PlaySfx_Flagged2(Actor* actor, u16 id) { ++hoverCalls; }

/* PRODUCTION_BODIES */

static void reset(void) {
    memset(&player, 0, sizeof(player));
    memset(&gSaveContext, 0, sizeof(gSaveContext));
    age.unk_94 = 0;
    player.ageProperties = &age;
    player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    player.actor.floorBgId = BGCHECK_SCENE;
    player.linearVelocity = 3.0f;
    currentForm = MM_PLAYER_FORM_DEKU;
    mmAvailable = acceptMm = 1;
    rolling = winter = mmCalls = exCalls = nativeCalls = hoverCalls = acceptedMm = 0;
    lastMmId = lastNativeId = lastToken = 0;
    lastFreq = lastVol = lastNativeParam = 0;
    queuedFreq = queuedVol = NULL;
    lastPos = NULL;
}

// Verbatim native curve from marsh6487/ComboShip f3931e973c5d4dd55890822295dc77cf15dbfbdc,
// mm/src/audio/code_8019AF00.c:4632. Only its name and static variables are
// renamed so this independently retained reference can run beside the adapter.
static f32 nativeSyncedVolume, nativeSyncedFreq;
static f32 NativeMm_SetSyncedSfxFreqAndVolume(f32 freqVolParam) {
    f32 ret = 1.0f;
    if (freqVolParam > 6.0f) {
        nativeSyncedVolume = 1.0f;
        nativeSyncedFreq = 1.1f;
    } else {
        ret = freqVolParam / 6.0f;
        nativeSyncedVolume = ret * (1.0f - 0.775f) + 0.775f;
        nativeSyncedFreq = (ret * 0.2f) + 0.9f;
    }
    return ret;
}

static void curve(void) {
    const struct { f32 speed, freq, vol; } cases[] = {
        { 0.0f, 0.9f, 0.775f }, { 1.5f, 0.95f, 0.83125f }, { 3.0f, 1.0f, 0.8875f },
        { 6.0f, 1.1f, 1.0f }, { 9.0f, 1.1f, 1.0f },
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        reset();
        NativeMm_SetSyncedSfxFreqAndVolume(cases[i].speed);
        near(nativeSyncedFreq, cases[i].freq, "native reference pitch");
        near(nativeSyncedVolume, cases[i].vol, "native reference volume");
        Player_PlaySteppingSfx(&player, cases[i].speed);
        near(lastFreq, cases[i].freq, "Deku stepping must retain native speed-dependent pitch");
        near(lastVol, cases[i].vol, "submit native volume to the existing audio adapter");
        require(mmCalls == 1 && acceptedMm == 1 && exCalls == 1 && nativeCalls == 0,
                "one existing step must submit one MM request and suppress OoT");
        require(lastPos == &player.actor.projectedPos && lastToken == 4,
                "retain the live source position and native SFX token");
    }
}

static void surfaces(void) {
    // Native Deku playerbank slots remain flagged one-shots, including snow/ice.
    const u16 ids[] = { 0x8F0, 0x8F1, 0x8F2, 0x8F3, 0x8F4, 0x8F5, 0x8F6, 0x8F7,
                        0x8F8, 0x8F9, 0x8FA, 0x8FB, 0x8FC, 0x8FD, 0x8FE, 0x8FF };
    for (u16 surface = 0; surface < 16; ++surface) {
        reset();
        player.floorSfxOffset = surface;
        Player_PlaySteppingSfx(&player, 6.0f);
        require(lastMmId == ids[surface] && (lastMmId & SFX_FLAG),
                "surface selection must retain Deku bank and one-shot flag");
        require(mmCalls == 1 && nativeCalls == 0, "never layer native footsteps over Deku");
    }
}

static void cadence(void) {
    reset();
    player.linearVelocity = 6.0f;
    // Existing OoT cadence advances 1.5 frames/update: 58 updates = three
    // 29-frame cycles, each containing the native phase-10 and phase-24 step.
    for (s32 frame = 0; frame < 58; ++frame) func_8084029C(&player, 1.0f);
    require(mmCalls == 6 && acceptedMm == 6 && nativeCalls == 0,
            "three existing animation cycles must produce exactly six Deku steps");
    require(gSaveContext.ship.stats.count[COUNT_STEPS] == 6 &&
                (player.stateFlags2 & PLAYER_STATE2_FOOTSTEP), "retain step count and footstep flag");
    near(player.unk_868, 0.0f, "keep existing animation cycle wrap");
}

static void fallback(void) {
    reset();
    mmAvailable = 0;
    player.floorSfxOffset = 2;
    age.unk_94 = 0x40;
    Player_PlaySteppingSfx(&player, 1.5f);
    require(mmCalls == 0 && nativeCalls == 1 && lastNativeId == 0x842,
            "unavailable MM audio must retain the native surface/age fallback");
    near(lastNativeParam, 1.5f, "native fallback keeps its speed parameter");
    reset();
    acceptMm = 0;
    Player_PlaySteppingSfx(&player, 6.0f);
    require(mmCalls == 1 && acceptedMm == 0 && nativeCalls == 1 && lastNativeId == 0x800,
            "a rejected Deku request must fall back once rather than silently suppress OoT");
}

static void controls(void) {
    const struct { s32 form; u16 id; } forms[] = {
        { MM_PLAYER_FORM_FIERCE_DEITY, 0x880 }, { MM_PLAYER_FORM_GORON, 0x950 },
        { MM_PLAYER_FORM_ZORA, 0x920 },
    };
    for (unsigned i = 0; i < sizeof(forms) / sizeof(forms[0]); ++i) {
        reset();
        currentForm = forms[i].form;
        Player_PlaySteppingSfx(&player, 0.0f);
        require(mmCalls == 1 && exCalls == 0 && lastMmId == forms[i].id && nativeCalls == 0,
                "other MM forms retain their existing routing");
        near(lastFreq, 1.0f, "other form pitch must retain baseline behavior");
        near(lastVol, 1.0f, "other form volume must retain baseline behavior");
    }
    for (s32 form = MM_PLAYER_FORM_HUMAN; form < MM_PLAYER_FORM_MAX; ++form) {
        reset(); currentForm = form;
        Player_PlaySteppingSfx(&player, 6.0f);
        require(mmCalls == 0 && nativeCalls == 1 && lastNativeId == 0x800,
                "human and custom forms retain native stepping");
    }
    reset(); currentForm = MM_PLAYER_FORM_GORON; rolling = 1;
    Player_PlaySteppingSfx(&player, 6.0f);
    require(mmCalls == 0 && nativeCalls == 0, "Goron rolling must not acquire footsteps");
    reset(); gSaveContext.ship.stats.gameComplete = 1;
    Player_PlaySteppingSfx(&player, 6.0f);
    require(gSaveContext.ship.stats.count[COUNT_STEPS] == 0, "keep completed-game step statistic gate");
    reset(); player.stateFlags2 = PLAYER_STATE2_IDLE_FIDGET | PLAYER_STATE2_CRAWLING;
    Player_PlaySteppingSfx(&player, 6.0f);
    require(gSaveContext.ship.stats.count[COUNT_STEPS] == 0, "keep fidget/crawl statistic gate");
}

static void legacy(void) {
    reset();
    u8 (*oldApi)(u16, Vec3f*) = TransformMasks_TryPlayMmStepSfx;
    require(oldApi(0x804, &player.actor.projectedPos), "the two-argument helper remains usable");
    require(mmCalls == 1 && exCalls == 0 && lastMmId == 0x8F4, "legacy helper keeps baseline mapping");
    near(lastFreq, 1.0f, "legacy helper keeps its default pitch");
}

static void lifetime(void) {
    reset();
    Player_PlaySteppingSfx(&player, 0.0f);
    require(queuedFreq && queuedVol, "Deku speed parameters must be queued by address");
    // Audio retains these addresses after the production helper returns.
    volatile unsigned char stackNoise[4096];
    memset((void*)stackNoise, 0xA5, sizeof(stackNoise));
    f32* firstFreq = queuedFreq;
    f32* firstVol = queuedVol;
    near(*queuedFreq, 0.9f, "queued frequency must outlive the stepping call");
    near(*queuedVol, 0.775f, "queued volume must outlive the stepping call");
    Player_PlaySteppingSfx(&player, 6.0f);
    require(firstFreq == queuedFreq && firstVol == queuedVol, "reuse persistent native-style parameter storage");
    near(*queuedFreq, 1.1f, "persistent frequency follows the next step");
}

static void jump_land(void) {
    reset(); player.floorSfxOffset = 2; age.unk_94 = 0x40;
    Player_PlayJumpingSfx(&player);
    require(nativeCalls == 1 && lastNativeId == 0x852 && mmCalls == 0, "jump keeps native surface/age audio");
    Player_PlayLandingSfx(&player);
    require(nativeCalls == 2 && lastNativeId == 0x862 && mmCalls == 0, "landing keeps native surface/age audio");
    player.currentBoots = PLAYER_BOOTS_IRON;
    Player_PlayJumpingSfx(&player);
    require(lastNativeId == NA_SE_PL_JUMP_HEAVYBOOTS && mmCalls == 0, "keep heavy-boot jump audio");
    Player_PlayLandingSfx(&player);
    require(lastNativeId == NA_SE_PL_LAND_HEAVYBOOTS && mmCalls == 0, "keep heavy-boot landing audio");
}

static void winter_surface(void) {
    reset(); winter = 1;
    Player_PlaySteppingSfx(&player, 6.0f);
    require(lastMmId == 0x8FE && mmCalls == 1, "winter ground uses the existing native Deku snow slot");
    reset(); winter = 1; player.floorSfxOffset = 4; player.actor.bgCheckFlags |= BGCHECKFLAG_WATER;
    Player_PlaySteppingSfx(&player, 6.0f);
    require(lastMmId == 0x8F4, "shallow-water stepping keeps its water surface");
    reset(); winter = 1; currentForm = MM_PLAYER_FORM_HUMAN; mmAvailable = 0;
    Player_PlaySteppingSfx(&player, 6.0f);
    require(acceptedMm == 0 && nativeCalls == 1 && lastNativeId == 0x801,
            "unavailable winter audio retains the soft native fallback");
}

int main(int argc, char** argv) {
    require(argc == 2, "select a named case");
    if (!strcmp(argv[1], "curve")) curve();
    else if (!strcmp(argv[1], "surfaces")) surfaces();
    else if (!strcmp(argv[1], "cadence")) cadence();
    else if (!strcmp(argv[1], "fallback")) fallback();
    else if (!strcmp(argv[1], "controls")) controls();
    else if (!strcmp(argv[1], "legacy")) legacy();
    else if (!strcmp(argv[1], "lifetime")) lifetime();
    else if (!strcmp(argv[1], "jump-land")) jump_land();
    else if (!strcmp(argv[1], "winter")) winter_surface();
    else require(0, "unknown case");
    printf("PASS %s\n", argv[1]);
    return 0;
}
