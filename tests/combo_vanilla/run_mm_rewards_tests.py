#!/usr/bin/env python3
"""Exercise MM's actual earned-item availability boundaries in both file modes."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(path, signature):
    source = (ROOT / path).read_text(encoding='utf-8-sig')
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


code = r'''
#include <cassert>
#include <cstdint>
#include <iostream>
#include "mm/mods/nei_save.h"
using u8 = uint8_t;
static bool vanilla;
static int option;
static int reads;
static bool Combo_IsVanilla() { return vanilla; }
static bool MagicMirror_IsOwned() { return false; }
static int CVarGetInteger(const char*, int) { ++reads; return option; }
#define CVAR_EXT_EQUIP_ENABLED "gCheats.ExtEquip.Enabled"
'''
code += function('mm/mods/extended_inventory.c', 'uint8_t Wand_RandoMode(void)')
code += '\n' + function('mm/mods/extended_inventory.c', 'uint8_t BombArrows_RandoMode(void)')
code += '\n' + function('mm/mods/extended_equipment.c', 'u8 ExtEquip_IsEnabled(void)')
for signature in ('bool ExtInv_IsCustomItemsEnabled(void)', 'bool ExtInv_IsMmMasksEnabled(void)',
                  'bool ExtInv_IsOnlyTransformation(void)'):
    code += '\n' + function('mm/mods/extended_inventory.c', signature)
code += r'''
int main() {
    for (option = 0; option < 3; ++option) {
        vanilla = false;
        assert(Wand_RandoMode() == option);
        assert(BombArrows_RandoMode() == option);
        assert(ExtEquip_IsEnabled() == (option != 0));
        assert(ExtInv_IsCustomItemsEnabled() == (option != 0));
        assert(ExtInv_IsMmMasksEnabled() == (option != 0));
        assert(ExtInv_IsOnlyTransformation() == (option != 0));
        vanilla = true;
        const int before = reads;
        assert(Wand_RandoMode() == WAND_RANDO_ELEMENTAL);
        assert(BombArrows_RandoMode() == BOMB_ARROWS_RANDO_OFF);
        assert(ExtEquip_IsEnabled());
        assert(ExtInv_IsCustomItemsEnabled());
        assert(ExtInv_IsMmMasksEnabled());
        assert(!ExtInv_IsOnlyTransformation());
        assert(reads == before); // A loaded vanilla file ignores stale global settings.
    }
    std::cout << "PASS MM exact wand modes and earned equipment, seeded/solo controls\n";
}
'''
with tempfile.TemporaryDirectory(prefix='vanilla-mm-rewards-') as tmp:
    source = Path(tmp) / 'test.cpp'
    source.write_text(code)
    flags = ['-std=c++20', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
             '-I'+str(ROOT), '-I'+str(ROOT/'mm'), '-I'+str(ROOT/'mm/2s2h')]
    if '--sanitize' in sys.argv:
        flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                  '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
    binary = Path(tmp) / 'test'
    subprocess.run([*shlex.split(os.environ.get('CXX', 'c++')), *flags,
                    str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
