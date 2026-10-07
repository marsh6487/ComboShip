#include "mods/extended_inventory.h"
#include "2s2h/Enhancements/Audio/MMWeather.h"
#include "2s2h/Enhancements/Audio/MMWeatherAudio.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "overlays/actors/ovl_Object_Kankyo/z_object_kankyo.h"
#include "overlays/actors/ovl_En_Test4/z_en_test4.h"
#include "overlays/actors/ovl_En_Weather_Tag/z_en_weather_tag.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include <cassert>
#include <cstdio>
#include <cstring>

void MMAutumnSceneFoliage_Update(const PlayState*) {
}
void MMAutumnSceneFoliage_Reset() {
}

static_assert(PRECIP_RAIN_MAX == 0 && PRECIP_RAIN_CUR == 1 && PRECIP_SNOW_CUR == 2 && PRECIP_SNOW_MAX == 3);
static_assert(WEATHER_MODE_RAIN == 1 && WEATHER_MODE_SNOW == 3 && SEASON_WINTER == 3);

SaveContext gSaveContext{};
PlayState* gPlayState;
RegEditor editor{};
RegEditor* gRegEditor = &editor;
u8 gWeatherMode;
u8 gInterruptSongOfStorms, gLightConfigAfterUnderwater;
u8 D_801BDBC0, D_801BDBC4, D_801F4E30;
u8 sSeqCmdWritePos;
u32 sAudioSeqCmds[256];
static f32 D_808DE5B0;
static u16 D_808DE340;
static int snowDraws, rainDraws, spawns, refreshes;
static int gameplayRandomCalls;
static unsigned leafPalettes, leafDraws, firstLeafAlpha;
static float rainGain;
static u8 nativeRainAmbience, nativeThunderAmbience;
static ObjectKankyo supplemental[32];

