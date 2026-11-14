#include "ComboGameMode.h"
#include "functions.h"
#include "variables.h"
#include "macros.h"
#include "mods/transformation_masks/transformation_masks.h"
#include "mods/transformation_masks/assets/mm_asset_loader.h"
#include "mods/anim_translator/mm_anim_loader.h"
#include "mods/mm_sources/objects/object_link_goron.h"
#include "mods/mm_sources/objects/object_link_nuts.h"
#include "mods/mm_sources/objects/object_link_zora.h"
#include "objects/object_link_boy/object_link_boy.h"
#include "objects/object_link_child/object_link_child.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

static void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}
static bool Near(float a, float b) { return std::fabs(a - b) < 0.0001f; }

SaveContext gSaveContext{};
static GameInfo registers{};
GameInfo* gGameInfo = &registers;
PlayState* gPlayState;
uintptr_t gSegments[16]{};
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
static PlayState play;
static Player player;
static GraphicsContext graphics;
static Gfx displayBuffer[256];
static u32 sDisableAnimQueueFlags, sAnimQueueFlags = 1;

struct TestFormState {
    MmPlayerTransformation currentForm;
    SkelAnime formSkelAnime;
    /* PRODUCTION_GAKKI_FIELDS */
};
static TestFormState gFormState;
static u8 sKafeiWhistling, sKafeiNotePulse;
static struct { bool active; } sNativeMaskCs;

struct GameInteractor {
    struct OnOcarinaNote {};
    struct OnOcarinaPlaybackNote {};
    static GameInteractor* Instance;
    std::function<void(uint8_t, float, int8_t)> live;
    std::function<void(uint8_t, float)> replay;
    template <typename Hook, typename Fn> void RegisterGameHook(Fn callback) {
        if constexpr (std::is_same_v<Hook, OnOcarinaNote>) live = callback;
        else replay = callback;
    }
};
static GameInteractor events;
GameInteractor* GameInteractor::Instance = &events;
#define GameInteractor_Should(flag, fallback, ...) (fallback)
#define MMFORM_LOG(...) ((void)0)
static const char* CustomForms_ActiveSkin() { return nullptr; }

static u8 liveButton = OCARINA_BTN_INVALID, playbackButton = OCARINA_BTN_INVALID;
static bool instrumentMuted;
static u8 engineInstrument = OCARINA_INSTRUMENT_OFF;
extern "C" u8 AudioOcarina_GetCurrentButtonIndex() { return liveButton; }
extern "C" u8 AudioOcarina_GetPlaybackButtonIndex() { return playbackButton; }
extern "C" u8 AudioOcarina_IsInstrumentSfxMuted() { return instrumentMuted; }
extern "C" u8 AudioOcarina_GetInstrumentId() { return engineInstrument; }
static int noteStarts, noteStops, noteUpdates, instrumentChanges;
static u8 lastPitch;
static float lastBend;
static bool sounding;
static bool jingleSounding;
extern "C" void MmGakki_PlayPitch(s32, u8 pitch, f32 bend, Vec3f*) {
    noteStarts++;
    lastPitch = pitch;
    lastBend = bend;
    sounding = true;
}
extern "C" void MmGakki_UpdatePitch(u8 pitch, f32 bend) {
    noteUpdates++;
    lastPitch = pitch;
    lastBend = bend;
}
extern "C" void MmGakki_RefreshNote() { noteUpdates++; }
extern "C" void MmGakki_StopNote() { noteStops++; sounding = false; }
extern "C" void FormJingle_Stop() { jingleSounding = false; }
extern "C" void MmSfx_Stop(u16) { sounding = false; }
extern "C" void Audio_StopSfxById(u32) {}
extern "C" void AudioOcarina_SetInstrument(u8 instrument) { instrumentChanges++; engineInstrument = instrument; }
extern "C" s32 MmGakki_GetNativeInstrument(s32 form) {
    return form == MM_PLAYER_FORM_GERUDO ? 2 : form == MM_PLAYER_FORM_RITO ? 4 : 0;
}
extern "C" s32 MmGakki_GetVoiceType(s32 form) {
    if (form >= MM_PLAYER_FORM_GORON && form <= MM_PLAYER_FORM_DEKU) return GAKKI_VOICE_MM_FONT;
    if (form == MM_PLAYER_FORM_GERUDO || form == MM_PLAYER_FORM_RITO) return GAKKI_VOICE_NATIVE;
    return GAKKI_VOICE_NONE;
}
extern "C" s32 MmGakki_FormHasOwnInstrument(s32 form) { return MmGakki_GetVoiceType(form) != GAKKI_VOICE_NONE; }
extern "C" void Player_Action_8084E3C4(Player*, PlayState*) {}

static int ResourceMgr_OTRSigCheck(void* resource) {
    return resource != nullptr && std::memcmp(resource, "__OTR__", 7) == 0;
}
static void* ResourceMgr_LoadAnimByName(void*) { std::abort(); }
extern "C" s32 osRecvMesg(OSMesgQueue*, OSMesg*, s32) { return 0; }
extern "C" void AnimationContext_MoveActor(PlayState*, AnimationEntryData*) { std::abort(); }
extern "C" void SkelAnime_CopyFrameTable(SkelAnime*, Vec3s*, Vec3s*);
extern "C" s32 LinkAnimation_Loop(PlayState*, SkelAnime*);
extern "C" s32 LinkAnimation_Once(PlayState*, SkelAnime*);
extern "C" s32 LinkAnimation_Morph(PlayState*, SkelAnime*);
/* PRODUCTION_ANIMATION_QUEUE */

