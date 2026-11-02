#!/usr/bin/env python3
"""Compile real OoT/MM meter branches and bounded OoT magic-load diagnostics.

This probe supplies only option/audio/hook service boundaries. It does not run
the complete native save loader, shared-state transport or game update loop.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile


def braced(source, start):
    begin = source.index('{', start)
    end, depth = begin + 1, 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[begin:end]


def function(source, name):
    match = re.search(r'^.*\b' + name + r'\([^;{}]*\)\s*\{', source, re.M)
    if not match:
        raise ValueError(name)
    return source[match.start():source.index('{', match.start())] + braced(source, match.start())


def hook(source, name):
    start = source.index('COND_VB_SHOULD(' + name + ',')
    return braced(source, start)


FIXTURE = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <map>
#include <string>
#include <vector>
using s8 = int8_t; using u8 = uint8_t; using s16 = int16_t;
using s32 = int32_t;
constexpr int MAGIC_NORMAL_METER = 48;
constexpr int GAMEMODE_NORMAL = 0, NA_SE_SY_GAUGE_UP = 1, SFX_FLAG = 0;
/* ENUM */
struct PlayState {};
struct Save {
    s8 magic = 0, magicLevel = 0;
    u8 isMagicAcquired = 0, isDoubleMagicAcquired = 0;
    s16 magicState = 0, prevMagicState = 0, magicCapacity = 0;
    s16 magicFillTarget = 0, magicTarget = 0;
    int gameMode = GAMEMODE_NORMAL, sceneLayer = 0;
    struct { struct { struct { struct {
        u8 magicStatUpgrades = 0, comboNativeMagicLevel = 0;
    } randomizer; } data; } quest; } ship;
} gSaveContext;
static int sounds = 0, gSfxDefaultPos, gSfxDefaultFreqAndVolScale, gSfxDefaultReverb;
void Audio_PlaySfxGeneral(int, void*, int, void*, void*, void*) { ++sounds; }
#define osSyncPrintf(...) ((void)0)
enum { VB_MAGIC_FILL_TARGET, VB_MAGIC_STEP_CAPACITY_TARGET };
enum RandomizerSettingKey { RSK_MAGIC_STAT_UPGRADE_ADJUSTABLE, RSK_MAGIC_STAT_UPGRADE_TOTAL,
                            RSK_MAGIC_STAT_UPGRADE_REQUIRED, RSK_MAGIC_STAT_UPGRADE };
struct Option { int value; int Get() const { return value; } operator bool() const { return value; } };
namespace Rando { struct Context {
    bool adjustable = false;
    int required = 14;
    bool enabled = true;
    static Context* GetInstance() { static Context ctx; return &ctx; }
    Option GetOption(RandomizerSettingKey key) {
        return { key == RSK_MAGIC_STAT_UPGRADE ? enabled :
                 key == RSK_MAGIC_STAT_UPGRADE_ADJUSTABLE ? adjustable :
                 key == RSK_MAGIC_STAT_UPGRADE_TOTAL ? required - 1 : 99 };
    }
}; }
#define RAND_GET_OPTION(key) (Rando::Context::GetInstance()->GetOption(key))
/* CAPACITY */
/* REQUIRED */
static bool isRando = true;
#define IS_RANDO isRando
static std::vector<std::string> phases;
template <typename... T> void CaptureLog(const char*, const char* phase, unsigned, T...) {
    phases.emplace_back(phase);
}
#define SPDLOG_INFO(...) CaptureLog(__VA_ARGS__)
/* TRACE */
struct GameInteractor {
    struct OnLoadGame {};
    static GameInteractor* Instance;
    template <typename T> void ExecuteHooks(int32_t) {}
};
static GameInteractor gi;
GameInteractor* GameInteractor::Instance = &gi;
void Combo_SOH_BeforeNativeLoadGame() {}
void Combo_SOH_AfterNativeLoadGame() {}
#define COMBO_BUILD
/* LOAD_DISPATCH */
static bool hooksEnabled = true;
bool GameInteractor_Should(int id, bool result, ...) {
    if (!hooksEnabled) return result;
    va_list args; va_start(args, result);
    bool* should = &result;
    if (id == VB_MAGIC_FILL_TARGET) {
        [&]() /* FILL_HOOK */ ();
    } else if (id == VB_MAGIC_STEP_CAPACITY_TARGET) {
        [&]() /* CAPACITY_HOOK */ ();
    }
    va_end(args);
    return result;
}
/* MAGIC_FILL */
void UpdateMagicBar(PlayState* play) {
    s16 temp;
    switch (gSaveContext.magicState) {
        /* UPDATE */
        default: break;
    }
}
void Tick(PlayState* play) {
    /* INIT */
    UpdateMagicBar(play);
    /* TRACE_UPDATE */
}
int main() {
    PlayState play;
    int cases = 0;
    for (bool adjustable : { false, true }) {
      for (int required : { 8, 14, 15, 100 }) {
        if (!adjustable && required != 8) continue;
        auto* ctx = Rando::Context::GetInstance();
        ctx->adjustable = adjustable; ctx->required = required;
        for (int level = 1; level <= required; ++level) {
          for (int nativeTier : { 0, 1, 2 }) {
            const int capacity = MagicStatCapacity(level, required, adjustable, nativeTier);
            for (int saved : { 0, capacity / 2, capacity }) {
              gSaveContext = {};
              auto& r = gSaveContext.ship.quest.data.randomizer;
              r.magicStatUpgrades = level; r.comboNativeMagicLevel = nativeTier;
              gSaveContext.isMagicAcquired = 1;
              gSaveContext.isDoubleMagicAcquired = capacity > 48;
              gSaveContext.magic = saved;
              /* LOAD */
              phases.clear();
              GameInteractor_ExecuteOnLoadGame(0);
              sounds = 0;
              for (int frame = 0; frame < 128; ++frame) Tick(&play);
              if (gSaveContext.magicState != MAGIC_STATE_IDLE ||
                  gSaveContext.magicCapacity != capacity || gSaveContext.magic != saved) {
                  std::fprintf(stderr, "FAIL adj=%d req=%d level=%d native=%d saved=%d cap=%d state=%d magic=%d target=%d\n",
                               adjustable, required, level, nativeTier, saved, capacity,
                               gSaveContext.magicState, gSaveContext.magic, gSaveContext.magicFillTarget);
                  return 1;
              }
              int settledSounds = sounds;
              for (int frame = 0; frame < 256; ++frame) Tick(&play);
              assert(sounds == settledSounds);
              assert(gSaveContext.magicLevel != 0);
              assert((phases == std::vector<std::string>{ "begin", "settled" }));
              Magic_Fill(&play);
              for (int frame = 0; frame < 128; ++frame) Tick(&play);
              assert(gSaveContext.magic == capacity && gSaveContext.magicState == MAGIC_STATE_IDLE);
              ++cases;
            }
          }
        }
      }
    }
    // A deliberately reset amount models the unknown external writer, not a
    // reproduction of the user's trigger. The trace must report it once and
    // leave the native behavior untouched, so it can identify this failure.
    auto* ctx = Rando::Context::GetInstance();
    ctx->adjustable = true; ctx->required = 15;
    gSaveContext = {};
    gSaveContext.ship.quest.data.randomizer.magicStatUpgrades = 1;
    gSaveContext.isMagicAcquired = 1;
    gSaveContext.magicFillTarget = 7;
    phases.clear();
    GameInteractor_ExecuteOnLoadGame(0);
    sounds = 0;
    for (int frame = 0; frame < 1000; ++frame) {
        gSaveContext.magic = 0;
        Tick(&play);
    }
    assert((phases == std::vector<std::string>{ "begin", "still-active" }));
    assert(gSaveContext.magicState == MAGIC_STATE_FILL && sounds > 900);
    ctx->enabled = false;
    phases.clear();
    GameInteractor_ExecuteOnLoadGame(0);
    for (int frame = 0; frame < 1000; ++frame) Tick(&play);
    assert(phases.empty());
    std::printf("PASS: %d OoT fractional/native-floor load cases; saved magic and sound requests settle\n", cases);
    std::puts("PASS: OoT begin/settled/still-active traces are bounded; injected stall is observable and unchanged");
}
'''


MM_FIXTURE = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "combo/rando/RpgStats.h"
using s8 = int8_t; using u8 = uint8_t; using s16 = int16_t;
constexpr int MAGIC_NORMAL_METER = 48, GAMEMODE_NORMAL = 0;
constexpr int NA_SE_SY_GAUGE_UP = 1, SFX_FLAG = 0;
/* ENUM */
struct PlayState {};
struct PlayerData { s8 magic = 0, magicLevel = 0; u8 isMagicAcquired = 0, isDoubleMagicAcquired = 0; };
struct Save {
    struct { struct { PlayerData playerData; } saveInfo; } save;
    s16 magicState = 0, magicCapacity = 0, magicFillTarget = 0;
    int gameMode = GAMEMODE_NORMAL, sceneLayer = 0;
} gSaveContext;
struct Nei { ComboRpgState comboRpg{}; } nei;
Nei* Nei_Save() { return &nei; }
/* CAPACITY */
static int sounds = 0, buttonItem;
void Audio_PlaySfx(int) { ++sounds; }
#define MIN(a, b) std::min<int>(a, b)
#define BUTTON_ITEM_EQUIP(form, slot) buttonItem
#define ITEM_DEKU_NUT 1
void UpdateMagicBar(PlayState*) {
    s16 magicCapacityTarget;
    switch (gSaveContext.magicState) {
        /* UPDATE */
        default: break;
    }
}
void Tick(PlayState* play) {
    /* INIT */
    UpdateMagicBar(play);
}
int main() {
    PlayState play;
    int cases = 0;
    for (int total : { 96, 100 }) {
      for (int required : { 8, 14, 15, 100 }) {
        if (total == 96 && required != 8) continue;
        for (int level = 1; level <= required; ++level) {
          for (int tier : { 0, 1, 2 }) {
            auto& r = nei.comboRpg;
            r = {}; r.knownMask = r.enabledMask = 1 << COMBO_RPG_MAGIC;
            r.magicTotal = total; r.required[COMBO_RPG_MAGIC] = required;
            r.level[COMBO_RPG_MAGIC] = level; r.nativeMagicLevel = tier;
            const int cap = ComboRpgState_MagicCapacity(&r, 0);
            for (int saved : { 0, cap / 2, cap }) {
              gSaveContext = {};
              auto& pd = gSaveContext.save.saveInfo.playerData;
              pd.isMagicAcquired = 1; pd.isDoubleMagicAcquired = cap > 48; pd.magic = saved;
              sounds = 0;
              for (int frame = 0; frame < 128; ++frame) Tick(&play);
              assert(pd.magic == saved && pd.magicLevel != 0);
              assert(gSaveContext.magicCapacity == cap && gSaveContext.magicState == MAGIC_STATE_IDLE);
              const int settled = sounds;
              for (int frame = 0; frame < 256; ++frame) Tick(&play);
              assert(sounds == settled);
              ++cases;
            }
          }
        }
      }
    }
    std::printf("PASS: %d MM fractional/native-floor load cases; saved magic and sound requests settle\n", cases);
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    root = args.root
    native = (root / 'soh/src/code/z_parameter.c').read_text()
    save = (root / 'soh/include/z64save.h').read_text()
    stats = (root / 'soh/soh/Enhancements/randomizer/randostatupgrade.h').read_text()
    hooks = (root / 'soh/soh/Enhancements/randomizer/randostatupgrade.cpp').read_text()
    dispatch = (root / 'soh/soh/Enhancements/game-interactor/GameInteractor_Hooks.cpp').read_text()
    load = (root / 'soh/src/overlays/gamestates/ovl_file_choose/z_file_choose.c').read_text()
    enum = re.search(r'typedef enum\s*\{[^{}]*MAGIC_STATE_IDLE[^{}]*\}\s*MagicState;', save, re.S).group()
    update = function(native, 'Interface_UpdateMagicBar')
    step = update[update.index('        case MAGIC_STATE_STEP_CAPACITY:'):update.index('        case MAGIC_STATE_CONSUME_SETUP:')]
    init_start = native.index('if ((gSaveContext.isMagicAcquired != 0) && (gSaveContext.magicLevel == 0))')
    init = native[init_start:native.index('{', init_start)] + braced(native, init_start)
    # Compile the actual save-to-HUD transfer; runtime-only fields are cleared in the fixture Save initializer.
    load_start = load.index('    gSaveContext.magicFillTarget = gSaveContext.magic;')
    load_end = load.index('\n', load.index('    gSaveContext.magicLevel = gSaveContext.magic;', load_start))
    replacements = {
        'ENUM': enum,
        'CAPACITY': function(stats, 'MagicStatCapacity'),
        'REQUIRED': function(stats, 'StatUpgradeRequired'),
        'FILL_HOOK': hook(hooks, 'VB_MAGIC_FILL_TARGET'),
        'CAPACITY_HOOK': hook(hooks, 'VB_MAGIC_STEP_CAPACITY_TARGET'),
        'MAGIC_FILL': function(native, 'Magic_Fill'),
        'UPDATE': step,
        'INIT': init,
        'LOAD': load[load_start:load_end],
        'TRACE': hooks[hooks.index('static bool sMagicLoadTraceActive'):hooks.index('static void RegisterRandoStatUpgradeHooks')],
        'TRACE_UPDATE': 'RandoStat_TraceMagicLoad();' if 'RandoStat_TraceMagicLoad();' in update else '',
        'LOAD_DISPATCH': function(dispatch, 'GameInteractor_ExecuteOnLoadGame'),
    }
    fixture = FIXTURE
    for key, value in replacements.items():
        fixture = fixture.replace('/* ' + key + ' */', value)
    mm_native = (root / 'mm/src/code/z_parameter.c').read_text()
    mm_save = (root / 'mm/include/z64save.h').read_text()
    mm_rpg = (root / 'mm/mods/combo_rpg.cpp').read_text()
    mm_enum = re.search(r'typedef enum\s*\{[^{}]*MAGIC_STATE_IDLE[^{}]*\}\s*MagicState;', mm_save, re.S).group()
    mm_update = function(mm_native, 'Magic_Update')
    mm_step = mm_update[mm_update.index('        case MAGIC_STATE_STEP_CAPACITY:'):mm_update.index('        case MAGIC_STATE_CONSUME_SETUP:')]
    mm_init_start = mm_native.index('if ((gSaveContext.save.saveInfo.playerData.isMagicAcquired) &&')
    mm_init = mm_native[mm_init_start:mm_native.index('{', mm_init_start)] + braced(mm_native, mm_init_start)
    mm_fixture = MM_FIXTURE
    for key, value in {'ENUM': mm_enum, 'CAPACITY': function(mm_rpg, 'ComboRpg_MagicCapacity'),
                       'UPDATE': mm_step, 'INIT': mm_init}.items():
        mm_fixture = mm_fixture.replace('/* ' + key + ' */', value)
    with tempfile.TemporaryDirectory(prefix='magic-load-') as temp:
        path = Path(temp)
        flags = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if args.sanitize else []
        for engine, code in [('oot', fixture), ('mm', mm_fixture)]:
            source = path / (engine + '.cpp')
            binary = path / engine
            source.write_text(code)
            subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Wno-unused-parameter', *flags,
                            '-I', str(root), str(source), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    main()
