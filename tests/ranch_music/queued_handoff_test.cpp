// Engine output and editor lookup are boundaries. The queue, native Malon
// routing and scene-night adapter below are all compiled from production.
#include "audio/NightMusic.h"
#include "audio_sequence_stubs/global.h"
#include "night_bgm_bridge.h"
#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <vector>
using s16 = int16_t;
static std::vector<u16> played, masks;
extern "C" {
AudioContext gAudioContext = {};
u8 gSfxChannelLayout = 0;
s8 D_80133390[] = "SEQ H", D_80133398[] = "    L";
void Audio_QueueCmdS32(u32 command, s32) {
  if ((command >> 24) == 0x82 || (command >> 24) == 0x85) {
    played.push_back(command & 0xffff);
    gAudioContext.seqPlayers[(command >> 16) & 255].enabled = 1;
  } else if ((command >> 24) == 0x83)
    gAudioContext.seqPlayers[(command >> 16) & 255].enabled = 0;
}
void Audio_QueueCmdF32(u32, f32) {}
void Audio_QueueCmdS8(u32, s8) {}
void Audio_QueueCmdU16(u32, u16 mask) { masks.push_back(mask); }
void AudioDebug_ScrPrt(const s8 *, u16) {}
void func_800E5F88(u8) {}
void func_800F71BC(u8) {}
s32 func_800E5E20(u32 *) { return 0; }
s32 func_800E5EDC() { return 1; }
void func_800F7170() {}
u16 AudioEditor_GetReplacementSeq(u16 id) {
  return id == NA_BGM_LONLON ? 0x15d : id;
}
u16 AudioEditor_ResolveSceneSequence(u16, u16 resolved) { return resolved; }
u16 func_800FA0B4(u8);
void Audio_SetVolScale(u8, u8, u8, u8);
}
constexpr int SCENE_LON_LON_RANCH = 99, SCENE_HYRULE_FIELD = 81,
              CS_STATE_IDLE = 0, GAMEMODE_NORMAL = 0;
struct PlayState {
  int sceneNum = 99;
  struct {
    u16 seqId = NA_BGM_LONLON;
    u8 natureAmbienceId = 2;
  } sequenceCtx;
  struct {
    int state = 0;
  } csCtx;
};
static PlayState play;
static PlayState *gPlayState = &play;
static struct {
  int gameMode = 0, fileNum = 0, forcedSeqId = 0;
  u16 dayTime = 0xd000;
} gSaveContext;
static std::map<std::string, int> cvars;
static int CVarGetInteger(const char *key, int fallback) {
  auto i = cvars.find(key);
  return i == cvars.end() ? fallback : i->second;
}
static bool Player_InCsMode(PlayState *) { return false; }
extern "C" uint8_t Audio_IsSceneNightMusicInCombat() { return false; }
static u16 AudioEditor_GetSceneMusicRow(u16) { return 0; }
struct AudioCollection {
  static AudioCollection *Instance;
  bool HasSequenceNum(u16) { return true; }
  u16 GetReplacementSequence(u16 id) {
    return AudioEditor_GetReplacementSeq(id);
  }
};
static AudioCollection collection;
AudioCollection *AudioCollection::Instance = &collection;
static u8 sMalonSingingDisabled = 0, sMalonSingingTimer = 0;
static bool sIsMalonSinging;
static float sMalonSingingDist;
/* MACROS */
/* NATIVE */
/* NIGHT */
int main() {
  for (int globalNight : {0, 1})
    for (bool updaterFirst : {false, true}) {
      ResetSceneNightMusic();
      Audio_ProcessSeqCmds();
      sMalonSingingDisabled = sMalonSingingTimer = 0;
      cvars["gAudioEditor.EnableNightMusic"] = globalNight;
      gAudioContext.audioBufferParameters.updatesPerFrame = 4;
      for (auto &seq : gActiveSeqs)
        seq.seqId = NA_BGM_DISABLED;
      Audio_QueueResolvedSeqCmd(SEQ_PLAYER_BGM_MAIN, NA_BGM_NATURE_AMBIENCE, 0);
      Audio_ProcessSeqCmds();
      cvars["gAudioEditor.DisableRanchProximityMusic"] = 1;
      Audio_UpdateMalonSinging(100, NA_BGM_LONLON);
      UpdateSceneNightMusic();
      Audio_ProcessSeqCmds();

      assert(Audio_IsSceneNightBgmActive() &&
             (func_800FA0B4(SEQ_PLAYER_BGM_SUB) & 255) == 0x5d);
      // The option changes before actor draw, then the end-of-frame updater
      // runs, then the real native queue resolves the same custom bank for a
      // new owner.
      cvars["gAudioEditor.DisableRanchProximityMusic"] = 0;
      if (updaterFirst)
        UpdateSceneNightMusic();
      Audio_UpdateMalonSinging(100, NA_BGM_LONLON);
      UpdateSceneNightMusic();
      Audio_ProcessSeqCmds();
      UpdateSceneNightMusic();
      Audio_ProcessSeqCmds();
      assert(func_800FA0B4(SEQ_PLAYER_BGM_SUB) == NA_BGM_LONLON &&
             "live OFF must retain native Malon after same-bank ownership "
             "handoff");
      assert(!Audio_IsSceneNightBgmActive() && played.back() == 0x15d);
      assert(masks.size() >= 2 && masks[masks.size()-2] == 0xfffc && masks.back() == 3);
      for (int frame = 0; frame < 12; ++frame) {
        Audio_UpdateMalonSinging(100, NA_BGM_LONLON);
        UpdateSceneNightMusic();
        Audio_ProcessSeqCmds();
      }
      assert(func_800FA0B4(SEQ_PLAYER_BGM_SUB) == NA_BGM_LONLON);
    }
  std::cout << "PASS actual native queue + Malon + scene-night: custom-bank "
               "live OFF handoff remains native\n";
}