struct Clip {
    LinkAnimationHeader header{};
    std::vector<s16> data;
    Clip() = default;
    Clip(int frames, int tag) : data(frames * (PLAYER_LIMB_MAX * 3 + 1), 0) {
        header.common.frameCount = frames;
        header.segment = data.data();
        for (int frame = 0; frame < frames; frame++) {
            for (int limb = 0; limb < PLAYER_LIMB_MAX; limb++) {
                data[frame * (PLAYER_LIMB_MAX * 3 + 1) + limb * 3] = tag + frame;
                data[frame * (PLAYER_LIMB_MAX * 3 + 1) + limb * 3 + 1] = limb;
            }
        }
    }
};
static std::map<MmAnimId, Clip> clips;
static MmAnimId missing = MM_ANIM_MAX;
extern "C" LinkAnimationHeader* MmAnim_Load(MmAnimId id) {
    if (id == missing) return nullptr;
    auto found = clips.find(id);
    Require(found != clips.end(), "unexpected animation asset requested");
    return &found->second.header;
}
static void InitClips() {
    clips.clear();
    for (auto entry : {std::array<int, 3>{MM_ANIM_PG_GAKKISTART, 14, 10},
                       {MM_ANIM_PG_GAKKIPLAY, 18, 100}, {MM_ANIM_PG_GAKKIWAIT, 18, 200},
                       {MM_ANIM_PG_GAKKIPLAYA, 9, 300}, {MM_ANIM_PG_GAKKIPLAYL, 9, 400},
                       {MM_ANIM_PG_GAKKIPLAYD, 9, 500}, {MM_ANIM_PG_GAKKIPLAYU, 9, 600},
                       {MM_ANIM_PG_GAKKIPLAYR, 9, 700}, {MM_ANIM_PN_GAKKISTART, 12, 800},
                       {MM_ANIM_PN_GAKKIPLAY, 8, 900}, {MM_ANIM_PZ_GAKKISTART, 15, 1000},
                       {MM_ANIM_PZ_GAKKIPLAY, 7, 1100}}) {
        clips.try_emplace((MmAnimId)entry[0], entry[1], entry[2]);
    }
}

struct DrawCall { Gfx* displayList; Vec3f scale; };
static std::vector<DrawCall> draws;
static std::vector<Vec3f> matrixStack;
static Vec3f matrixScale{1, 1, 1};
static Mtx matrices[64];
static int matrixCount;
extern "C" void FrameInterpolation_RecordOpenChild(const void*, int) {}
extern "C" void FrameInterpolation_RecordCloseChild() {}
extern "C" void Graph_OpenDisps(Gfx**, GraphicsContext*, const char*, s32) {}
extern "C" void Graph_CloseDisps(Gfx**, GraphicsContext*, const char*, s32) {}
extern "C" void Matrix_Push() { matrixStack.push_back(matrixScale); }
extern "C" void Matrix_Pop() { matrixScale = matrixStack.back(); matrixStack.pop_back(); }
extern "C" void Matrix_Scale(f32 x, f32 y, f32 z, u8) {
    matrixScale.x *= x; matrixScale.y *= y; matrixScale.z *= z;
}
extern "C" Mtx* Matrix_NewMtx(GraphicsContext*, char*, s32) {
    Require(matrixCount < 64, "matrix output exceeded fixture capacity");
    return &matrices[matrixCount++];
}
extern "C" void gSPDisplayList(Gfx* packet, Gfx* displayList) {
    draws.push_back({displayList, matrixScale});
    __gSPDisplayList(packet, displayList);
}
static Gfx nativeGuitar[1], nativeGuitarHand[1], altGuitar[1], altGuitarHand[1];
static Gfx nativeAdultOcarina[1], nativeChildFairy[1], nativeChildOcarina[1];
static Gfx altAdultOcarina[1], altChildFairy[1], altChildOcarina[1];
static Gfx emptyFDHand[1], ordinaryZoraHand[1];
/* PRODUCTION_OCARINA_HAND_TABLES */
static bool altAssets;
extern "C" Gfx* ResourceMgr_LoadGfxByName(const char* path) {
    if (std::strcmp(path, gLinkZoraGuitarDL) == 0) return altAssets ? altGuitar : nativeGuitar;
    if (std::strcmp(path, gLinkZoraGuitarHandDL) == 0) return altAssets ? altGuitarHand : nativeGuitarHand;
    if (std::strcmp(path, gLinkAdultRightHandHoldingOotNearDL) == 0) return altAssets ? altAdultOcarina : nativeAdultOcarina;
    if (std::strcmp(path, gLinkChildRightHandHoldingFairyOcarinaNearDL) == 0) return altAssets ? altChildFairy : nativeChildFairy;
    if (std::strcmp(path, gLinkChildRightHandAndOotNearDL) == 0) return altAssets ? altChildOcarina : nativeChildOcarina;
    return (Gfx*)path;
}
enum FDHandDLIndex { FD_DL_LEFT_HAND_SWORD, FD_DL_LEFT_HAND_EMPTY, FD_DL_LEFT_HAND_BOTTLE, FD_DL_RIGHT_HAND_EMPTY };
static Gfx* MmForm_GetFDHandDL(PlayState*, FDHandDLIndex) { return emptyFDHand; }
static void MmForm_InterruptNativeMaskForScene(Player*, PlayState*, u8) {}
static void MmForm_CancelNativeMaskCutscene(u8) {}
static void MmForm_ClearCachedDLs() {}
static void MmForm_UnpinFormResources() {}
static void MmForm_FreeRootMotion() {}
static void MmForm_ResetDekuBubble(PlayState*) {}

