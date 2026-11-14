#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

using u8 = unsigned char;
using s8 = signed char;
using s16 = short;
using u16 = unsigned short;
using s32 = int;
using Gfx = int;
enum MmPlayerTransformation {
    MM_PLAYER_FORM_FIERCE_DEITY,
    MM_PLAYER_FORM_GORON,
    MM_PLAYER_FORM_ZORA,
    MM_PLAYER_FORM_DEKU,
    MM_PLAYER_FORM_HUMAN,
    MM_PLAYER_FORM_PIKACHU,
    MM_PLAYER_FORM_GERUDO,
    MM_PLAYER_FORM_GARO,
    MM_PLAYER_FORM_RITO,
    MM_PLAYER_FORM_KEATON,
    MM_PLAYER_FORM_KAFEI
};
enum {
    MMFORM_STATE_INACTIVE,
    MMFORM_STATE_TRANSFORMING,
    MMFORM_STATE_ACTIVE,
    MMFORM_STATE_DETRANSFORMING,
    ITEM_MM_MASK_DEKU = 53,
    ITEM_MM_MASK_GORON = 59,
    ITEM_MM_MASK_ZORA = 65,
    MM_NA_SE_PL_ZORA_SWIM_LV = 1,
    MM_NA_SE_PL_ZORA_SPARK_BARRIER,
    SFX_FLAG = 0,
    ITEM_NONE = 255,
    GAMEMODE_NORMAL = 0,
    FS_BTN_SELECT_FILE_1 = 0,
    NA_SE_SY_FSEL_DECIDE_L = 0,
    ENTR_LOAD_OPENING = 1,
    ENTR_LINKS_HOUSE_CHILD_SPAWN = 2,
    NA_BGM_DISABLED = 255,
    TIMER_STATE_OFF = 0,
    SUBTIMER_STATE_OFF = 0,
    MAGIC_STATE_IDLE = 0,
    NA_BGM_GENERAL_SFX = 0,
    TRANS_NEXT_TYPE_DEFAULT = 0,
    BTN_ENABLED = 0,
    QUEST_NORMAL = 0,
    QUEST_RANDOMIZER = 1,
    QUEST_OOTXMM = 2,
    GAME_REGION_PAL = 1,
    LANGUAGE_JPN = 0,
    NAME_LANGUAGE_PAL = 1,
    NAME_LANGUAGE_NTSC_JPN = 2,
    NAME_LANGUAGE_NTSC_ENG = 3
};
#define MMFORM_LOG(...) ((void)0)
#define ARRAY_COUNT(a) (int(sizeof(a) / sizeof((a)[0])))
#define CVAR_ENHANCEMENT(x) x
#define SET_NEXT_GAMESTATE(...) ((void)0)
#define IS_OOTXMM false
#define IS_SPEEDRUN false
#define VT_FGCOL(x) ""
#define VT_RST ""
#define osSyncPrintf(...) ((void)0)

