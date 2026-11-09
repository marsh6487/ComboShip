#!/usr/bin/env python3
"""Real native map grants and fill guard: registers, save, time, exception/TLS isolation."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
parser = argparse.ArgumentParser()
parser.add_argument('--source-rev', help='native guard revision for a semantic negative control')
args = parser.parse_args()

def block(source, marker):
    start = source.index(marker)
    begin = source.index('{', start)
    end, depth = begin + 1, 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

guard = (subprocess.check_output(['git', 'show', args.source_rev + ':mm/2s2h/Rando/Logic/GlitchlessLogic.cpp'],
                               cwd=ROOT, text=True) if args.source_rev else
         (ROOT / 'mm/2s2h/Rando/Logic/GlitchlessLogic.cpp').read_text())
give = (ROOT / 'mm/2s2h/Rando/GiveItem.cpp').read_text()
inventory = (ROOT / 'mm/src/code/z_inventory.c').read_text()
program = r'''
#include <nlohmann/json.hpp>
#include "Rando/Logic/Logic.h"
#include "regs.h"
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <thread>
extern "C" { SaveContext gSaveContext{}; RegEditor* gRegEditor = nullptr; }
namespace Rando::Logic { uint64_t gCurrentRegionTime = 0; }
'''
program += re.search(r'^thread_local bool Rando::gItemGrantSimulation\s*=\s*false;', give, re.M)[0] + '\n'
program += 'namespace Rando::Logic {\n' + block(guard, 'struct FillSimulation {') + ';\n}\n'
program += block(inventory, 'u32 gBitFlags[') + ';\n'
program += block(inventory, 'static u16 sSceneIdsPerTingleMap[') + ';\n'
program += block(inventory, 'void Inventory_SetWorldMapCloudVisibility(') + '\n'
program += r'''
int main() {
    RegEditor registers;
    std::memset(&registers, 0x5A, sizeof(registers));
    gRegEditor = &registers;
    const RegEditor originalRegisters = registers;
    gSaveContext.save.saveInfo.playerData.health = 0x50;
    const SaveContext originalSave = gSaveContext;
    Rando::Logic::gCurrentRegionTime = 0x1234;
    for (int exceptional = 0; exceptional != 2; ++exceptional) {
        for (int map = 0; map != TINGLE_MAP_MAX; ++map) {
            try {
                Rando::Logic::FillSimulation simulation;
                assert(Rando::gItemGrantSimulation);
                bool otherThread = true;
                std::thread thread([&] { otherThread = Rando::gItemGrantSimulation; });
                thread.join();
                assert(!otherThread);
                Inventory_SetWorldMapCloudVisibility(map);
                assert(R_MINIMAP_DISABLED == 0); // Actual native map grant writes outside SaveContext.
                gSaveContext.save.saveInfo.playerData.health = 0x10;
                Rando::Logic::gCurrentRegionTime = 0xFFFF;
                if (exceptional) throw std::runtime_error("native guard exit");
            } catch (const std::runtime_error&) {}
            assert(gRegEditor == &registers);
            assert(std::memcmp(&registers, &originalRegisters, sizeof(registers)) == 0);
            assert(std::memcmp(&gSaveContext, &originalSave, sizeof(gSaveContext)) == 0);
            assert(Rando::Logic::gCurrentRegionTime == 0x1234);
            assert(!Rando::gItemGrantSimulation);
        }
    }
    {
        Rando::Logic::FillSimulation outer;
        { Rando::Logic::FillSimulation inner; assert(Rando::gItemGrantSimulation); }
        assert(Rando::gItemGrantSimulation); // Nested scope restores the already-active probe.
    }
    assert(!Rando::gItemGrantSimulation);
    gRegEditor = nullptr;
    { Rando::Logic::FillSimulation simulation; assert(Rando::gItemGrantSimulation); }
    assert(!gRegEditor && !Rando::gItemGrantSimulation);
}
'''
with tempfile.TemporaryDirectory(prefix='native-fill-guard-') as temporary:
    directory = Path(temporary)
    source = directory / 'guard.cpp'
    source.write_text(program)
    for combo in (False, True):
        binary = directory / ('combo' if combo else 'original')
        # Native MM target already allows C asset/constant initializer narrowing.
        flags = ['-std=c++20', '-Wno-narrowing', '-O1', '-g', '-pthread', '-DF3DEX_GBI_2', '-DGBI_FLOAT_MTX', '-DGBI_S32_VTX',
                 '-DCONTROLLERBUTTONS_T=uint32_t', '-DLOG_LEVEL_GAME_PRINTS=0', '-DFMT_HEADER_ONLY',
                 '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-no-pie']
        if combo:
            flags.append('-DCOMBO_BUILD')
        for relative in ('.', 'combo', 'mm', 'mm/2s2h', 'mm/include', 'mm/include/PR', 'mm/src',
                         'mm/assets', 'libultraship/include'):
            flags += ['-I' + str(ROOT / relative)]
        subprocess.run([os.environ.get('CXX', 'c++'), *flags, str(source), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
print('PASS actual native six map grants: full register/save/time restoration, exception exits, null and thread-local controls (both guards, ASan/UBSan)')