/* PRODUCTION_KEYFRAMES */
/* PRODUCTION_GAKKI_DECLARATIONS */
/* PRODUCTION_GAKKI_FUNCTIONS */
static void UpdateInstrumentSession(Player* player, PlayState* play) {
    /* PRODUCTION_GAKKI_SESSION */
}
/* PRODUCTION_DESTROY_CLEANUP */

static void RunCleanup(const std::string& which) {
    Player* player = &::player;
    PlayState* play = &::play;
    if (which == "cleanup-damage") { /* PRODUCTION_DAMAGE_CLEANUP */ }
    else if (which == "cleanup-restore") { /* PRODUCTION_RESTORE_CLEANUP */ }
    else if (which == "cleanup-reload") { /* PRODUCTION_RELOAD_CLEANUP */ }
    else if (which == "cleanup-init") { /* PRODUCTION_INIT_CLEANUP */ }
    else if (which == "cleanup-reset") { /* PRODUCTION_RESET_CLEANUP */ }
    else if (which == "cleanup-death") { /* PRODUCTION_DEATH_CLEANUP */ }
    else if (which == "cleanup-destroy") MmForm_OnPlayDestroy(play, player);
    else Require(false, "unknown cleanup entry");
}

static void LoadInstrumentAnims() {
    if (gFormState.currentForm == MM_PLAYER_FORM_GORON) {
        /* PRODUCTION_LOAD_GORON */
    } else if (gFormState.currentForm == MM_PLAYER_FORM_ZORA) {
        /* PRODUCTION_LOAD_ZORA */
    } else if (gFormState.currentForm == MM_PLAYER_FORM_DEKU) {
        /* PRODUCTION_LOAD_DEKU */
    }
}
static void OverrideInstrumentHand(s32 limbIndex, Gfx** dList) {
    Player* player = &::player;
    PlayState* play = &::play;
    /* PRODUCTION_ZORA_HAND_OVERRIDE */
    /* PRODUCTION_FD_HAND_OVERRIDE */
}
static void DrawInstrument(s32 limbIndex) {
    Player* player = &::player;
    PlayState* play = &::play;
    draws.clear(); matrixCount = 0; matrixScale = {1, 1, 1};
    graphics.polyOpa.p = displayBuffer;
    /* PRODUCTION_INSTRUMENT_DRAW */
}
alignas(16) static Vec3s joints[PLAYER_LIMB_MAX + 1], morph[PLAYER_LIMB_MAX + 1];
static void DrainQueue() { AnimationContext_Update(&play, &play.animationCtx); }
static void Tick() { MmForm_UpdateGakki(&player, &play); DrainQueue(); }
static void TickSession() { UpdateInstrumentSession(&player, &play); DrainQueue(); }
static void Reset(MmPlayerTransformation form, MmAnimId missingAsset = MM_ANIM_MAX) {
    play = {}; player = {}; graphics = {}; gFormState = {}; registers = {}; gSaveContext = {};
    std::memset(joints, 0, sizeof(joints)); std::memset(morph, 0, sizeof(morph));
    gPlayState = &play; play.state.gfxCtx = &graphics;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    R_UPDATE_RATE = 2;
    sDisableAnimQueueFlags = 0; sAnimQueueFlags = 1;
    gFormState.currentForm = form;
    gFormState.formSkelAnime.limbCount = PLAYER_LIMB_MAX;
    gFormState.formSkelAnime.jointTable = joints;
    gFormState.formSkelAnime.morphTable = morph;
    gFormState.gakkiLastNoteIdx = OCARINA_PITCH_NONE;
    missing = missingAsset;
    LoadInstrumentAnims();
    noteStarts = noteStops = noteUpdates = instrumentChanges = 0;
    sounding = jingleSounding = instrumentMuted = altAssets = false;
    engineInstrument = OCARINA_INSTRUMENT_OFF;
    sNativeMaskCs.active = false;
    liveButton = playbackButton = OCARINA_BTN_INVALID;
    MmForm_EnterGakki(&player, &play);
    DrainQueue();
}
static void Open(MmPlayerTransformation form, MmAnimId missingAsset = MM_ANIM_MAX) {
    Reset(form, missingAsset);
    for (int frame = 0; gFormState.gakkiActive == 1 && frame < 40; frame++) Tick();
    Require(gFormState.gakkiActive == 2, "instrument opening failed to reach playable phase");
}
static void Live(u8 button, u8 pitch, float bend = 1.0f) {
    liveButton = button;
    Require(bool(events.live), "production live-note hook was not registered");
    events.live(pitch, bend, OCARINA_INSTRUMENT_DEFAULT);
}
static void Replay(u8 button, u8 pitch, float bend = 1.0f) {
    playbackButton = button;
    Require(bool(events.replay), "production replay-note hook was not registered");
    events.replay(pitch, bend);
}
static void CheckArm(int shoulder, int expected) {
    for (int limb = shoulder; limb <= shoulder + 2; limb++)
        Require(joints[limb].x == expected, "selected native strike did not reach exactly its arm joints");
}
static Gfx* ResolveDraw(Gfx* list) {
    if (std::memcmp((const char*)list, "__OTR__", 7) == 0) return ResourceMgr_LoadGfxByName((const char*)list);
    return list;
}