extern "C" {
int32_t CVarGetInteger(const char*, int32_t fallback) { return fallback; }
Color_RGBA8 CVarGetColor(const char*, Color_RGBA8 fallback) { return fallback; }
void MMWeatherAudio_Reset() { rainGain = 0; }
void MMWeatherAudio_SetRain(float gain) { rainGain = gain; }
void MMWeatherAudio_Thunder(float) {}
void MMWeather_ClearBolts() {}
void MMWeather_StartBolt() {}
void Environment_DrawLightningFlash(PlayState*, u8, u8, u8, u8) {}
void Audio_SetAmbienceChannelIO(u8 channel, u8 port, u8 value) {
    assert(port == CHANNEL_IO_PORT_1);
    SEQCMD_SET_CHANNEL_IO(SEQ_PLAYER_AMBIENCE, channel, port, value);
    value = sAudioSeqCmds[(u8)(sSeqCmdWritePos - 1)] & 0xFF;
    if (channel == AMBIENCE_CHANNEL_RAIN) nativeRainAmbience = value;
    else if (channel == AMBIENCE_CHANNEL_LIGHTNING) nativeThunderAmbience = value;
    else assert(false);
}
void Environment_PlayStormNatureAmbience(PlayState*) {
    Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_RAIN, CHANNEL_IO_PORT_1, 1);
    Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_LIGHTNING, CHANNEL_IO_PORT_1, 1);
}
void Environment_StopStormNatureAmbience(PlayState*) {
    Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_RAIN, CHANNEL_IO_PORT_1, 0);
    Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_LIGHTNING, CHANNEL_IO_PORT_1, 0);
}
void Environment_WipeRumbleRequests() {}
void Environment_UpdateSkyboxRotY(PlayState*) {}
void Environment_UpdateTimeBasedSequence(PlayState*) {}
void Environment_UpdateNextDayTime() {}
void Environment_UpdateTime(PlayState*, EnvironmentContext*, PauseContext*, MessageContext*, GameOverContext*) {}
void Environment_UpdateSun(PlayState*) {}
void Environment_UpdateLights(PlayState*, EnvironmentContext*, LightContext*) {}
void Environment_UpdatePostmanEvents(PlayState*) {}
void Environment_DrawRainImpl(PlayState*, View*, GraphicsContext*) { ++rainDraws; }
u32 Environment_GetStormState(PlayState* play) { return play->envCtx.stormState; }
NeiSaveData* Nei_Save() { return &gSaveContext.save.shipSaveInfo.nei; }
void ExtInv_RefreshButtonIconsForItem(PlayState*, u16 item) {
    assert(item == EXT_ITEM_ROD_OF_SEASONS);
    ++refreshes;
}
void Actor_Kill(Actor* actor) { actor->update = actor->draw = nullptr; }
f32 Rand_ZeroOne() { ++gameplayRandomCalls; return 0.25f; }
s16 Camera_GetCamDirPitch(Camera*) { return 0; }
f32 Math_Vec3f_DistXZ(Vec3f* a, Vec3f* b) { return sqrtf(SQ(a->x - b->x) + SQ(a->z - b->z)); }
f32 Math_Vec3f_DistXYZ(Vec3f* a, Vec3f* b) { return sqrtf(SQ(a->x-b->x) + SQ(a->y-b->y) + SQ(a->z-b->z)); }
void func_808DBE8C(ObjectKankyo*) {}
void func_808DC038(ObjectKankyo*, PlayState*) {}
void ObjectKankyo_Init(Actor*, PlayState*);
void ObjectKankyo_Update(Actor*, PlayState*);
void func_808DD3C8(Actor*, PlayState*);
Actor* Actor_Spawn(ActorContext* context, PlayState* play, s16 id, f32, f32, f32, s16, s16, s16, s32 params) {
    assert(spawns < 32 && id == ACTOR_OBJECT_KANKYO && params == 1);
    ObjectKankyo* snow = &supplemental[spawns++];
    *snow = {};
    snow->actor.id = id;
    snow->actor.params = params;
    snow->actor.update = ObjectKankyo_Update;
    snow->actor.draw = func_808DD3C8;
    ObjectKankyo_Init(&snow->actor, play);
    snow->actor.next = context->actorLists[ACTORCAT_ITEMACTION].first;
    context->actorLists[ACTORCAT_ITEMACTION].first = &snow->actor;
    return &snow->actor;
}
void Play_GetScreenPos(PlayState*, Vec3f*, Vec3f* screen) { *screen = { 100, 100, 1 }; }
void Matrix_Translate(f32, f32, f32, MatrixMode) {}
void Matrix_Scale(f32, f32, f32, MatrixMode) {}
void Matrix_Mult(MtxF*, MatrixMode) {}
void Matrix_RotateZS(s16, MatrixMode) {}
Mtx* Matrix_Finalize(GraphicsContext*) { static Mtx matrix; return &matrix; }
Gfx* Gfx_SetupDL(Gfx* gfx, u32) { return gfx; }
void* Lib_SegmentedToVirtual(void* resource) { return resource; }
s32 func_80173B48(GameState*) { return 14000000; }
f32 Math_SmoothStepToF(f32* value, f32 target, f32, f32, f32) { *value = target; return 0; }
void FrameInterpolation_RecordOpenChild(const void*, int) {}
void FrameInterpolation_RecordCloseChild() {}
f32 Actor_WorldDistXZToActor(Actor* a, Actor* b) { return Math_Vec3f_DistXZ(&a->world.pos, &b->world.pos); }
void func_80966E0C(EnWeatherTag*, PlayState*) {}
float OTRGetAspectRatio() { return 4.0f / 3.0f; }
float OTRGetDimensionFromLeftEdge(float value) { return value; }
float OTRGetDimensionFromRightEdge(float value) { return value; }
}

// Keep native GBI writes and snow renderer execution; resource submission,
// projection and frame interpolation are the asset-free engine boundaries.
#undef OPEN_DISPS_PORT_HELPERS
#undef CLOSE_DISPS_PORT_HELPERS
#define OPEN_DISPS_PORT_HELPERS(gfxCtx)
#define CLOSE_DISPS_PORT_HELPERS(gfxCtx)
#undef gSPDisplayList
#define gSPDisplayList(pkt, dl) do { ++snowDraws; __gSPDisplayList(pkt, (Gfx*)(dl)); } while (0)
#define gSPSegment(pkt, segment, resource) __gSPSegment(pkt, segment, (uintptr_t)(resource))
#define Lib_SegmentedToVirtual(resource) ((void*)(resource))
#include "mods/items/objects/object_autumn_leaves.h"

