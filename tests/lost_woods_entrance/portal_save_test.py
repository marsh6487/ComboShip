#!/usr/bin/env python3
"""Round-trip MM portal and reset saves through the real JSON converters.

Container I/O, migration and item-audit logging are explicit boundaries. Native
scene-flag capture and SaveContext serialization/deserialization execute unchanged.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import block_from

parser = argparse.ArgumentParser()
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()
production = (ROOT / 'mm/2s2h/SaveManager/SaveManager.cpp').read_text()
play = (ROOT / 'mm/src/code/z_play.c').read_text()
sram = (ROOT / 'mm/src/code/z_sram_NES.c').read_text()
saving = (ROOT / 'mm/2s2h/Enhancements/Saving/SavingEnhancements.cpp').read_text()
autosave = (ROOT / 'mm/2s2h/Enhancements/Saving/AutoSave.cpp').read_text()


def function(text, marker):
    return block_from(text, text.index(marker))


source = r'''
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <type_traits>
#include "global.h"
#include "BenJsonConversions.hpp"
#include "ComboGameMode.h"
#include "Enhancements/Saving/BetterSaveMenu.h"
#include "Enhancements/Saving/SavingEnhancements.h"
#include "CustomMessage/CustomMessage.h"
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
char gGitCommitHash[] = "portal-save-fixture";
}
#define SPDLOG_INFO(...) ((void)0)
#define SPDLOG_ERROR(...) ((void)0)
SaveContext gSaveContext{};
PlayState* gPlayState;
GameState* gGameState;
int gComboOwlBlobSlot = -1;
static nlohmann::json disk;
static int writes, failures;
static int flashWrites, flushes;
static bool rememberLocation, autosaveEnabled, betterSaveEnabled, canSave = true;
s32 gFlashOwlSaveStartPages[] = {1, 129, 2, 130, 3, 131};
s32 gFlashOwlSaveNumPages[] = {1, 1, 1, 1, 1, 1};
extern "C" int32_t CVarGetInteger(const char* key, int32_t fallback) {
    if (std::strcmp(key, "gEnhancements.Saving.RememberSaveLocation") == 0) return rememberLocation;
    if (std::strcmp(key, "gEnhancements.Saving.Autosave") == 0) return autosaveEnabled;
    if (std::strcmp(key, "gEnhancements.Saving.BetterSaveMenu") == 0) return betterSaveEnabled;
    return fallback;
}
extern "C" bool SavingEnhancements_CanSave() { return canSave; }
extern "C" void SavingEnhancements_AdvancePlaytime() {}
extern "C" void Combo_MM_FlushAccumulators() { ++flushes; }
extern "C" void Sram_SetFlashPagesOwlSave(SramContext* sram, s32 page, s32 count) {
    sram->curPage = page;
    sram->numPages = count;
}
extern "C" void Sram_StartWriteToFlashOwlSave(SramContext* sram) {
    SysFlashrom_WriteDataSync(sram->saveBuf, sram->curPage, sram->numPages);
    sram->status = 7;
}
extern "C" void SysFlashrom_WriteDataSync(void* buffer, u32 page, u32 count) {
    if (page != (flashWrites == 0 ? 2U : 130U) || count != 1U) std::abort();
    SaveContext snapshot{};
    std::memcpy(&snapshot, buffer, offsetof(SaveContext, fileNum));
    disk["owlSave"] = snapshot;
    disk.erase("comboPortalSession");
    ++flashWrites;
}
namespace ItemGrantAudit { struct Scope { explicit Scope(const char*) {} }; }
namespace ComboRando::CrossDungeon { int ReadCheckpoint(const nlohmann::json&) { return -1; } }
// This fixture exercises the existing seeded container path.
extern "C" int Combo_QueryVanillaMode(int) { return 0; }
void Combo_MM_LoadCrossDungeonCheckpoint(int) {}
void ComboBlackMarket_LoadState(const nlohmann::json&, int) {} // market persistence has its own production fixture
static std::string SaveManager_GetFileName(int slot) { return "file" + std::to_string(slot) + ".json"; }
static int SaveManager_ReadSaveFile(const std::filesystem::path&,nlohmann::json& value) { value=disk; return 0; }
static int SaveManager_MigrateSave(nlohmann::json&) { return 0; }
static void SaveManager_WriteSaveFile(const std::filesystem::path&,nlohmann::json& value) { disk=value; ++writes; }
static int SaveManager_LoadFailedForCombo(int code) { ++failures; return code; }
static void require(bool value,const char* message) {
    if (!value) { std::fprintf(stderr,"FAIL: %s\n",message); std::exit(1); }
}

static std::string prompt;
void CustomMessage::SetActiveCustomMessage(std::string text, Entry) { prompt = text; }
extern "C" {
s32 gHorseIsMounted;
u32 gBitFlags[32];
SceneEntranceTableEntry sSceneEntranceTable[ENTR_SCENE_MAX]{};
s32 gSceneEntranceTableCount = ENTR_SCENE_MAX;
s32 Entrance_GetSceneIdAbsolute(u16 entrance) {
    return sSceneEntranceTable[entrance >> 9].table[(entrance >> 4) & 0x1F][entrance & 0xF].sceneId;
}
void Message_StartTextbox(PlayState* play, u16 id, Actor*) {
    play->msgCtx.currentTextId = id;
    play->msgCtx.msgMode = MSGMODE_TEXT_DONE;
    play->msgCtx.textboxEndType = prompt.ends_with('\xC3') ? TEXTBOX_ENDTYPE_THREE_CHOICE : TEXTBOX_ENDTYPE_TWO_CHOICE;
}
void Message_CloseTextbox(PlayState* play) {
    play->msgCtx.msgMode = MSGMODE_TEXT_CLOSING;
    play->msgCtx.textboxEndType = TEXTBOX_ENDTYPE_DEFAULT;
}
u8 Message_GetState(MessageContext* ctx) {
    return ctx->msgMode == MSGMODE_TEXT_DONE ? TEXT_STATE_CHOICE : TEXT_STATE_NONE;
}
bool Message_ShouldAdvance(PlayState* play) { return CONTROLLER1(&play->state)->press.button & BTN_A; }
void Interface_SetAButtonDoAction(PlayState*, u16) {}
void Audio_PlaySfx(u16) {}
void Audio_PlaySfx_MessageDecide() {}
void AudioOcarina_SetInstrument(u8) {}
}
'''
source += re.search(r'const uint32_t CURRENT_SAVE_VERSION = \d+;', production).group(0) + '\n'
source += function(play, 's16 Play_GetOriginalSceneId(')
source += function(play, 'void Play_SaveCycleSceneFlags(').replace('PlayState* this', 'PlayState* self').replace('this->', 'self->')
source += function(sram, 'u16 Sram_CalcChecksum(').replace(
    'u8* dataPtr = data;', 'u8* dataPtr = (u8*)data;')
source += function(sram, 'void func_8014546C(')
for marker in ('static bool SaveManager_CapturePortalSession(', 'static void SaveManager_RestorePortalRespawns('):
    if marker in production:
        source += function(production, marker)
source += function(production, 'void SaveManager_SaveCurrentForCombo(')
source += function(production, 'int SaveManager_LoadSaveFile(')
title = (ROOT / 'mm/src/code/title_setup.c').read_text()
source += 'void RestoreCycleFlags() {\n' + function(title, 'for (int i = 0; i < ARRAY_COUNT(gSaveContext.cycleSceneFlags); i++)') + '\n}\n'
source += function(saving, 'void LoadRespawnData(')
source += '\n' + '\n'.join(line for line in saving.splitlines() if line.startswith('#define CVAR_REMEMBER_SAVE_LOCATION')) + '\n'
source += '\n'.join(line for line in autosave.splitlines() if line.startswith('#define CVAR_AUTOSAVE')) + '\n'
source += 'static int lastScene, entranceToSave;\n'
source += function(saving, 'extern "C" void SavingEnhancements_PersistSaveEntranceInfo(')
source += function(saving, 'extern "C" void SavingEnhancements_ClearSaveEntranceInfo(')
arrival = 'COND_HOOK(OnSceneInit, true, [](s8 sceneId, s8)'
source += 'void ObserveScene(s8 sceneId)' + function(saving, arrival).split(arrival, 1)[1]
source += function(saving, 'extern "C" void SavingEnhancements_WriteOwlSnapshot(')
source += function(autosave, 'extern "C" void SavingEnhancements_AutosaveOnReset(')
source += r'''
template<class Function> void SavePortalSession(Function save) {
    if constexpr (std::is_invocable_v<Function,bool>) save(true);
    else save(); // permits executing the actual pre-fix writer as the negative control
}
template<class Function> void SaveWithoutPortalSession(Function save) {
    if constexpr (std::is_invocable_v<Function,bool>) save(false);
    else save();
}
static void CheckBetterSave(PlayState& play, int selected, bool validEntrance) {
    gSaveContext = {};
    play = {};
    betterSaveEnabled = true;
    rememberLocation = true;
    canSave = true;
    flashWrites = 0;
    gSaveContext.fileNum = 1;
    gSaveContext.save.day = 3;
    gSaveContext.save.time = 0xABCD;
    gSaveContext.save.saveInfo.playerData.rupees = 84;
    std::memcpy(gSaveContext.save.saveInfo.playerData.newf, "ZELDA3", 6);
    gSaveContext.respawn[RESPAWN_MODE_RETURN].entrance = ENTRANCE(TERMINA_FIELD, 0);

    static EntranceTableEntry spawn[] = { { SCENE_00KEIKOKU, 6, 0 } };
    static EntranceTableEntry* entrances[7] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, spawn };
    sSceneEntranceTable[ENTR_SCENE_TERMINA_FIELD].tableCount = ARRAY_COUNT(entrances);
    sSceneEntranceTable[ENTR_SCENE_TERMINA_FIELD].table = entrances;
    const s32 entrance = ENTRANCE(TERMINA_FIELD, 6);
    gSaveContext.save.entrance = validEntrance ? entrance + 3 : ENTRANCE(TERMINA_FIELD, 7);
    entranceToSave = gSaveContext.save.entrance;
    play.sceneId = SCENE_00KEIKOKU;
    play.actorCtx.sceneFlags.chest = 0x91;
    static Player player{};
    static Actor horse{};
    player.rideActor = &horse;
    horse.world.pos = { 11, 22, 33 };
    horse.shape.rot.y = 42;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    gHorseIsMounted = true;
    gBitFlags[QUEST_SONG_EPONA] = 1U << QUEST_SONG_EPONA;
    SET_QUEST_ITEM(QUEST_SONG_EPONA);
    play.pauseCtx.state = PAUSE_STATE_MAIN;
    static u8 buffer[SAVE_BUFFER_SIZE];
    play.sramCtx.saveBuf = buffer;
    auto& press = CONTROLLER1(&play.state)->press.button;

    press = BTN_B;
    betterSaveEnabled = false;
    require(BetterSaveMenu_Update(&play) == MM_BETTER_SAVE_INACTIVE, "disabled menu must leave pause handling alone");
    betterSaveEnabled = true;
    play.sramCtx.status = 7;
    require(BetterSaveMenu_Update(&play) == MM_BETTER_SAVE_INACTIVE, "must wait for an existing save");
    play.sramCtx.status = 0;
    require(BetterSaveMenu_Update(&play) == MM_BETTER_SAVE_HANDLED && prompt.ends_with('\xC2'),
            "B opens a two-choice save prompt");
    press = BTN_A;
    if (selected < 0) {
        play.msgCtx.choiceIndex = 1;
        require(BetterSaveMenu_Update(&play) == MM_BETTER_SAVE_CLOSE_PAUSE && flashWrites == 0,
                "No closes the prompt without writing");
        return;
    }
    require(BetterSaveMenu_Update(&play) == MM_BETTER_SAVE_HANDLED && flashWrites == 1,
            "Yes writes one current-cycle owl snapshot");
    const auto saved = disk.at("owlSave").get<SaveContext>();
    require(saved.save.day == 3 && saved.save.time == 0xABCD && saved.save.saveInfo.playerData.rupees == 84 &&
            saved.save.shipSaveInfo.pauseSaveEntrance == entranceToSave &&
            saved.save.saveInfo.permanentSceneFlags[SCENE_00KEIKOKU].chest == 0x91,
            "Better Save records the clock, inventory, scene flags, and normal resume entrance");
    require(!gSaveContext.save.isOwlSave &&
            BetterSaveMenu_Update(&play) == MM_BETTER_SAVE_HANDLED &&
            play.pauseCtx.savePromptState == PAUSE_SAVEPROMPT_STATE_BETTER_SAVING && flashWrites == 1,
            "the second prompt waits until the save and first textbox finish");
    press = 0;
    play.sramCtx.status = 0;
    play.msgCtx.msgMode = MSGMODE_NONE;
    require(BetterSaveMenu_Update(&play) == MM_BETTER_SAVE_HANDLED &&
            prompt.ends_with(validEntrance ? '\xC3' : '\xC2'),
            "the destination prompt includes exactly the usable choices");
    press = BTN_A;
    play.msgCtx.choiceIndex = selected;
    require(BetterSaveMenu_Update(&play) == MM_BETTER_SAVE_CLOSE_PAUSE && play.transitionTrigger == TRANS_TRIGGER_OFF,
            "the destination waits for native pause cleanup before starting a fade");
    play.pauseCtx.state = PAUSE_STATE_OFF;
    BetterSaveMenu_OnUnpause(&play);
    if (selected == 0) {
        require(play.transitionTrigger == TRANS_TRIGGER_OFF && !BetterSaveMenu_ConsumeDirectTransition(&play),
                "Continue must leave the running scene alone");
        require(gHorseIsMounted, "continuing keeps Epona mounted");
    } else {
        const s32 expected = validEntrance && selected == 1 ? entrance : ENTRANCE(SOUTH_CLOCK_TOWN, 0);
        require(play.nextEntrance == expected && play.transitionTrigger == TRANS_TRIGGER_START &&
                play.transitionType == TRANS_TYPE_FADE_BLACK && gSaveContext.respawnFlag == 0 &&
                gSaveContext.nextCutsceneIndex == 0xFFEF && gSaveContext.gameMode == GAMEMODE_NORMAL &&
                gSaveContext.save.day == 3 && gSaveContext.save.time == 0xABCD &&
                gSaveContext.save.saveInfo.playerData.rupees == 84 &&
                gSaveContext.respawn[RESPAWN_MODE_RETURN].entrance == ENTRANCE(TERMINA_FIELD, 0),
                "return uses the resolved spawn or Clock Town and keeps the live cycle and grotto exit");
        require(BetterSaveMenu_ConsumeDirectTransition(&play) && !BetterSaveMenu_ConsumeDirectTransition(&play),
                "only the requested return bypasses entrance shuffle");
        const auto& parked = gSaveContext.save.saveInfo.horseData;
        require(!gHorseIsMounted && parked.sceneId == play.sceneId && parked.pos.y == 22 && parked.yaw == 42,
                "travel leaves Epona parked in the original scene");
    }
}
static void CheckResetSave(PlayState& play, int mode) {
    gComboGameMode=mode;
    gSaveContext={};
    gSaveContext.fileNum=1;
    gSaveContext.flashSaveAvailable=true;
    gSaveContext.gameMode=GAMEMODE_NORMAL;
    gSaveContext.save.day=3;
    gSaveContext.save.time=0xABCD;
    gSaveContext.eventInf[2]=0x55;
    gSaveContext.respawn[RESPAWN_MODE_TOP].pos.y=24;
    lastScene=entranceToSave=-1;
    disk=nlohmann::json::object();
    flashWrites=flushes=0;
    static u8 buffer[SAVE_BUFFER_SIZE]{};
    play.sramCtx.saveBuf=buffer;
    play.sceneId=SCENE_10YUKIYAMANOMURA;
    play.actorCtx.sceneFlags.chest=0x31;
    const auto mountainEntrance=ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 1);
    gSaveContext.save.entrance=mountainEntrance;

    // A player may turn the checkbox on after entering the scene.
    rememberLocation=false;
    autosaveEnabled=true;
    ObserveScene(play.sceneId);
    SavingEnhancements_AutosaveOnReset();
    rememberLocation=true;
    autosaveEnabled=false;
    SavingEnhancements_AutosaveOnReset();
    autosaveEnabled=true;
    canSave=false;
    SavingEnhancements_AutosaveOnReset();
    canSave=true;
    play.gameOverCtx.state=GAMEOVER_DEATH_START;
    SavingEnhancements_AutosaveOnReset();
    play.gameOverCtx.state=GAMEOVER_INACTIVE;
    play.transitionTrigger=TRANS_TRIGGER_START;
    SavingEnhancements_AutosaveOnReset();
    play.transitionTrigger=TRANS_TRIGGER_OFF;
    require(flashWrites==0 && flushes==0, "disabled or unsafe resets must leave the previous save alone");

    SavingEnhancements_AutosaveOnReset();
    require(flashWrites==(mode==COMBO_GAME_BOTH ? 1 : 2) && flushes==(mode==COMBO_GAME_BOTH ? 1 : 0),
            "reset must write the right slot and only keep a separate backup in Solo MM");
    require(!gSaveContext.save.isOwlSave && gSaveContext.save.shipSaveInfo.pauseSaveEntrance==-1,
            "writing the reset checkpoint must clean up live owl metadata");
    const auto snapshot=disk.at("owlSave").get<SaveContext>();
    require(snapshot.save.isOwlSave && snapshot.save.shipSaveInfo.pauseSaveEntrance==mountainEntrance &&
            snapshot.save.saveInfo.playerData.savedSceneId==play.sceneId && snapshot.save.day==3 &&
            snapshot.save.time==0xABCD && snapshot.eventInf[2]==0x55 &&
            snapshot.save.shipSaveInfo.respawn[RESPAWN_MODE_TOP].pos.y==24 &&
            snapshot.save.saveInfo.permanentSceneFlags[play.sceneId].chest==0x31,
            "reset must preserve Mountain Village's entrance, current cycle state, scene flags and respawns");

    const auto grotto=ENTRANCE(GROTTOS, 4);
    gSaveContext.save.entrance=grotto;
    ObserveScene(SCENE_KAKUSIANA);
    gSaveContext.save.entrance=mountainEntrance;
    ObserveScene(play.sceneId);
    SavingEnhancements_PersistSaveEntranceInfo();
    require(gSaveContext.save.shipSaveInfo.pauseSaveEntrance==grotto,
            "a positioned grotto exit must keep its previous native save entrance");
}
int main() {
    PlayState play{}; gPlayState=&play; gGameState=&play.state; play.state.running=true;
    for (bool remember : {false,true}) for (int slot=0;slot<3;++slot) {
        gSaveContext={}; disk=nlohmann::json{}; gComboOwlBlobSlot=-1;
        gSaveContext.fileNum=slot; gSaveContext.gameMode=GAMEMODE_NORMAL;
        gSaveContext.save.entrance=ENTRANCE(LOST_WOODS,0);
        gSaveContext.save.day=3; gSaveContext.save.time=0xABCD;
        gSaveContext.save.timeSpeedOffset=-2;
        gSaveContext.save.playerForm=PLAYER_FORM_GORON;
        gSaveContext.save.isFirstCycle=true; gSaveContext.save.hasTatl=true;
        gSaveContext.save.saveInfo.playerData.health=0x50;
        gSaveContext.save.saveInfo.playerData.rupees=147;
        gSaveContext.save.saveInfo.inventory.items[0]=ITEM_BOW;
        gSaveContext.save.saveInfo.weekEventReg[12]=0xA5;
        gSaveContext.eventInf[2]=0xA5;
        gSaveContext.jinxTimer=123;
        gSaveContext.respawn[RESPAWN_MODE_TOP].pos={11,22,33};
        gSaveContext.cycleSceneFlags[SCENE_MITURIN].chest=0xA501;
        gSaveContext.cycleSceneFlags[SCENE_MITURIN].switch0=0xDA12;
        play.sceneId=SCENE_LOST_WOODS;
        play.roomCtx.curRoom.num=0;
        play.actorCtx.sceneFlags.chest=0x1A;
        play.actorCtx.sceneFlags.switches[0]=0x5678;
        SavePortalSession(SaveManager_SaveCurrentForCombo);
        require(disk.contains("owlSave"),
                "portal departure must preserve a full current-session snapshot, even without an earlier owl save");
        require(disk.value("comboPortalSession",false), "the full session must identify its respawn-restoration policy");
        require(disk["owlSave"]["save"]["saveInfo"]["permanentSceneFlags"][SCENE_LOST_WOODS]["chest"]==0x1A,
                "portal snapshot must capture the current scene before teardown");
        const auto liveProgress=nlohmann::json(gSaveContext.save.saveInfo);
        auto snapshotProgress=disk["owlSave"]["save"]["saveInfo"];
        require(!gSaveContext.save.isOwlSave,
                "portal snapshot metadata must not turn the running game into an owl-save quit");
        gSaveContext={};
        require(SaveManager_LoadSaveFile(slot+1)==0 && gSaveContext.fileNum==slot && failures==0,
                "portal round trip must reload the same existing slot without initialization or repair");
        RestoreCycleFlags();
        if (remember) LoadRespawnData(slot);
        snapshotProgress["checksum"]=gSaveContext.save.saveInfo.checksum;
        require(nlohmann::json(gSaveContext.save.saveInfo)==snapshotProgress,
                "all native/randomizer/NEI progress must survive real JSON serialization and loading");
        require(gSaveContext.save.day==3 && gSaveContext.save.time==0xABCD &&
                gSaveContext.save.timeSpeedOffset==-2 && gSaveContext.save.playerForm==PLAYER_FORM_GORON &&
                gSaveContext.eventInf[2]==0xA5 && gSaveContext.jinxTimer==123 &&
                gSaveContext.respawn[RESPAWN_MODE_TOP].pos.y==22 &&
                gSaveContext.cycleSceneFlags[SCENE_MITURIN].switch0==0xDA12,
                "current cycle, form, temporary quest state, timers, scene switches and Farore point must survive");
    }
    require(writes==6,"each portal snapshot must persist once");
    gComboOwlBlobSlot=-1;
    SaveWithoutPortalSession(SaveManager_SaveCurrentForCombo);
    require(!disk.contains("owlSave") && !disk.contains("comboPortalSession"),
            "dropping a stale owl snapshot must also remove its portal restoration policy");
    disk["comboPortalSession"]=true;
    SaveWithoutPortalSession(SaveManager_SaveCurrentForCombo);
    require(!disk.contains("comboPortalSession"), "a bare save must not retain a stray portal marker");
    disk["owlSave"]=nlohmann::json::object(); disk["comboPortalSession"]=true;
    gComboOwlBlobSlot=(int)gSaveContext.fileNum+1;
    SaveWithoutPortalSession(SaveManager_SaveCurrentForCombo);
    require(!disk.contains("owlSave") && !disk.contains("comboPortalSession"),
            "dropping a malformed owl snapshot must also remove its portal restoration policy");
    const int completedWrites=writes;
    const auto completedDisk=disk;
    gGameState=nullptr;
    SavePortalSession(SaveManager_SaveCurrentForCombo);
    require(writes==completedWrites && disk==completedDisk,
            "a requested portal snapshot must refuse to save outside the active play state");
    gGameState=&play.state;
    gSaveContext.gameMode=GAMEMODE_TITLE_SCREEN;
    SavePortalSession(SaveManager_SaveCurrentForCombo);
    require(writes==completedWrites && disk==completedDisk,
            "a title-demo portal request must never persist its initialized state over progress");
    gSaveContext.gameMode=GAMEMODE_NORMAL;
    for (int invalidSlot : {-1,3,255}) {
        gSaveContext.fileNum=invalidSlot;
        SavePortalSession(SaveManager_SaveCurrentForCombo);
    }
    require(writes==completedWrites && disk==completedDisk, "invalid slots must never write a portal snapshot");
    CheckResetSave(play, COMBO_GAME_BOTH);
    CheckResetSave(play, COMBO_GAME_MM);
    std::puts("PASS MM portal and reset saves: slots, native checkpoints, complete progress and safe write guards");
    for (int choice : {-1, 0, 1, 2}) CheckBetterSave(play, choice, true);
    CheckBetterSave(play, 1, false);
    std::puts("PASS MM Better Save Menu: cancel, one owl save, wait, continue, scene return and Clock Town");
}
'''

with tempfile.TemporaryDirectory(prefix='portal-save-') as directory:
    cpp=Path(directory)/'test.cpp'; binary=Path(directory)/'test'; cpp.write_text(source)
    includes=['.','mm','mm/2s2h','mm/include','mm/include/PR','mm/src','mm/assets','libultraship/include','combo','combo/menu']
    command=[os.environ.get('CXX','c++'),'-std=c++20','-w','-DCOMBO_BUILD','-DF3DEX_GBI_2',
             '-DFMT_HEADER_ONLY','-ffunction-sections','-fdata-sections','-Wl,--gc-sections',
             *['-I'+str(ROOT/path) for path in includes]]
    if args.sanitize:
        command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie']
    # The real SaveContext JSON converters serialize and validate Solo MM
    # entrance layouts; link their production codec instead of replacing it.
    result=subprocess.run([*command,str(cpp),str(ROOT/'mm/2s2h/Rando/SoloEntrances.cpp'),
                           str(ROOT/'mm/2s2h/Enhancements/Saving/BetterSaveMenu.cpp'),
                           '-o',str(binary)],capture_output=True,text=True)
    if result.returncode: raise SystemExit(result.stderr[:6500])
    subprocess.run([str(binary)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