static void RecognizeSong(MmPlayerTransformation form, MmAnimId missingAsset = MM_ANIM_MAX) {
    Open(form, missingAsset);
    if (form == MM_PLAYER_FORM_GERUDO || form == MM_PLAYER_FORM_RITO) {
        // The real custom forms supply retargeted play clips through the resource table.
        // Use a tagged, Link-format clip at that resource boundary in this fixture.
        gFormState.gakkiPlayAnim = MmAnim_Load(MM_ANIM_PZ_GAKKIPLAY);
        MmForm_GakkiStartPlayAnimation(&play);
        DrainQueue();
    }
    play.msgCtx.ocarinaMode = OCARINA_MODE_01;
    play.msgCtx.ocarinaAction = OCARINA_ACTION_FREE_PLAY;
    play.msgCtx.msgMode = MSGMODE_SONG_PLAYED;
    TickSession();
}
static void RequirePerformance() {
    Require(gFormState.gakkiActive == 2, "successful song left instrument-out state before the player's decision");
    Require(gFormState.formSkelAnime.animation == gFormState.gakkiPlayAnim,
            "successful song did not select the form's full performance clip");
    Require(gFormState.formSkelAnime.mode == ANIMMODE_LOOP && gFormState.formSkelAnime.playSpeed > 0,
            "successful song froze its one-shot gesture instead of continuing the performance loop");
}
static void RequireSeveralPerformanceLoops() {
    RequirePerformance();
    int wraps = 0;
    bool poseChanged = false;
    auto& animation = gFormState.formSkelAnime;
    for (int i = 0; i < 80; i++) {
        float before = animation.curFrame;
        Vec3s previous = joints[PLAYER_LIMB_ROOT];
        TickSession();
        wraps += animation.curFrame < before;
        poseChanged |= joints[PLAYER_LIMB_ROOT].x != previous.x;
        RequirePerformance();
    }
    Require(wraps >= 3 && poseChanged, "performance did not keep advancing and wrapping through the production frame queue");
}

