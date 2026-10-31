#!/usr/bin/env python3
"""Execute OoT's full status updater for equipment aliases and native controls."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/oot_equipment_pause'))
from run_access_tests import function

PREFIX = r'''
#include "global.h"
#include "mods/extended_equipment.h"
#include "mods/ext_buttons/ext_buttons.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
SaveContext gSaveContext;
u16 gEquipMasks[] = { 0x000F, 0x00F0, 0x0F00, 0xF000 };
u8 gEquipShifts[] = { 0, 4, 8, 12 };
static s32 hazard;
#define PLAYER_ON_REAL_HORSE(player) ((player)->stateFlags1 & PLAYER_STATE1_ON_HORSE)
#define BUTTON_STATUS_INDEX(button) ((button) >= 4 ? (button) + 1 : (button))
bool GameInteractor_Should(GIVanillaBehavior behavior, uint32_t result, ...) { return result; }
u8 GameInteractor_PacifistModeActive(void) { return 0; }
s32 CVarGetInteger(const char* key, s32 fallback) { return fallback; }
s32 Combo_IsCombinedGame(void) { return 0; }
s32 ComboFw_HasPoint(s32 nativePoint) { return 0; }
s32 Flags_GetSwitch(PlayState* play, s32 flag) { return 0; }
s32 Player_GetEnvironmentalHazard(PlayState* play) { return hazard; }
void Interface_ChangeHudVisibilityMode(u16 mode) {}
void Interface_LoadItemIcon1(PlayState* play, u16 button) {}
#undef osSyncPrintf
void osSyncPrintf(const char* fmt, ...) {}
'''

CHECKS = r'''
int main(void) {
    static PlayState play;
    static Player player;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    for (int environment = 0; environment < 4; ++environment) {
        for (int button = 1; button < 8; ++button) {
            for (int offset = 0; offset < 12; ++offset) {
                for (int legacy = 0; legacy < 2; ++legacy) {
                    memset(&gSaveContext, 0, sizeof(gSaveContext));
                    memset(&play.interfaceCtx.restrictions, 0, sizeof(play.interfaceCtx.restrictions));
                    memset(gSaveContext.equips.buttonItems, ITEM_NONE, sizeof(gSaveContext.equips.buttonItems));
                    hazard = environment == 3 ? 0 : environment + 2;
                    play.interfaceCtx.restrictions.all = environment == 3;
                    u16 item = legacy ? ITEM_EXT_SWORD_1 + offset : EXT_EQUIP_BUTTON_ITEM_BASE + offset;
                    if (legacy) gSaveContext.equips.buttonItems[button] = item;
                    else ExtButton_SetItem(button, item);
                    gSaveContext.buttonStatus[BUTTON_STATUS_INDEX(button)] = BTN_DISABLED;
                    func_80083108(&play);
                    assert(gSaveContext.buttonStatus[BUTTON_STATUS_INDEX(button)] == BTN_ENABLED &&
                           "full-ID equipment lost legacy underwater/scene status exemption");
                }
            }
            // The marker alone must not exempt an ordinary full-ID tool.
            ExtButton_SetItem(button, 0x0223);
            gSaveContext.buttonStatus[BUTTON_STATUS_INDEX(button)] = BTN_ENABLED;
            func_80083108(&play);
            assert(gSaveContext.buttonStatus[BUTTON_STATUS_INDEX(button)] == BTN_DISABLED);
            // Native equipment retains its exemption, native bombs remain restricted.
            gSaveContext.equips.buttonItems[button] = ITEM_BOOTS_IRON;
            func_80083108(&play);
            assert(gSaveContext.buttonStatus[BUTTON_STATUS_INDEX(button)] == BTN_ENABLED);
            gSaveContext.equips.buttonItems[button] = ITEM_BOMB;
            func_80083108(&play);
            assert(gSaveContext.buttonStatus[BUTTON_STATUS_INDEX(button)] == BTN_DISABLED);
        }
    }
    puts("PASS OoT status updater: all 12 aliases/legacy equipment x seven buttons, underwater hazards and scene restriction; ordinary full-ID/native item controls");
}
'''

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source-ref')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    def read(path):
        return subprocess.check_output(['git', 'show', args.source_ref + ':' + path], cwd=ROOT, text=True) if args.source_ref else (ROOT / path).read_text()
    status = read('soh/src/code/z_parameter.c')
    buttons = read('soh/mods/ext_buttons/ext_buttons.cpp')
    equip = read('soh/mods/extended_equipment.c')
    code = PREFIX
    for name in ('ExtButton_GetItem', 'ExtButton_SetItem'):
        code += '\n' + function(buttons, name)
    code += '\n' + function(equip, 'ExtEquip_IsButtonItem')
    if 'static u8 Interface_IsExtEquipmentButton(' in status:
        code += '\n' + function(status, 'Interface_IsExtEquipmentButton')
    code += '\n' + function(status, 'func_80083108') + CHECKS
    includes = ['-I' + str(ROOT / path) for path in ('soh', 'soh/include', 'soh/include/PR', 'soh/src', 'soh/assets', 'soh/soh', 'libultraship/include', 'combo', 'combo/menu')]
    flags = ['-std=gnu11', '-w', '-Werror=implicit-function-declaration', '-DF3DEX_GBI_2', '-DCONTROLLERBUTTONS_T=uint32_t', '-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"', '-DCVAR_PREFIX_SETTING="gSettings"']
    if args.sanitize:
        flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
    with tempfile.TemporaryDirectory(prefix='nei-status-oot-') as td:
        source = Path(td) / 'test.c'
        source.write_text(code)
        for combo in (False, True):
            binary = Path(td) / ('combo' if combo else 'solo')
            subprocess.run(['cc', *flags, *(['-DCOMBO_BUILD'] if combo else []), *includes, str(source), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True, env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0', UBSAN_OPTIONS='halt_on_error=1'))

if __name__ == '__main__':
    main()