typedef void (*BoxMenuConfirmFn)(s32);
static u8 sBoxMOpen, sBoxMHoldSeen, sBoxMStickHeld;
static s16 sBoxMPulse;
static u16 sBoxMHoldButton;
static s32 sBoxMCursor, sBoxMCount;
static BoxMenuConfirmFn sBoxMOnConfirm;
#define BOXM_STICK_DEAD 30
static void BoxMenu_PlaySfx(u16) {}
static s32 BoxMenu_Step(s32 from, s32) { return from; }
extern "C" {
#include "native_weather.inc"
}

static void Reset(PlayState* play, Camera* camera, GraphicsContext* gfx) {
    MMWeather_Reset();
    *play = {};
    *camera = {};
    play->sceneId = SCENE_TOWN;
    play->skyboxId = SKYBOX_NORMAL_SKY;
    play->cameraPtrs[0] = camera;
    play->view.at.z = 1;
    play->state.gfxCtx = gfx;
    play->envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
    play->envCtx.stormState = STORM_STATE_ON;
    gSaveContext = {};
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    gSaveContext.save.day = 1;
    gSaveContext.save.time = CLOCK_TIME(12, 0);
    gSaveContext.save.shipSaveInfo.nei.seasonsOwned = 0x0F;
    gSaveContext.save.shipSaveInfo.nei.season = SEASON_OFF;
    gPlayState = play;
    D_801F4E30 = 0;
    gWeatherMode = WEATHER_MODE_CLEAR;
    AudioSeq_QueueSeqCmd((SEQCMD_OP_PLAY_SEQUENCE << 28) | (SEQ_PLAYER_AMBIENCE << 24) | NA_BGM_AMBIENCE);
    nativeRainAmbience = nativeThunderAmbience = 0;
    // Same storage, scene and gameplay frame rewind must reset rod ownership.
    play->gameplayFrames = 0;
    Environment_Update(play, &play->envCtx, &play->lightCtx, &play->pauseCtx, &play->msgCtx,
                       &play->gameOverCtx, gfx);
}

static void EnvironmentFrame(PlayState* play) {
    Environment_Update(play, &play->envCtx, &play->lightCtx, &play->pauseCtx, &play->msgCtx,
                       &play->gameOverCtx, play->state.gfxCtx);
}

static void Frame(PlayState* play, EnTest4* dayActor = nullptr) {
    ++play->state.frames;
    ++play->gameplayFrames;
    if (dayActor) {
        if (dayActor->weather == THREEDAY_WEATHER_RAIN) EnTest4_UpdateWeatherRainy(dayActor, play);
        else EnTest4_UpdateWeatherClear(dayActor, play);
    }
    for (Actor* actor = play->actorCtx.actorLists[ACTORCAT_ITEMACTION].first; actor; actor = actor->next) {
        if (actor->update) actor->update(actor, play);
    }
    EnvironmentFrame(play);
}

static void Confirm(PlayState* play, u8 season) {
    // Play_Update's paused wheel update precedes Environment_Update, and does
    // not run Player_Update or advance gameplayFrames on the confirm frame.
    const u32 frame = play->gameplayFrames;
    sBoxMOpen = 1;
    sBoxMCursor = season;
    sBoxMCount = 5;
    sBoxMOnConfirm = Seasons_OnWheelConfirm;
    play->pauseCtx.state = PAUSE_STATE_MAIN;
    play->state.input[0].press.button = BTN_A;
    BoxMenu_Update(play);
    assert(!sBoxMOpen && play->pauseCtx.state == PAUSE_STATE_OFF);
    EnvironmentFrame(play);
    assert(play->gameplayFrames == frame);
}