static void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}
struct ItemEquips {
    u8 buttonItems[8];
    u8 cButtonSlots[7];
    int equipment;
};
struct SaveContext {
    int fileNum, gameMode, respawnFlag, seqId, natureAmbienceId, showTitleCard, dogParams, timerState, subTimerState,
        eventInf[4], prevHudVisibilityMode, nayrusLoveTimer, healthAccumulator, magicState, prevMagicState, forcedSeqId,
        skyboxTime, nextTransitionType, nextCutsceneIndex, cutsceneTrigger, chamberCutsceneNum, nextDayTime,
        retainWeatherMode, buttonStatus[8], forceRisingButtonAlphas, nextHudVisibilityMode, hudVisibilityMode,
        hudVisibilityModeTimer, magicCapacity, magicFillTarget, magic, magicLevel, naviTimer, entranceIndex, linkAge,
        dayTime, cutsceneIndex, language, n64ddFlag;
    struct {
        int entranceIndex;
    } respawn[1];
    struct {
        int filenameLanguage;
        struct {
            int id;
        } quest;
    } ship;
    u8 playerName[8];
    ItemEquips equips;
    u8 inventory[72];
} gSaveContext;
struct GameState {
    bool running;
};
struct FileChooseContext {
    GameState state;
    int buttonIndex;
    u8 questType[3];
    int n64ddFlag, speedrunIndex;
};
struct SaveFileMetaInfo {
    int quest;
    u8 playerName[8];
};
struct Actor {
    int room;
};
struct Player {
    Actor actor;
    int* ageProperties;
    int itemAction, heldItemAction, heldItemId;
};
struct PlayState {
    int shootingGalleryStatus, bombchuBowlingStatus;
    void (*playerInit)(), (*playerUpdate)(), (*isPlayerDroppingFish)(), (*startPlayerFishing)(), (*grabPlayer)(),
        (*startPlayerCutscene)(), (*func_11D54)(), (*damagePlayer)(), (*talkWithPlayer)();
};
static PlayState* gPlayState;
static PlayState play;
static Player player;
static int sAgeProperties[2];
static struct {
    int state;
    MmPlayerTransformation currentForm, targetForm;
    int cutsceneTimer, cutscenePhase, flashAlpha;
    u8 initialized, gakkiActive, skeletonLoaded, meleeActive, zoraJumpTrailActive;
    void* savedAgeProperties;
    struct {
        void *jointTable, *morphTable;
    } formSkelAnime;
    struct {
        float *rootX[3], *rootZ[3];
        int frameCount[3], active;
    } rootMotion;
    int unrelatedTransient[12];
} gFormState;
static MmPlayerTransformation sPendingReactivateForm;
static u8 sPendingReactivate, sForceInstantTransform, sCutsceneMaskAnim, sPendingSoftReload, sEquipsSaved;
static s8 sFleetPendingForm = -1;
static ItemEquips sPreTransformEquips;
static u8 sRollOwnsPause, sNativeMaskInstantFallback;
static struct {
    bool active, removing;
    void* owner;
} sNativeMaskCs;
static struct {
    u8 hasPending;
    int damage, effect;
} gMmFormPendingDamage;
static u8 sDekuBubbleActive, sZoraBoomerangActive;
static bool extBackup, windHandles, formEnabled = true, combined = true, vanillaNewFile, allowPreflight = true;
static bool vanillaPreflight = true, debugQuest, gerudoAnims, ritoAnims, bowActive;
static int softReloads, nativeLoads, nativeCreates, saveWrites, nativeMaskEndCalls;
static std::string forcedModel;
static std::shared_ptr<std::vector<std::shared_ptr<int>>> sPinnedFormResources;
static Gfx *gZoraFinBoomerangLDL, *gZoraFinBoomerangRDL;
/* PRODUCTION_CACHE_DECLARATIONS */

// Safe external services release process-owned resources, never the scene/save.
static void MmForm_ResetDekuBubble(PlayState* play) {
    assert(play == nullptr);
}
static void MmForm_GakkiVoiceOff() {
}
static void FormJingle_Stop() {
}
static void FormJingle_Update() {
}
static void MmForm_StopGoronRollSfx() {
}
static void MmForm_RegisterGoronRollSfxCleanup() {
}
static void MmSfx_Stop(int) {
}
static void MmForm_StopNativeMaskSfx() {
}
static void MmForm_RitoWindClear() {
    windHandles = false;
}
static void MmForm_RitoRestoreLanding() {
    ritoAnims = false;
}
static void MmForm_GerudoRestoreAnims() {
    gerudoAnims = false;
}
static void MmForm_RitoBowReset() {
    bowActive = false;
}
static void MmForm_RitoResetFlight() {
    Require(!windHandles, "departed wind handles reached actor cleanup");
}
static void MmForm_ClearZoraSwimTrails(PlayState* value) {
    Require(value == nullptr, "departed trails used a scene");
}
static void ExtEquip_ClearTransformBackup() {
    extBackup = false;
}
static const char* O2rLoader_GetForcedName() {
    return forcedModel.empty() ? nullptr : forcedModel.c_str();
}
static void O2rLoader_ClearForcedModel() {
    forcedModel.clear();
}
static u8 MmAssets_IsAvailable() {
    return 1;
}
static u8 MmForm_IsEnabled() {
    return formEnabled;
}
static u8 RitoItem_FormEnabled() {
    return 1;
}
static bool WolfLinkForm_IsSelected() {
    return false;
}
static void WolfLinkForm_Cleanup() {
}
static void PikachuForm_Cleanup() {
}
static bool MaskProgression_CanTransform(int) {
    return true;
}
static int CVarGetInteger(const char* key, int fallback) {
    return std::string(key) == "gCombo.NewFileVanilla" ? vanillaNewFile : fallback;
}
static void CVarSetInteger(const char*, int) {
}
static bool MmForm_IsNativeMaskForm(MmPlayerTransformation form) {
    return form < MM_PLAYER_FORM_HUMAN;
}
static void MmForm_SaveAndRestrictEquips(PlayState*) {
    gSaveContext.equips.buttonItems[1] = 90;
}
static void MmForm_RestoreEquips(PlayState*) {
    gSaveContext.equips = sPreTransformEquips;
}
static void ExtEquip_RestoreFromTransform() {
    gSaveContext.equips.equipment = 91;
}
static void MmForm_EndNativeMaskCutscene(Player*, PlayState*) {
    ++nativeMaskEndCalls;
    sNativeMaskCs = {};
}
static bool MmForm_SoftReload(PlayState*, Player*, MmPlayerTransformation) {
    ++softReloads;
    gFormState.skeletonLoaded = 1;
    return true;
}
static void MmForm_RestoreMeleeColliders(Player*) {
}
/* PRODUCTION_FORM_FUNCTIONS */

