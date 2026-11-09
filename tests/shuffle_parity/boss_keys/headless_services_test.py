#!/usr/bin/env python3
"""Actual MM initializer, real service types, native register init and map grants."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
source = (ROOT / 'mm/2s2h/BenPort.cpp').read_text()
a = source.index('extern "C" COMBO_EXPORT void MM_InitRandoHeadless(void)')
b = source.index('{', a); end = b + 1; depth = 1
while depth:
    depth += (source[end] == '{') - (source[end] == '}'); end += 1
program = r'''
#include "Rando/Logic/Logic.h"
#include "Rando/Logic/HeadlessRegisters.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
extern "C" {
#include "regs.h"
#include "functions.h"
SaveContext gSaveContext;
void* SystemArena_Malloc(size_t size) { auto* p = std::malloc(size); std::memset(p, 0xa5, size); return p; }
}
#define COMBO_EXPORT
GameInteractor* GameInteractor::Instance = nullptr;
int graphInitializations = 0, nameInitializations = 0;
namespace ShipInit { void Init(const char*) { ++graphInitializations; } }
namespace Rando::StaticData { void PopulateCheckNames() { ++nameInitializations; } }
''' + source[a:end] + r'''
int main(int argc, char**) {
    Regs_Init(); // Complete native z_debug.c implementation, allocation boundary only.
    auto* reference = gRegEditor;
    if (argc > 1) reference->data[0] = 23;
    else gRegEditor = nullptr;
    MM_InitRandoHeadless();
    assert(GameInteractor::Instance);
    assert(GameInteractor::Instance->nextHookId == 1 && GameInteractor::Instance->events.empty());
    assert(gRegEditor); // Old headless init fails here; a native Tingle grant otherwise segfaults.
    auto* headless = gRegEditor;
    if (argc > 1) assert(headless == reference && headless->data[0] == 23);
    else {
        assert(headless->regPage == reference->regPage && headless->regGroup == reference->regGroup);
        assert(headless->regCur == reference->regCur && headless->dPadInputPrev == reference->dPadInputPrev);
        assert(headless->inputRepeatTimer == reference->inputRepeatTimer);
        assert(std::memcmp(headless->data, reference->data, sizeof(reference->data)) == 0);
    }
    auto* service = GameInteractor::Instance;
    headless->data[0] = 17;
    MM_InitRandoHeadless();
    assert(GameInteractor::Instance == service && gRegEditor == headless && headless->data[0] == 17);
    assert(graphInitializations == 1 && nameInitializations == 1);
    // Every real native Tingle map follows the same save/register path as full native Regs_Init.
    for (int map = 0; map < TINGLE_MAP_MAX; ++map) {
        gSaveContext = {}; gRegEditor = reference; R_MINIMAP_DISABLED = true;
        Inventory_SetWorldMapCloudVisibility(map);
        const auto expected = gSaveContext.save.saveInfo;
        assert(!R_MINIMAP_DISABLED);
        gSaveContext = {}; gRegEditor = headless; R_MINIMAP_DISABLED = true;
        Inventory_SetWorldMapCloudVisibility(map);
        assert(!R_MINIMAP_DISABLED);
        assert(gSaveContext.save.saveInfo.worldMapCloudVisibility == expected.worldMapCloudVisibility);
        assert(std::memcmp(gSaveContext.save.saveInfo.scenesVisible, expected.scenesVisible,
                           sizeof(expected.scenesVisible)) == 0);
    }
    delete service; GameInteractor::Instance = nullptr;
    if (headless != reference) std::free(reference); else std::free(headless);
    gRegEditor = nullptr;
}
'''
with tempfile.TemporaryDirectory(prefix='headless-service-') as directory:
    work = Path(directory); cpp = work / 'test.cpp'; cpp.write_text(program)
    flags = ['-DF3DEX_GBI_2', '-DGBI_FLOAT_MTX', '-DGBI_S32_VTX', '-DCONTROLLERBUTTONS_T=uint32_t',
             '-ffunction-sections', '-fdata-sections']
    for name in ('.', 'combo', 'mm/include', 'mm/include/PR', 'mm', 'mm/2s2h', 'mm/src', 'mm/assets', 'libultraship/include'):
        flags += ['-I' + str(ROOT / name)]
    for combined in (False, True):
        mode = ['-DCOMBO_BUILD'] if combined else []
        objects = []
        for native in ('z_debug.c', 'z_inventory.c'):
            obj = work / (native + str(combined) + '.o'); objects.append(str(obj))
            subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', *flags, *mode,
                            '-c', str(ROOT / 'mm/src/code' / native), '-o', str(obj)], check=True)
        binary = work / ('combo' if combined else 'original')
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', *flags, *mode,
                        '-include', str(ROOT / 'tests/dungeon_rewards/headless.h'), str(cpp), *objects,
                        '-Wl,--gc-sections', '-o', str(binary)], check=True)
        for args in ([], ['preserve']): subprocess.run([str(binary), *args], check=True)
print('PASS real MM headless services: native Regs_Init equivalence, six complete native Tingle-map grants, existing pointer/state preservation; graph execution not claimed')