int main(int argc, char** argv) {
    Require(argc == 2, "one test case is required");
    InitClips();
    std::string test = argv[1];
    if (test == "performance-loop") {
        for (int updateRate : {2, 3}) {
            for (auto form : {MM_PLAYER_FORM_GORON, MM_PLAYER_FORM_ZORA, MM_PLAYER_FORM_DEKU,
                              MM_PLAYER_FORM_GERUDO, MM_PLAYER_FORM_RITO}) {
                RecognizeSong(form);
                R_UPDATE_RATE = updateRate; // Also cover the host's fractional 1.5-frame advance.
                RequireSeveralPerformanceLoops();
            }
        }
        RecognizeSong(MM_PLAYER_FORM_GORON, MM_ANIM_PG_GAKKIWAIT);
        RequireSeveralPerformanceLoops();
    } else if (test == "performance-result-dialogue") {
        for (auto form : {MM_PLAYER_FORM_GORON, MM_PLAYER_FORM_ZORA, MM_PLAYER_FORM_DEKU}) {
            RecognizeSong(form);
            for (auto mode : {MSGMODE_SETUP_DISPLAY_SONG_PLAYED, MSGMODE_DISPLAY_SONG_PLAYED,
                              MSGMODE_DISPLAY_SONG_PLAYED_TEXT_BEGIN, MSGMODE_DISPLAY_SONG_PLAYED_TEXT,
                              MSGMODE_SONG_PLAYED_ACT_BEGIN, MSGMODE_SONG_PLAYED_ACT,
                              MSGMODE_TEXT_START, MSGMODE_TEXT_DISPLAYING, MSGMODE_TEXT_DONE}) {
                play.msgCtx.msgMode = mode;
                // Message setup changes this action before it opens the ordinary result box.
                play.msgCtx.ocarinaAction = OCARINA_ACTION_FREE_PLAY_DONE;
                TickSession();
                RequirePerformance();
            }
            RequireSeveralPerformanceLoops();
        }
    } else if (test == "performance-callbacks") {
        for (auto form : {MM_PLAYER_FORM_GORON, MM_PLAYER_FORM_ZORA, MM_PLAYER_FORM_DEKU}) {
            RecognizeSong(form);
            play.msgCtx.msgMode = MSGMODE_DISPLAY_SONG_PLAYED;
            for (int i = 0; i < 8; i++) TickSession();
            float before = gFormState.formSkelAnime.curFrame;
            Vec3s pose = joints[PLAYER_LIMB_L_SHOULDER];
            // No player tick occurs during these callbacks: they must retain the pose,
            // including when native pause stops the owner update but audio emits a release.
            Replay(0, 2, 1); Replay(0, 2, 1.25f);
            Require(noteStarts == 2 && Near(lastBend, 1.25f), "performance merged repeated replay onsets or lost their bend");
            Replay(OCARINA_BTN_INVALID, OCARINA_PITCH_NONE);
            Require(!sounding, "performance swallowed the direct voice's replay release");
            instrumentMuted = true;
            Replay(3, 11);
            Require(noteStarts == 2 && !sounding, "performance layered a direct note over the muted song fanfare");
            Require(Near(gFormState.formSkelAnime.curFrame, before) && joints[PLAYER_LIMB_L_SHOULDER].x == pose.x,
                    "an audio callback advanced or restarted the performance outside its player update");
            RequirePerformance();
            DrainQueue();
            Require(joints[PLAYER_LIMB_L_SHOULDER].x == pose.x, "a replay callback queued a live arm gesture over the song performance");
            TickSession();
            RequirePerformance();
        }
    } else if (test == "performance-scales") {
        RecognizeSong(MM_PLAYER_FORM_GORON);
        RequirePerformance();
        play.msgCtx.msgMode = MSGMODE_DISPLAY_SONG_PLAYED;
        Replay(0, 2);
        while (gFormState.formSkelAnime.curFrame < 7) TickSession();
        int fullBodyFrame = 100 + (int)gFormState.formSkelAnime.curFrame;
        CheckArm(PLAYER_LIMB_L_SHOULDER, fullBodyFrame);
        CheckArm(PLAYER_LIMB_R_SHOULDER, fullBodyFrame);
        DrawInstrument(PLAYER_LIMB_TORSO);
        Require(draws.size() == 6, "performance hid a Goron drum");
        Require(!Near(draws[0].scale.x, 1), "Goron performance retained its static wait-container scale");
        for (int i = 1; i < 6; i++)
            Require(Near(draws[i].scale.x, draws[0].scale.x), "Goron performance retained an independent live-note drum deformation");
        RecognizeSong(MM_PLAYER_FORM_DEKU);
        RequirePerformance();
        // Entry starts the loop; two owner updates reach the native inflation keyframe.
        while (gFormState.formSkelAnime.curFrame < 2) TickSession();
        Replay(3, 11); DrainQueue();
        DrawInstrument(PLAYER_LIMB_HEAD);
        Require(draws.size() == 6 && Near(draws[0].scale.x, 1), "Deku performance lost its full-size pipe container");
        for (int i = 1; i < 6; i++)
            Require(Near(draws[i].scale.x, 1.2f), "Deku performance did not inflate all pipes with its native loop curve");
    } else if (test == "performance-decision") {
        for (auto mode : {OCARINA_MODE_02, OCARINA_MODE_04}) {
            RecognizeSong(MM_PLAYER_FORM_ZORA);
            play.msgCtx.msgMode = MSGMODE_TEXT_DONE;
            play.msgCtx.ocarinaAction = OCARINA_ACTION_FREE_PLAY_DONE;
            play.msgCtx.ocarinaMode = mode;
            player.stateFlags1 |= PLAYER_STATE1_IN_ITEM_CS;
            player.actionFunc = Player_Action_8084E3C4;
            player.av2.actionVar2 = 1;
            TickSession();
            Require(gFormState.gakkiActive == 3 && gFormState.formSkelAnime.playSpeed < 0,
                    "a resolved song decision retained the performance while the closing textbox remained up");
            auto exitAnimation = gFormState.formSkelAnime.animation;
            Replay(0, 2); DrainQueue();
            Require(gFormState.formSkelAnime.animation == exitAnimation, "a late replay note interrupted put-away after a song decision");
            for (int i = 0; i < 30; i++) TickSession();
            Require(!gFormState.gakkiActive && !sounding, "resolved decision re-entered or stranded the instrument session");
        }
    } else if (test == "performance-close") {
        RecognizeSong(MM_PLAYER_FORM_DEKU);
        play.msgCtx.msgMode = MSGMODE_NONE;
        player.actionFunc = nullptr;
        for (int i = 0; i < 30; i++) TickSession();
        Require(!gFormState.gakkiActive && !sounding, "a closed successful-song session kept performing");
        Open(MM_PLAYER_FORM_DEKU);
        play.msgCtx.msgMode = MSGMODE_TEXT_DONE;
        play.msgCtx.ocarinaMode = OCARINA_MODE_01;
        TickSession();
        Require(gFormState.formSkelAnime.mode == ANIMMODE_ONCE && Near(gFormState.formSkelAnime.playSpeed, 0),
                "a previous song's performance latch leaked into a new live instrument session");
    } else if (test == "performance-unrelated-text") {
        for (auto mode : {MSGMODE_OCARINA_PLAYING, MSGMODE_OCARINA_FAIL, MSGMODE_TEXT_DONE}) {
            Open(MM_PLAYER_FORM_DEKU);
            play.msgCtx.msgMode = mode;
            play.msgCtx.ocarinaMode = OCARINA_MODE_01;
            TickSession();
            Require(gFormState.formSkelAnime.mode == ANIMMODE_ONCE && Near(gFormState.formSkelAnime.playSpeed, 0),
                    "live input, failure, or unrelated dialogue started a successful-song loop");
        }
    } else if (test == "performance-cleanup") {
        for (const char* cleanup : {"cleanup-damage", "cleanup-restore", "cleanup-reload", "cleanup-init",
                                    "cleanup-reset", "cleanup-death", "cleanup-destroy"}) {
            RecognizeSong(MM_PLAYER_FORM_GORON); Replay(0, 2);
            RequirePerformance();
            RunCleanup(cleanup);
            Require(!gFormState.gakkiActive && !sounding, "lifecycle interruption left a recognized-song session active");
            play.msgCtx.msgMode = MSGMODE_OCARINA_PLAYING;
            play.msgCtx.ocarinaMode = OCARINA_MODE_01;
            LoadInstrumentAnims(); // Restore/reload legitimately discard the old form's assets.
            MmForm_EnterGakki(&player, &play);
            for (int i = 0; gFormState.gakkiActive == 1 && i < 40; i++) Tick();
            Require(gFormState.formSkelAnime.animation == gFormState.gakkiGoronWaitAnim,
                    (std::string("lifecycle cleanup leaked the successful-song loop into the next instrument use: ") + cleanup).c_str());
        }
    } else if (test == "opening-after-song") {
        Reset(MM_PLAYER_FORM_DEKU);
        MmForm_GakkiVoiceOff(); gFormState.gakkiActive = 0;
        // OoT resets the prior terminal mode only after the fresh intro finishes
        // and Message_StartOcarina opens the staff. That old result is not a cancel.
        play.msgCtx.ocarinaMode = OCARINA_MODE_04;
        player.stateFlags1 |= PLAYER_STATE1_IN_ITEM_CS;
        player.actionFunc = Player_Action_8084E3C4;
        player.av2.actionVar2 = 0;
        TickSession();
        Require(gFormState.gakkiActive == 1 && gFormState.formSkelAnime.playSpeed > 0,
                "a previous song's terminal mode prevented the next instrument opening");
        for (int i = 0; gFormState.gakkiActive == 1 && i < 40; i++) TickSession();
        Require(gFormState.gakkiActive == 2, "a previous terminal result reversed the new instrument intro");
        player.av2.actionVar2 = 1;
        play.msgCtx.ocarinaMode = OCARINA_MODE_01;
        play.msgCtx.msgMode = MSGMODE_OCARINA_PLAYING;
        Live(0, 2); TickSession();
        Require(sounding && noteStarts == 1 && gFormState.formSkelAnime.mode == ANIMMODE_ONCE,
                "a fresh instrument session did not retain ordinary live-note behavior");
    } else if (test == "opening-note") {
        for (auto form : {MM_PLAYER_FORM_GORON, MM_PLAYER_FORM_ZORA, MM_PLAYER_FORM_DEKU}) {
            Reset(form);
            auto start = gFormState.formSkelAnime.animation;
            Live(OCARINA_BTN_A, OCARINA_PITCH_D4);
            Require(gFormState.formSkelAnime.animation == start, "an early note interrupted the native opening animation");
            for (int i = 0; gFormState.gakkiActive == 1 && i < 40; i++) Tick();
            Require(gFormState.gakkiActive == 2, "early note stranded the native opening phase");
        }
    } else if (test == "goron-wait") {
        Open(MM_PLAYER_FORM_GORON);
        Require(gFormState.formSkelAnime.animation == MmAnim_Load(MM_ANIM_PG_GAKKIWAIT), "Goron did not use his native instrument wait clip");
        Require(gFormState.formSkelAnime.mode == ANIMMODE_LOOP && gFormState.formSkelAnime.playSpeed != 0, "Goron instrument wait must keep its native idle motion");
    } else if (test == "goron-five-buttons") {
        const int tags[] = {300, 400, 500, 600, 700};
        const u8 pitches[] = {2, 5, 9, 11, 14};
        for (u8 button = 0; button < 5; button++) {
            Open(MM_PLAYER_FORM_GORON);
            Live(button, pitches[button]); Tick();
            CheckArm(button < 3 ? PLAYER_LIMB_R_SHOULDER : PLAYER_LIMB_L_SHOULDER, tags[button] + 3);
            Require(joints[PLAYER_LIMB_ROOT].x < 300, "a drum strike replaced the base body's animation");
        }
    } else if (test == "goron-alternating-arms") {
        Open(MM_PLAYER_FORM_GORON);
        Live(0, 2); Tick();
        Live(3, 11); Tick();
        CheckArm(PLAYER_LIMB_R_SHOULDER, 304);
        CheckArm(PLAYER_LIMB_L_SHOULDER, 603);
        for (int i = 0; i < 8; i++) Tick();
        Require(joints[PLAYER_LIMB_R_SHOULDER].x < 300 && joints[PLAYER_LIMB_L_SHOULDER].x < 300, "completed strikes did not return their arms to native wait");
    } else if (test == "goron-drum-scales") {
        Open(MM_PLAYER_FORM_GORON);
        Live(2, 9); Tick(); Tick();
        DrawInstrument(PLAYER_LIMB_TORSO);
        Require(draws.size() == 6, "Goron must draw container and all five native drums");
        Require(Near(draws[0].scale.x, 1) && Near(draws[0].scale.y, 1), "a drum strike deformed the entire container");
        Require(Near(draws[3].scale.x, .9f) && Near(draws[3].scale.y, 1.1f), "selected drum did not use native strike deformation");
        Require(Near(draws[1].scale.x, 1) && Near(draws[2].scale.x, 1) && Near(draws[4].scale.x, 1) && Near(draws[5].scale.x, 1), "a drum strike deformed unselected skins");
    } else if (test == "deku-selected-pipe" || test == "deku-release") {
        Open(MM_PLAYER_FORM_DEKU);
        Live(3, 11); Tick();
        DrawInstrument(PLAYER_LIMB_HEAD);
        Require(draws.size() == 6, "Deku must draw container and five native pipes");
        Require(Near(draws[4].scale.x, 1.2f), "selected Deku pipe did not inflate");
        Require(Near(draws[1].scale.x, 1) && Near(draws[2].scale.x, 1) && Near(draws[3].scale.x, 1) && Near(draws[5].scale.x, 1), "Deku inflated unselected pipes");
        if (test == "deku-release") {
            Live(OCARINA_BTN_INVALID, OCARINA_PITCH_NONE); Tick();
            Require(Near(gFormState.gakkiPieceScales[3], 1.16f), "released pipe did not relax with native step");
            for (int i = 0; i < 6; i++) Tick();
            Require(Near(gFormState.gakkiPieceScales[3], 1), "released pipe did not settle to rest scale");
        }
    } else if (test == "pitch-modifier-button") {
        Open(MM_PLAYER_FORM_GORON);
        Live(2, 9); Tick();
        Live(2, 10); Tick();
        CheckArm(PLAYER_LIMB_R_SHOULDER, 504);
        Require(noteStarts == 2 && lastPitch == 10, "audio pitch change was lost while retaining button identity");
    } else if (test == "missing-wait" || test == "missing-strike") {
        Open(MM_PLAYER_FORM_GORON, test == "missing-wait" ? MM_ANIM_PG_GAKKIWAIT : MM_ANIM_PG_GAKKIPLAYL);
        Live(1, 5);
        for (int i = 0; i < 12; i++) Tick();
        Require(gFormState.gakkiActive == 2 && sounding, "missing optional native clip broke instrument use");
        MmForm_ExitGakki(&player, &play);
        for (int i = 0; gFormState.gakkiActive && i < 30; i++) Tick();
        Require(!gFormState.gakkiActive && !sounding, "missing clip stranded the controller or note at exit");
    } else if (test == "exit") {
        Open(MM_PLAYER_FORM_GORON); Live(0, 2); Tick();
        jingleSounding = true;
        MmForm_ExitGakki(&player, &play);
        auto exitAnim = gFormState.formSkelAnime.animation;
        Live(3, 11);
        Require(gFormState.formSkelAnime.animation == exitAnim, "a late note replaced the put-away animation");
        for (int i = 0; gFormState.gakkiActive && i < 30; i++) Tick();
        Require(!gFormState.gakkiActive && !sounding, "normal put-away failed to stop voice and finish");
        Require(jingleSounding, "ordinary instrument put-away cut off the recognized-song jingle");
    } else if (test == "custom-form") {
        Reset(MM_PLAYER_FORM_GERUDO);
        gFormState.gakkiPlayAnim = MmAnim_Load(MM_ANIM_PZ_GAKKIPLAY);
        Live(0, 2); DrainQueue();
        Require(gFormState.formSkelAnime.animation == gFormState.gakkiPlayAnim && gFormState.formSkelAnime.playSpeed == 1, "custom form's generic note gesture changed");
        Require(noteStarts == 0, "native OoT custom voice was replaced by an MM sample");
    } else if (test == "ordinary-voice") {
        for (auto form : {MM_PLAYER_FORM_HUMAN, MM_PLAYER_FORM_FIERCE_DEITY}) {
            Reset(form); gFormState.gakkiActive = 0;
            Live(0, 2); Replay(0, 2);
            Require(noteStarts == 0 && instrumentChanges == 0, "ordinary human/FD ocarina audio was overridden");
        }
    } else if (test == "native-voice-cleanup") {
        Reset(MM_PLAYER_FORM_GERUDO);
        MmForm_GakkiVoiceOff();
        Require(instrumentChanges == 0, "custom-form cleanup re-enabled an already-OFF ocarina");
        engineInstrument = OCARINA_INSTRUMENT_WHISTLE;
        MmForm_GakkiVoiceOff();
        Require(instrumentChanges == 0, "custom-form cleanup changed another owner's native instrument");
        engineInstrument = 2;
        gFormState.gakkiActive = 0;
        MmForm_GakkiVoiceOff();
        Require(instrumentChanges == 0, "inactive custom form reset a native instrument it did not own");
        gFormState.gakkiActive = 2;
        MmForm_GakkiVoiceOff();
        Require(instrumentChanges == 1 && engineInstrument == OCARINA_INSTRUMENT_DEFAULT, "active custom native voice was not restored on interruption");
    } else if (test == "replay-repeat") {
        Open(MM_PLAYER_FORM_GORON);
        Replay(0, 2); Tick(); Replay(0, 2); Tick();
        Require(noteStarts == 2, "separate equal-pitch playback onsets were merged into one note");
        CheckArm(PLAYER_LIMB_R_SHOULDER, 303);
    } else if (test == "held-bend") {
        Open(MM_PLAYER_FORM_ZORA);
        Live(0, 2, 1); Live(0, 2, 1.25f);
        Require(noteStarts == 1 && Near(lastBend, 1.25f), "held live note bend was not forwarded without restarting");
    } else if (test == "late-note-off") {
        Open(MM_PLAYER_FORM_GORON); Live(0, 2);
        gFormState.gakkiActive = 0;
        Replay(OCARINA_BTN_INVALID, OCARINA_PITCH_NONE);
        Require(!sounding, "an explicit note-off was discarded after the controller became inactive");
    } else if (test == "muted-replay") {
        Open(MM_PLAYER_FORM_GORON); instrumentMuted = true;
        Replay(0, 2); Tick();
        Require(noteStarts == 0 && !sounding, "muted native fanfare replay also started a direct form note");
        CheckArm(PLAYER_LIMB_R_SHOULDER, 303);
    } else if (test == "mute-held") {
        Open(MM_PLAYER_FORM_GORON); Live(0, 2);
        instrumentMuted = true; Tick();
        Require(!sounding, "native bank mute left an already-playing form note audible");
        instrumentMuted = false; Live(0, 2);
        Require(sounding && noteStarts == 2, "unmuted held note did not resume after direct voice was stopped");
    } else if (test == "zora-hand") {
        Open(MM_PLAYER_FORM_ZORA);
        for (bool alt : {false, true}) {
            altAssets = alt;
            Gfx* hand = ordinaryZoraHand;
            OverrideInstrumentHand(PLAYER_LIMB_L_HAND, &hand);
            Require(hand == nullptr, "ordinary Zora hand remained under the native guitar grip");
            DrawInstrument(PLAYER_LIMB_L_HAND);
            Require(draws.size() == 2, "Zora guitar and its native grip hand were not both emitted");
            Require(ResolveDraw(draws[0].displayList) == (alt ? altGuitar : nativeGuitar), "guitar bypassed current resource resolution");
            Require(ResolveDraw(draws[1].displayList) == (alt ? altGuitarHand : nativeGuitarHand), "guitar grip bypassed current resource resolution");
        }
    } else if (test == "zora-intro") {
        Reset(MM_PLAYER_FORM_ZORA); Tick();
        Gfx* hand = ordinaryZoraHand;
        OverrideInstrumentHand(PLAYER_LIMB_L_HAND, &hand);
        DrawInstrument(PLAYER_LIMB_L_HAND);
        Require(hand == ordinaryZoraHand && draws.empty(), "guitar appeared before native opening frame six");
    } else if (test == "fd-ocarina") {
        Reset(MM_PLAYER_FORM_FIERCE_DEITY);
        player.leftHandType = PLAYER_MODELTYPE_LH_OPEN;
        for (int age : {0, 1}) {
            gSaveContext.linkAge = age;
            for (auto type : {PLAYER_MODELTYPE_RH_OCARINA, PLAYER_MODELTYPE_RH_OOT}) {
                player.rightHandType = type;
                player.rightHandDLists = &(type == PLAYER_MODELTYPE_RH_OCARINA ? sPlayerRightHandOcarinaDLs : sPlayerRightHandOotDLs)[age];
                for (bool alt : {false, true}) {
                    altAssets = alt; Gfx* hand = emptyFDHand;
                    OverrideInstrumentHand(PLAYER_LIMB_R_HAND, &hand);
                    Gfx* expected = age == 0 ? (alt ? altAdultOcarina : nativeAdultOcarina)
                        : type == PLAYER_MODELTYPE_RH_OCARINA ? (alt ? altChildFairy : nativeChildFairy)
                        : (alt ? altChildOcarina : nativeChildOcarina);
                    Require(hand == expected, "FD lost the age/item-selected ordinary ocarina hand or its active Alt asset");
                }
            }
        }
    } else if (test.starts_with("cleanup-")) {
        Open(MM_PLAYER_FORM_GORON); Live(0, 2); Tick();
        jingleSounding = true;
        RunCleanup(test);
        Require(!sounding, "form lifecycle cleanup left its direct note playing");
        Require(test == "cleanup-damage" ? jingleSounding : !jingleSounding, "lifecycle cleanup used the wrong song-jingle ownership boundary");
    } else Require(false, "unknown test case");
    std::printf("PASS %s\n", argv[1]);
}