static int DrawSnow(PlayState* play) {
    static Gfx commands[2048];
    play->state.gfxCtx->polyXlu.p = commands;
    int before = snowDraws;
    leafPalettes = leafDraws = 0;
    firstLeafAlpha = 0;
    for (Actor* actor = play->actorCtx.actorLists[ACTORCAT_ITEMACTION].first; actor; actor = actor->next) {
        if (actor->draw) actor->draw(actor, play);
    }
    bool havePrim = false;
    for (const Gfx* command = commands; command < play->state.gfxCtx->polyXlu.p; ++command) {
        if ((command->words.w0 >> 24) == G_SETPRIMCOLOR && !havePrim) {
            firstLeafAlpha = command->words.w1 & 255;
            havePrim = true;
        }
        if ((command->words.w0 >> 24) == G_SETTIMG) {
            for (unsigned palette = 0; palette < 4; ++palette) {
                if (command->words.w1 == (uintptr_t)sAutumnLeafTextures[palette]) {
                    assert(((command->words.w0 >> 19) & 3) == G_IM_SIZ_32b);
                    leafPalettes |= 1 << palette;
                    ++leafDraws;
                }
            }
        }
    }
    return snowDraws - before;
}

static void AutumnCoverageRegression() {
    static PlayState play;
    Camera camera{};
    GraphicsContext gfx{};
    Reset(&play, &camera, &gfx);
    play.sceneId = SCENE_00KEIKOKU;
    camera.fov = 60;
    Confirm(&play, SEASON_AUTUMN);
    for (int frame = 0; frame < 30; ++frame) Frame(&play);
    auto* actor = (ObjectKankyo*)play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first;
    unsigned near = 0, middle = 0, far = 0;
    for (const auto& p : actor->unk_14C) {
        if (!p.unk_1C) continue;
        Vec3f pos{p.unk_00 + p.unk_0C, p.unk_04 + p.unk_10, p.unk_08 + p.unk_14};
        const float distance = Math_Vec3f_DistXZ(&pos, &play.view.eye);
        near += distance < 1200;
        middle += distance >= 1200 && distance < 3200;
        far += distance >= 3200;
    }
    assert(near && middle && far); // The old snow bubble never reaches the middle/background.
    assert(near == 16 && middle == 32 && far == 48); // User requested a quieter foreground.
    auto first = actor->unk_14C[0];
    play.view.eye.x += 30;
    play.view.at = {30, 0, -1}; // Turn the view without dragging existing leaves along.
    Frame(&play);
    assert(actor->unk_14C[0].epoch == first.epoch);
    assert(fabsf(actor->unk_14C[0].unk_00 - first.unk_00) < 2);
    assert(fabsf(actor->unk_14C[0].unk_08 - first.unk_08) < 2);
    assert(DrawSnow(&play) <= 128);
    // A background leaf must retain visible alpha rather than inherit the snow's 300-unit fade.
    actor->unk_14C[0].unk_00 = actor->unk_14C[0].unk_0C = actor->unk_14C[0].unk_10 = actor->unk_14C[0].unk_14 = 0;
    actor->unk_14C[0].unk_04 = 0;
    actor->unk_14C[0].unk_18 = 30;
    for (float distance : { 1500.0f, 6000.0f }) {
        actor->unk_14C[0].unk_08 = distance;
        DrawSnow(&play);
        assert(firstLeafAlpha > 100);
    }
    Player link{};
    link.actor.world.pos.z = actor->unk_14C[0].unk_08;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &link.actor;
    DrawSnow(&play);
    assert(firstLeafAlpha == 0); // The clear pocket follows Link, even after camera/wind movement.
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = nullptr;
    std::puts("PASS autumn foreground/midground/background coverage, world stability and bounded rendering");
}

