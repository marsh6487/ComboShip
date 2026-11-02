#!/usr/bin/env python3
"""Verify Vanilla Mirror source selection against the real native-forest owner policy."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[2]


def function(path,signature):
    source=(ROOT/path).read_text();start=source.index(signature);brace=source.index('{',start);depth=1;end=brace+1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]


code=r'''
#include "combo/entrances/NativeForest.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
static bool assets=true, graph=true, vanilla=true, owned=false, shuffled=false, stumpAvailable=false;
static std::string missing;
static std::vector<std::string> lookedUp;
static uint8_t ResourceMgr_FileExists(const char* path) {
    lookedUp.emplace_back(path);
    return assets && missing!=path;
}
namespace NativeForest {
NativeForestDependencies::Graph LoadDependencies(const char* path) {
    assert(std::string(path)==DependencyPath(kCombinedScene));
    if(!graph) return {};
    return {{"custom/native/actual_owner"},{"stock/native/required"}};
}
}
struct { struct { struct { struct { int threeDayResetCount=1; } playerData; } saveInfo; } save; } gSaveContext;
static bool Combo_IsVanilla() { return vanilla; }
static bool MagicMirror_IsOwned() { return owned; }
namespace MirrorShuffle { static bool StumpAvailable() { return stumpAvailable; } }
'''
adapter='mm/2s2h/Enhancements/Gameplay/ComboVanillaRewards.cpp'
code+=function(adapter,'bool AuthoredMirrorSourceAvailable()')+'\n'
code+=function(adapter,'extern "C" int MM_VanillaMirrorUsesAuthoredSource(void) {')+'\n'
code+='namespace MirrorHost { static bool Shuffled() { return shuffled; }\n'
code+=function('mm/2s2h/Enhancements/Gameplay/MagicMirror.cpp','bool PickupAvailable()')+'\n}\n'
code+=r'''
int main() {
    assert(MM_VanillaMirrorUsesAuthoredSource());
    assert(MirrorHost::PickupAvailable());
    assert(std::find(lookedUp.begin(),lookedUp.end(),NativeForest::kCombinedScene)!=lookedUp.end());
    for(const auto& path:lookedUp) assert(path.find("Z2_00KEIKOKU")==std::string::npos);
    gSaveContext.save.saveInfo.playerData.threeDayResetCount=0;
    assert(!MirrorHost::PickupAvailable());
    gSaveContext.save.saveInfo.playerData.threeDayResetCount=1;
    for(const auto* path:{NativeForest::kCombinedScene,NativeForest::DependencyPath(NativeForest::kCombinedScene),
                         "custom/native/actual_owner","stock/native/required"}) {
        missing=path;
        assert(!MM_VanillaMirrorUsesAuthoredSource());
        assert(!MirrorHost::PickupAvailable());
    }
    missing.clear();graph=false;
    assert(!MM_VanillaMirrorUsesAuthoredSource());
    graph=true;owned=true;
    assert(!MirrorHost::PickupAvailable());
    owned=false;vanilla=false;assets=false;lookedUp.clear();
    assert(MirrorHost::PickupAvailable());
    shuffled=true;assert(!MirrorHost::PickupAvailable());
    stumpAvailable=true;assert(MirrorHost::PickupAvailable());
    assert(lookedUp.empty());
    std::cout<<"PASS MM authored/fallback Mirror owner, incomplete dependency rejection, first-cycle guard and seeded/solo source controls\n";
}
'''
code=code.replace('#include <cassert>','#include <algorithm>\n#include <cassert>')
with tempfile.TemporaryDirectory(prefix='vanilla-mm-mirror-owner-') as tmp:
    cpp=Path(tmp)/'test.cpp';cpp.write_text(code)
    flags=['-std=c++20','-Wall','-Wextra','-Werror','-I'+str(ROOT)]
    if '--sanitize' in sys.argv:
        flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie']
    binary=Path(tmp)/'test'
    subprocess.run([*shlex.split(os.environ.get('CXX','c++')),*flags,str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
