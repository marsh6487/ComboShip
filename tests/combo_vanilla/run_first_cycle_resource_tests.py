#!/usr/bin/env python3
"""Run native first-cycle resource, spawn and return policies from real headers.

Archive availability is a service boundary. Native MM types, the exploration
predicate, forest selection and every tested spawn/return helper are production.
"""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

SOURCE = r'''
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "global.h"
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "combo/entrances/NativeForest.h"
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
int gComboVanillaMode = 1;
}
SaveContext gSaveContext;
static const char* active;
static void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL %s: %s\n", active, message);
        std::exit(1);
    }
}
int main() {
    struct Case {
        const char* name;
        int mode, vanilla, resets, firstCycle, spawn, layer, cutscene;
        bool exploration;
    };
    const Case cases[] = {
        {"fresh Vanilla intro", COMBO_GAME_BOTH, 1, 0, 0, 0, 0, 0, false},
        {"Vanilla first-cycle dawn", COMBO_GAME_BOTH, 1, 0, 1, 0, 0, 0, false},
        {"Vanilla after first reset", COMBO_GAME_BOTH, 1, 1, 0, 0, 0, 0, true},
        {"Vanilla later cycle", COMBO_GAME_BOTH, 1, 2, 0, 0, 0, 0, true},
        {"seeded combined before reset", COMBO_GAME_BOTH, 0, 0, 0, 0, 0, 0, true},
        {"seeded combined first-cycle flag", COMBO_GAME_BOTH, 0, 0, 1, 0, 0, 0, true},
        {"solo MM before reset", COMBO_GAME_MM, 0, 0, 0, 0, 0, 0, false},
        {"solo MM stale Vanilla identity", COMBO_GAME_MM, 1, 0, 0, 0, 0, 0, false},
        {"solo MM after reset", COMBO_GAME_MM, 0, 1, 0, 0, 0, 0, false},
        {"solo OoT", COMBO_GAME_OOT, 0, 1, 0, 0, 0, 0, false},
        {"Vanilla flashback layer", COMBO_GAME_BOTH, 1, 1, 0, 0, 1, 0, false},
        {"seeded flashback layer", COMBO_GAME_BOTH, 0, 0, 0, 0, 1, 0, false},
        {"Vanilla prologue spawn", COMBO_GAME_BOTH, 1, 1, 0, 1, 0, 0, false},
        {"seeded prologue spawn", COMBO_GAME_BOTH, 0, 0, 0, 1, 0, 0, false},
        {"Vanilla scripted cutscene", COMBO_GAME_BOTH, 1, 1, 0, 0, 0, 0xFFF3, false},
        {"seeded scripted cutscene", COMBO_GAME_BOTH, 0, 0, 0, 0, 0, 0xFFF3, false},
    };
    unsigned checked=0;
    for (const auto& test : cases) for (int form=0; form<PLAYER_FORM_MAX; ++form) {
        active=test.name;
        gComboGameMode=test.mode; gComboVanillaMode=test.vanilla;
        gSaveContext={};
        gSaveContext.gameMode=GAMEMODE_NORMAL; gSaveContext.fileNum=0;
        gSaveContext.sceneLayer=test.layer;
        gSaveContext.save.entrance=ENTRANCE(LOST_WOODS, test.spawn);
        gSaveContext.save.cutsceneIndex=test.cutscene;
        gSaveContext.save.isFirstCycle=test.firstCycle;
        gSaveContext.save.playerForm=form;
        gSaveContext.save.saveInfo.playerData.threeDayResetCount=test.resets;
        PlayState play{}; play.sceneId=SCENE_LOST_WOODS;
        ActorEntry spawn{}; spawn.params=PLAYER_PARAMS(3, PLAYER_START_MODE_D);
        const auto originalSpawn=spawn;
        play.linkActorEntry=&spawn;
        alignas(Gfx) static const char portal[]="__OTR__custom/comboship/native_forest/portal_root";
        RoomShapeCullableEntry entry{}; entry.opa=(Gfx*)portal;
        RoomShape shape{}; shape.cullable.base.type=ROOM_SHAPE_TYPE_CULLABLE;
        shape.cullable.numEntries=1; shape.cullable.entries=&entry;
        play.roomCtx.curRoom.roomShape=&shape;
        const auto before=gSaveContext;
        const bool exploration=ComboOpeningForest_IsExploration(&play);
        Require(exploration==test.exploration, "actual exploration predicate selected the wrong opening policy");
        int probes=0;
        auto exists=[&](const char*) { ++probes; return true; };
        auto dependencies=[](const char*) {
            return NativeForestDependencies::Graph{{"custom/comboship/native_forest/geometry/ancestor"},
                                                  {"scenes/native/required"}};
        };
        const char* selected=NativeForest::CombinedScene(exploration, exists, dependencies);
        if (test.exploration) {
            Require(selected && !std::strcmp(selected, "scenes/nonmq/comboship_forest/comboship_forest"),
                    "post-reset or seeded exploration must retain the actual private scene choice");
            Require(probes>0, "eligible scene selection must validate resource availability");
        } else {
            Require(!selected && probes==0, "native opening must not select or probe custom forest resources");
        }
        ComboOpeningForest_PrepareSpawn(&play);
        Require(!std::memcmp(&spawn, &originalSpawn, sizeof(spawn)), "cached source spawn must remain immutable");
        if (test.exploration) {
            Require(play.linkActorEntry!=&spawn && play.linkActorEntry->params==PLAYER_PARAMS(0xFF, PLAYER_START_MODE_E),
                    "eligible exploration must retain its ordinary-control spawn adapter");
        } else {
            Require(play.linkActorEntry==&spawn, "native opening must retain its original spawn and start mode");
        }
        const Vec3f point={-695, 0, -313};
        Require(ComboOpeningForest_ReturnPackVersion(&play)==(test.exploration ? 9 : 0),
                "native prologue must not acquire the custom return mesh");
        Require(bool(ComboOpeningForest_AtReturn(&play, &point))==test.exploration,
                "custom return must remain inactive during the native opening");
        Require(!std::memcmp(&before, &gSaveContext, sizeof(before)),
                "resource/spawn/return policy must preserve all save and progression state");
        ++checked;
    }
    std::printf("PASS native first-cycle forest selection, spawn and return: %u form/session controls\n", checked);
}
'''

with tempfile.TemporaryDirectory(prefix="first-cycle-resource-") as directory:
    folder = Path(directory)
    source, binary = folder / "first_cycle.cpp", folder / "first_cycle"
    source.write_text(SOURCE)
    includes = ("", "mm", "mm/2s2h", "mm/include", "mm/include/PR", "mm/src", "mm/assets",
                "libultraship/include", "combo")
    subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-w", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                    "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-no-pie",
                    *["-I" + str(ROOT / path) for path in includes], str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