int main() {
    AutumnCoverageRegression();
    static PlayState play;
    Camera camera{};
    GraphicsContext gfx{};
    Reset(&play, &camera, &gfx);
    Confirm(&play, SEASON_WINTER);
    assert(play.envCtx.precipitation[PRECIP_SNOW_CUR] == 0 && play.envCtx.precipitation[PRECIP_SNOW_MAX] == 0);
    const int firstSpawn = spawns;
    Frame(&play);
    assert(DrawSnow(&play) == 64 && spawns == firstSpawn);
    Actor* winterActor = play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first;
    Confirm(&play, SEASON_SPRING);
    assert(play.envCtx.precipitation[PRECIP_SNOW_CUR] == 0 && play.envCtx.precipitation[PRECIP_SNOW_MAX] == 0);
    assert(winterActor->update == nullptr && winterActor->draw == nullptr && DrawSnow(&play) == 0);
    assert(MMWeather_RainDensity() == 30 && rainGain == 1.0f);
    Frame(&play);
    assert(DrawSnow(&play) == 0);
    Confirm(&play, SEASON_WINTER);
    assert(spawns == firstSpawn + 1 && play.envCtx.precipitation[PRECIP_SNOW_CUR] == 0);
    Frame(&play);
    assert(DrawSnow(&play) == 64);
    Confirm(&play, SEASON_AUTUMN);
    assert(DrawSnow(&play) == 96 && play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first->update != nullptr);
    assert(leafPalettes == 15 && leafDraws == 96);
    auto* leaves = (ObjectKankyo*)play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first;
    const f32 beforeLeafY = leaves->unk_14C[0].unk_10;
    const int beforeLeafRandom = gameplayRandomCalls;
    Frame(&play);
    assert(leaves->unk_14C[0].unk_10 < beforeLeafY && gameplayRandomCalls == beforeLeafRandom);
    Confirm(&play, SEASON_WINTER);
    assert(spawns == firstSpawn + 1); // Autumn/Winter reuse the same native particle owner.
    const int beforeRestoreRandom = gameplayRandomCalls;
    assert(DrawSnow(&play) == 64);
    assert(leaves->unk_14C[0].unk_1C == 1 && firstLeafAlpha > 0);
    assert(gameplayRandomCalls == beforeRestoreRandom);
    Frame(&play);
    assert(DrawSnow(&play) == 64);
    assert(leafDraws == 0 && leafPalettes == 0);
    Confirm(&play, SEASON_OFF);
    assert(play.envCtx.precipitation[PRECIP_SNOW_CUR] == 0 && play.envCtx.precipitation[PRECIP_SNOW_MAX] == 0);
    assert(DrawSnow(&play) == 0);
    std::puts("PASS real paused selector confirm: Winter/Spring/Off apply immediately, supplemental snow is removed");

    Reset(&play, &camera, &gfx);
    play.envCtx.stormState = STORM_STATE_OFF;
    const int beforeAutumnSpawnRandom = gameplayRandomCalls;
    Confirm(&play, SEASON_AUTUMN);
    Frame(&play);
    assert(play.envCtx.stormState == STORM_STATE_OFF && DrawSnow(&play) == 96);
    assert(leafPalettes == 15 && leafDraws == 96);
    assert(gameplayRandomCalls == beforeAutumnSpawnRandom);

    // Scene-native snow actors must also initialize their autumn leaf phases
    // without advancing the random sequence used by gameplay.
    ObjectKankyo autumnNormal{}, autumnBlizzard{};
    autumnNormal.actor.params = 3;
    autumnBlizzard.actor.params = 2;
    play.envCtx.precipitation[PRECIP_SNOW_CUR] = 128;
    ObjectKankyo_Init(&autumnNormal.actor, &play);
    ObjectKankyo_Init(&autumnBlizzard.actor, &play);
    assert(gameplayRandomCalls == beforeAutumnSpawnRandom);
    std::puts("PASS clear-weather Autumn submits all four generated RGBA palettes, native fall and gameplay RNG isolation");

    // Snowhead's actual blizzard actors and environment remain the native
    // owners. Winter reuses them, clear seasons suppress all variants, Off restores them.
    Reset(&play, &camera, &gfx);
    ObjectKankyo blizzard{}, normal{};
    blizzard.actor.id = normal.actor.id = ACTOR_OBJECT_KANKYO;
    blizzard.actor.params = 2;
    normal.actor.params = 3;
    blizzard.actor.update = normal.actor.update = ObjectKankyo_Update;
    blizzard.actor.draw = normal.actor.draw = func_808DD3C8;
    ObjectKankyo_Init(&blizzard.actor, &play);
    ObjectKankyo_Init(&normal.actor, &play);
    blizzard.actor.next = &normal.actor;
    play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first = &blizzard.actor;
    play.envCtx.precipitation[PRECIP_SNOW_MAX] = 128;
    play.envCtx.sandstormState = SANDSTORM_A;
    play.envCtx.sandstormPrimA = play.envCtx.sandstormEnvA = D_801F4E30 = 155;
    int count = spawns;
    Confirm(&play, SEASON_WINTER);
    Frame(&play);
    assert(spawns == count && play.envCtx.precipitation[PRECIP_SNOW_CUR] == 128);
    assert(DrawSnow(&play) > 0);
    for (u8 season : { SEASON_SPRING, SEASON_SUMMER }) {
        Confirm(&play, season);
        Frame(&play);
        assert(play.envCtx.precipitation[PRECIP_SNOW_CUR] == 128 && DrawSnow(&play) == 0);
        assert(!NativeFogVisible(&play, SANDSTORM_A));
        assert(play.envCtx.sandstormState == SANDSTORM_A && D_801F4E30 == 155);
        assert(blizzard.actor.update && normal.actor.update && spawns == count);
    }
    Confirm(&play, SEASON_AUTUMN);
    Frame(&play);
    assert(play.envCtx.precipitation[PRECIP_SNOW_CUR] == 128 && play.envCtx.precipitation[PRECIP_SNOW_MAX] == 128);
    assert(DrawSnow(&play) == 96 && NativeFogVisible(&play, SANDSTORM_A) && D_801F4E30 == 155);
    const auto autumnSnowhead = play.envCtx;
    Confirm(&play, SEASON_WINTER);
    assert(DrawSnow(&play) == 128 && play.envCtx.precipitation[PRECIP_SNOW_CUR] == 128);
    Confirm(&play, SEASON_AUTUMN);
    assert(DrawSnow(&play) == 96 && NativeFogVisible(&play, SANDSTORM_A));
    assert(std::memcmp(&autumnSnowhead, &play.envCtx, sizeof(autumnSnowhead)) == 0);
    Confirm(&play, SEASON_OFF);
    assert(play.envCtx.precipitation[PRECIP_SNOW_CUR] == 128 && play.envCtx.precipitation[PRECIP_SNOW_MAX] == 128);
    assert(play.envCtx.sandstormState == SANDSTORM_A && D_801F4E30 == 155 && DrawSnow(&play) > 0);
    assert(NativeFogVisible(&play, SANDSTORM_A));
    std::puts("PASS native Snowhead actor reuse, Spring/Summer masking, Autumn leaves/fog and live Off restoration");

    // Real Winter Fog tags write MAX only on exit. Native CUR/fog must never
    // become a saved seasonal count: continue its native decay and expose it on Off.
    Reset(&play, &camera, &gfx);
    ObjectKankyo tagSnow{};
    tagSnow.actor.id = ACTOR_OBJECT_KANKYO;
    tagSnow.actor.params = 1;
    tagSnow.actor.update = ObjectKankyo_Update;
    tagSnow.actor.draw = func_808DD3C8;
    ObjectKankyo_Init(&tagSnow.actor, &play);
    play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first = &tagSnow.actor;
    play.envCtx.precipitation[PRECIP_SNOW_CUR] = play.envCtx.precipitation[PRECIP_SNOW_MAX] = 128;
    play.envCtx.sandstormState = SANDSTORM_A;
    play.envCtx.sandstormPrimA = play.envCtx.sandstormEnvA = D_801F4E30 = 155;
    Confirm(&play, SEASON_WINTER);
    Frame(&play);
    assert(DrawSnow(&play) == 64 && play.envCtx.precipitation[PRECIP_SNOW_CUR] == 128);
    Player nativePlayer{};
    nativePlayer.actor.world.pos.x = 200;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &nativePlayer.actor;
    EnWeatherTag tag{};
    tag.actor.params = (1 << 8) | WEATHERTAG_TYPE_WINTERFOG;
    play.envCtx.lightMode = LIGHT_MODE_TIME;
    play.envCtx.lightConfig = play.envCtx.changeLightNextConfig = 2;
    func_80966E84(&tag, &play);
    assert(play.envCtx.precipitation[PRECIP_SNOW_MAX] == 0 && tag.actionFunc == func_80966E0C);
    assert(play.envCtx.precipitation[PRECIP_SNOW_CUR] == 128 && D_801F4E30 == 155);
    play.state.frames = 16;
    ObjectKankyo_Update(&tagSnow.actor, &play);
    EnvironmentFrame(&play);
    assert(play.envCtx.precipitation[PRECIP_SNOW_CUR] == 126 && DrawSnow(&play) == 64);
    const auto afterNativeTag = play.envCtx;
    Confirm(&play, SEASON_AUTUMN);
    assert(DrawSnow(&play) == 96 && NativeFogVisible(&play, SANDSTORM_A));
    assert(std::memcmp(&afterNativeTag, &play.envCtx, sizeof(afterNativeTag)) == 0);
    Confirm(&play, SEASON_OFF);
    assert(std::memcmp(&afterNativeTag, &play.envCtx, sizeof(afterNativeTag)) == 0);
    assert(play.envCtx.precipitation[PRECIP_SNOW_CUR] == 126 && DrawSnow(&play) == 126 && D_801F4E30 == 155);
    for (int i = 0; i < 64; ++i) {
        play.state.frames += 16;
        ObjectKankyo_Update(&tagSnow.actor, &play);
    }
    assert(play.envCtx.precipitation[PRECIP_SNOW_CUR] == 0);
    std::puts("PASS real native Winter Fog tag MAX-only exit, independent CUR/fog decay and exact Off state");

    Reset(&play, &camera, &gfx);
    EnTest4 dayActor{};
    for (int day : { 1, 2, 3 }) {
        gSaveContext.save.day = day;
        play.envCtx.precipitation[PRECIP_RAIN_MAX] = play.envCtx.precipitation[PRECIP_RAIN_CUR] = 0;
        dayActor.weather = THREEDAY_WEATHER_CLEAR;
        Confirm(&play, SEASON_SPRING);
        Frame(&play, &dayActor);
        if (day == 2) {
            // Native rain ramps its CUR index every eight frames.
            for (int i = 0; i < 8; ++i) Frame(&play, &dayActor);
        }
        int draws = rainDraws;
        DrawRainFromPlay(&play);
        assert(rainDraws == draws + 1);
        if (day == 2) {
            assert(gWeatherMode == WEATHER_MODE_RAIN && play.envCtx.precipitation[PRECIP_RAIN_MAX] == 60);
            assert(MMWeather_RainDensity() == 0 && rainGain == 0);
        } else {
            assert(MMWeather_RainDensity() == 30 && rainGain == 1);
        }
    }
    gSaveContext.save.day = 2;
    play.envCtx.precipitation[PRECIP_RAIN_MAX] = play.envCtx.precipitation[PRECIP_RAIN_CUR] = 60;
    play.envCtx.lightningState = LIGHTNING_ON;
    gWeatherMode = WEATHER_MODE_RAIN;
    Confirm(&play, SEASON_SUMMER);
    assert(nativeRainAmbience == 0 && nativeThunderAmbience == 0 && MMWeather_SeasonClearsRain());
    int draws = rainDraws;
    DrawRainFromPlay(&play);
    assert(rainDraws == draws);
    u8 first = 1, second = 1, blend = 255;
    MMWeather_ApplySky(&first, &second, &blend);
    assert(first == 0 && second == 0 && blend == 0);
    AdjLightSettings light{};
    EnvLightSettings settings[24]{};
    func_800F6CEC(&play, 4, &light, settings);
    assert(light.ambientColor[0] == 0 && light.light1Color[0] == 0);
    Frame(&play, &dayActor);
    assert(gWeatherMode == WEATHER_MODE_RAIN && play.envCtx.precipitation[PRECIP_RAIN_MAX] == 60);
    Confirm(&play, SEASON_AUTUMN);
    assert(nativeRainAmbience == 1 && nativeThunderAmbience == 1 && MMWeather_RainDensity() == 0);
    draws = rainDraws;
    DrawRainFromPlay(&play);
    assert(rainDraws == draws + 1);
    Confirm(&play, SEASON_SUMMER);
    gSaveContext.save.time = CLOCK_TIME(18, 0);
    Frame(&play, &dayActor); // real native schedule ends the storm
    assert(gWeatherMode == WEATHER_MODE_CLEAR && dayActor.weather == THREEDAY_WEATHER_CLEAR);
    while (play.envCtx.precipitation[PRECIP_RAIN_MAX] > 8) Frame(&play, &dayActor);
    assert(play.envCtx.precipitation[PRECIP_RAIN_MAX] == 8 && play.envCtx.precipitation[PRECIP_RAIN_CUR] > 0);
    assert(nativeRainAmbience == 0 && nativeThunderAmbience == 0);
    Confirm(&play, SEASON_OFF);
    assert(nativeRainAmbience == 0 && nativeThunderAmbience == 0); // actual native stop, despite nonzero counters
    Confirm(&play, SEASON_AUTUMN);
    assert(nativeRainAmbience == 0 && nativeThunderAmbience == 0);
    func_800F6CEC(&play, 4, &light, settings);
    gWeatherMode = WEATHER_MODE_RAIN;
    func_800F6CEC(&play, 4, &light, settings);
    assert(light.ambientColor[0] == -50 && light.light1Color[0] == -100);
    gSaveContext.save.day = 3;
    Confirm(&play, SEASON_SUMMER);
    assert(!MMWeather_SeasonClearsRain());
    draws = rainDraws;
    DrawRainFromPlay(&play);
    assert(rainDraws == draws + 1); // Summer does not erase other-day native weather
    std::puts("PASS real native Day 2 schedule: Spring Days 1/3, Summer clear sky/light/rain/ambience, Autumn/native and Off");

    // A direct actor query must respect the current scene/story/camera state
    // even while the cached rendering bridge still contains last frame's season.
    Reset(&play, &camera, &gfx);
    Confirm(&play, SEASON_WINTER);
    for (int restriction = 0; restriction < 9; ++restriction) {
        switch (restriction) {
            case 0: play.csCtx.state = 1; break;
            case 1: play.envCtx.lightSettingOverride = 1; break;
            case 2: play.envCtx.customSkyboxFilter = true; break;
            case 3: camera.stateFlags = CAM_STATE_UNDERWATER; break;
            case 4: play.skyboxId = SKYBOX_NONE; break;
            case 5:
                // A real scene reload starts with fresh environment and actors.
                play.sceneId = SCENE_SONCHONOIE;
                play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first = nullptr;
                play.envCtx.precipitation[PRECIP_SNOW_CUR] = play.envCtx.precipitation[PRECIP_SNOW_MAX] = 0;
                break;
            case 6: play.envCtx.skyboxDisabled = true; break;
            case 7: gSaveContext.gameMode = GAMEMODE_END_CREDITS; break;
            case 8: play.gameOverCtx.state = GAMEOVER_DEATH_START; break;
        }
        assert(MMWeather_SeasonForPlay(&play) == -1);
        EnvironmentFrame(&play);
        assert(MMWeather_Season() == -1 && DrawSnow(&play) == 0);
        play.csCtx.state = CS_STATE_IDLE;
        play.envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
        play.envCtx.customSkyboxFilter = play.envCtx.skyboxDisabled = false;
        camera.stateFlags = 0;
        play.skyboxId = SKYBOX_NORMAL_SKY;
        play.sceneId = SCENE_TOWN;
        gSaveContext.gameMode = GAMEMODE_NORMAL;
        play.gameOverCtx.state = GAMEOVER_INACTIVE;
        Frame(&play);
        assert(MMWeather_Season() == SEASON_WINTER && DrawSnow(&play) > 0);
    }
    Confirm(&play, SEASON_SPRING);
    Frame(&play);
    assert(DrawSnow(&play) == 0 && refreshes > 0);
    std::puts("PASS direct actor season query, story/interior/underwater restrictions and restoration");
}