static SaveFileMetaInfo metadata[3];
static bool ComboStartupOot_PrepareContextChange() {
    return allowPreflight;
}
static bool Combo_IsCombinedGame() {
    return combined;
}
static bool Ship_QuestDebugEnabled(int) {
    return debugQuest;
}
static SaveFileMetaInfo* Save_GetSaveMetaInfo(int file) {
    return &metadata[file];
}
static void RequireCleanEntry() {
    Require(gFormState.state == MMFORM_STATE_INACTIVE && gFormState.currentForm == MM_PLAYER_FORM_HUMAN,
            "real save-session entry retained departed form");
    Require(sFleetPendingForm == -1 && !sPendingSoftReload && !sEquipsSaved && !sNativeMaskCs.active,
            "real save-session entry retained pending form or old equip/cutscene ownership");
}
static void SetNativeSave() {
    gSaveContext.equips = {};
    std::fill(std::begin(gSaveContext.equips.buttonItems), std::end(gSaveContext.equips.buttonItems), u8(41));
    gSaveContext.equips.equipment = 42;
    std::fill(std::begin(gSaveContext.inventory), std::end(gSaveContext.inventory), u8(43));
}
static void Sram_OpenSave() {
    RequireCleanEntry();
    ++nativeLoads;
    SetNativeSave();
}
static void Sram_InitNewSave() {
    RequireCleanEntry();
    ++nativeCreates;
    SetNativeSave();
}
static void Sram_InitDebugSave() {
    Sram_InitNewSave();
}
static void Sram_InitBossRushSave() {
    Sram_InitNewSave();
}
static void Randomizer_WaitForGeneration() {
}
static void Audio_PlaySfxGeneral(int, int*, int, int*, int*, int*) {
}
static int gSfxDefaultPos, gSfxDefaultFreqAndVolScale, gSfxDefaultReverb;
static void FleetComboFS_OnLoadSave(int) {
}
static void GameInteractor_ExecuteOnLoadGame(int) {
}
static int ResourceMgr_GetGameRegion(int) {
    return 0;
}
static bool Randomizer_IsSeedGenerated() {
    return true;
}
static bool Randomizer_IsSpoilerLoaded() {
    return false;
}
static void Randomizer_InitSaveFile() {
    gSaveContext.inventory[0] = 44;
}
static void FleetComboFS_OnCreateSave(int) {
}
static void Speedrun_InitSaveFile(int) {
}
static void Save_SaveFile() {
    ++saveWrites;
}
static void SaveManager_ThreadPoolWait() {
}
static bool VanillaCreate(int, int stage) {
    return stage != 0 || vanillaPreflight;
}
static int gComboVanillaCreationFailed, gComboVanillaMode;
static bool (*gComboVanillaCreateCallback)(int, int) = VanillaCreate;
static void (*gComboGenerateCallback)(int) = nullptr;
static void (*gComboSaveInitCallback)(int) = nullptr;
/* PRODUCTION_SELECT */
/* PRODUCTION_CREATE */

static void DinFireShield_Reset() {
}
static void DinFireSword_Reset() {
}
static void Player_InitCommon() {
}
static void Player_UpdateCommon() {
}
static void Player_IsDroppingFish() {
}
static void Player_StartFishing() {
}
static void func_80852F38() {
}
static void Player_TryCsAction() {
}
static void func_80853080() {
}
static void Player_InflictDamage() {
}
static void Player_StartTalking() {
}
static void ExtEquip_Init() {
}
static void Sw97_MigrateLayout(PlayState*) {
}
static void TransformMasks_Init(PlayState* play, Player* player) {
    Require(!sDekuBubbleActive && !sZoraBoomerangActive, "Player_Init retained departed form aim aliases");
    MmForm_Init(play, player);
}
/* PRODUCTION_PLAYER_INIT */

static void ArmDeparted(MmPlayerTransformation form = MM_PLAYER_FORM_FIERCE_DEITY) {
    gFormState = {};
    gFormState.state = MMFORM_STATE_ACTIVE;
    gFormState.currentForm = form;
    gFormState.targetForm = form;
    gFormState.initialized = 1;
    gFormState.skeletonLoaded = 1;
    gFormState.savedAgeProperties = reinterpret_cast<void*>(1);
    gFormState.formSkelAnime.jointTable = reinterpret_cast<void*>(1);
    gFormState.formSkelAnime.morphTable = reinterpret_cast<void*>(1);
    gFormState.unrelatedTransient[3] = 29;
    gFormState.rootMotion.rootX[0] = static_cast<float*>(std::malloc(16));
    gFormState.rootMotion.rootZ[0] = static_cast<float*>(std::malloc(16));
    gFormState.rootMotion.frameCount[0] = 4;
    gFormState.rootMotion.active = 1;
    sPendingReactivateForm = form;
    sPendingReactivate = sPendingSoftReload = sForceInstantTransform = sCutsceneMaskAnim = 1;
    sFleetPendingForm = form;
    sEquipsSaved = 1;
    std::memset(&sPreTransformEquips, 77, sizeof(sPreTransformEquips));
    sRollOwnsPause = sNativeMaskInstantFallback = 1;
    gMmFormPendingDamage = { 1, 8, 9 };
    extBackup = windHandles = gerudoAnims = ritoAnims = bowActive = true;
    sPinnedFormResources = std::make_shared<std::vector<std::shared_ptr<int>>>(1, std::make_shared<int>(7));
    sCachedFDHandDLs[0] = reinterpret_cast<Gfx*>(1);
    sFDHandDLCounts[0] = 1;
    sFDHandDLSafeCopies[0].push_back(7);
    gZoraFinBoomerangLDL = gZoraFinBoomerangRDL = reinterpret_cast<Gfx*>(1);
    // A poisoned global scene catches an accidental native/effect cleanup.
    gPlayState = reinterpret_cast<PlayState*>(1);
    gSaveContext.fileNum = 0;
    gSaveContext.linkAge = 0;
    SetNativeSave();
}
static void RequireHuman() {
    RequireCleanEntry();
    Require(gFormState.targetForm == MM_PLAYER_FORM_HUMAN && !gFormState.skeletonLoaded &&
                !gFormState.savedAgeProperties && !gFormState.formSkelAnime.jointTable &&
                !gFormState.formSkelAnime.morphTable,
            "discard retained actor/arena state");
    Require(!sPendingReactivate && !sForceInstantTransform && !sCutsceneMaskAnim && !sRollOwnsPause &&
                !sNativeMaskInstantFallback && !gMmFormPendingDamage.hasPending && !gFormState.unrelatedTransient[3],
            "discard retained transient ownership");
    Require(!extBackup && !windHandles && !gerudoAnims && !ritoAnims && !bowActive && !sPinnedFormResources,
            "discard retained owned equipment/resource/animation state");
    Require(!sCachedFDHandDLs[0] && !sFDHandDLCounts[0] && sFDHandDLSafeCopies[0].empty() && !gZoraFinBoomerangLDL &&
                !gZoraFinBoomerangRDL,
            "discard retained display-list caches");
}
static void RequireNativeSave() {
    Require(gSaveContext.equips.buttonItems[1] == 41 && gSaveContext.equips.equipment == 42 &&
                gSaveContext.inventory[1] == 43,
            "departed equip restore contaminated native save");
}
int main(int argc, char** argv) {
    Require(argc == 2, "case required");
    const std::string test = argv[1];
    ArmDeparted();
    if (test == "scene") {
        sFleetPendingForm = -1;
        sPendingSoftReload = 0;
        sPendingReactivate = 0;
        MmForm_Init(&play, &player);
        MmForm_Update(&play, &player);
        Require(gFormState.state == MMFORM_STATE_ACTIVE && gFormState.currentForm == MM_PLAYER_FORM_FIERCE_DEITY &&
                    softReloads == 1 && sEquipsSaved && extBackup,
                "same-session scene lost form or equipment backup");
    } else if (test == "aim") {
        sDekuBubbleActive = sZoraBoomerangActive = 1;
        Player_Init(&player.actor, &play);
        Require(!sDekuBubbleActive && !sZoraBoomerangActive, "new player inherited form aim aliases");
        Require(gFormState.currentForm == MM_PLAYER_FORM_FIERCE_DEITY, "per-player aim reset discarded logical form");
    } else if (test == "file-switch" || test == "same-slot-load") {
        FileChooseContext file{};
        file.buttonIndex = test == "file-switch" ? 2 : 0;
        FileChoose_LoadGame(&file.state);
        Require(nativeLoads == 1 && gSaveContext.fileNum == file.buttonIndex, "file selection failed to load target");
        MmForm_Init(&play, &player);
        MmForm_Update(&play, &player);
        RequireHuman();
        RequireNativeSave();
    } else if (test == "new-native" || test == "new-rando" || test == "same-slot-recreate") {
        FileChooseContext file{};
        file.buttonIndex = test == "same-slot-recreate" ? 0 : 2;
        vanillaNewFile = test == "new-native";
        file.questType[file.buttonIndex] = test == "new-rando" ? QUEST_RANDOMIZER : QUEST_NORMAL;
        Sram_InitSave(&file);
        Require(nativeCreates == 1 && saveWrites == 1, "creation failed to initialize/persist native save");
        MmForm_Init(&play, &player);
        MmForm_Update(&play, &player);
        RequireHuman();
        RequireNativeSave();
    } else if (test == "preflight-refusal") {
#ifdef COMBO_BUILD
        FileChooseContext file{};
        file.buttonIndex = 2;
        allowPreflight = false;
        FileChoose_LoadGame(&file.state);
        Require(nativeLoads == 0 && gFormState.state == MMFORM_STATE_ACTIVE && sEquipsSaved,
                "refused file load discarded still-owned form");
        vanillaNewFile = true;
        vanillaPreflight = false;
        Sram_InitSave(&file);
        Require(nativeCreates == 0 && gFormState.state == MMFORM_STATE_ACTIVE && sEquipsSaved,
                "refused creation discarded still-owned form");
#endif
    } else {
        if (test == "native-before-flash" || test == "native-after-flash") {
            sNativeMaskCs = { true, false, reinterpret_cast<void*>(1) };
            if (test == "native-before-flash") {
                gFormState.currentForm = MM_PLAYER_FORM_HUMAN;
                gFormState.skeletonLoaded = 0;
            }
            gFormState.state = MMFORM_STATE_TRANSFORMING;
        }
        if (test == "pending") {
            gFormState.state = MMFORM_STATE_INACTIVE;
            gFormState.currentForm = MM_PLAYER_FORM_HUMAN;
        }
        if (test == "owned-model")
            forcedModel = "rito";
        if (test == "external-model")
            forcedModel = "user-custom-skin";
        const SaveContext before = gSaveContext;
        MmForm_DiscardSaveSession();
        RequireHuman();
        Require(std::memcmp(&before, &gSaveContext, sizeof(before)) == 0, "discard wrote the live native save");
        Require(gPlayState == reinterpret_cast<PlayState*>(1) && !nativeMaskEndCalls,
                "discard touched departed scene ownership");
        if (test == "owned-model")
            Require(forcedModel.empty(), "departed form kept its forced model");
        if (test == "external-model")
            Require(forcedModel == "user-custom-skin", "discard removed an independent model selection");
        if (test == "fleet")
            MmForm_FleetApplyForm(MM_PLAYER_FORM_ZORA);
        MmForm_Init(&play, &player);
        MmForm_Update(&play, &player);
        RequireNativeSave();
        if (test == "fleet") {
            Require(gFormState.state == MMFORM_STATE_ACTIVE && gFormState.currentForm == MM_PLAYER_FORM_ZORA &&
                        softReloads == 1 && sFleetPendingForm == -1,
                    "new session rejected legitimate Fleet form");
        } else {
            RequireHuman();
            Require(softReloads == 0, "discarded session reactivated a stale form");
        }
    }
    // The scene/aim/refusal controls deliberately retain their same-session heap.
    MmForm_FreeRootMotion();
    std::printf("PASS %s\n", test.c_str());
}
