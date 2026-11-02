#!/usr/bin/env python3
"""Execute both actual goal predicates and native boss ending branches.

Persisted CVar lookup and peer completion are service boundaries. Vanilla and
solo sessions must not consult seeded goals or suppress their native ending.
"""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    end = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


PREAMBLE = r'''
#include <cstdlib>
#include <cstring>
#include <iostream>
#include "combo/vanilla/ComboVanillaMode.h"
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
int gComboVanillaMode = 0;
}
struct Session { int mode, vanilla; bool seededGoal; };
const Session sessions[] = {
    {COMBO_GAME_BOTH, 1, false},
    {COMBO_GAME_MM, 0, false}, {COMBO_GAME_OOT, 0, false},
    {COMBO_GAME_MM, 1, false}, {COMBO_GAME_OOT, 1, false},
    {COMBO_GAME_BOTH, 0, true},
};
'''

SETTINGS = r'''
static int enabled, goalMode, reads;
int CVarGetInteger(const char* name, int) {
    ++reads;
    if (!std::strcmp(name, "isFleetShipCombo.Enabled")) return enabled;
    if (!std::strcmp(name, "gFleetCombo.GoalMode")) return goalMode;
    std::cerr << "FAIL unexpected goal setting: " << name << '\n';
    std::exit(1);
}
'''

PREDICATE_CHECKS = r'''
int main() {
    struct Goal { int enabled, mode; bool gated; };
    const Goal goals[] = {
        {1, 0, true}, {1, 1, false}, {1, 7, false},
        {0, 0, false}, {0, 1, false}, {-1, 0, true},
    };
    int cases = 0;
    for (const auto& session : sessions) for (const auto& goal : goals) {
        gComboGameMode = session.mode;
        gComboVanillaMode = session.vanilla;
        enabled = goal.enabled; goalMode = goal.mode; reads = 0;
#ifdef COMBO_BUILD
        const bool expected = session.seededGoal ? goal.gated : false;
        const bool mustSkipSettings = !session.seededGoal;
#else
        // The standalone Fleet layout keeps its existing CVar policy.
        const bool expected = goal.gated;
        const bool mustSkipSettings = false;
#endif
        const bool actual = CHECK_GOAL();
        if (actual != expected || (mustSkipSettings && reads != 0)) {
            std::cerr << "FAIL " << TEST_NAME << ": session=" << session.mode
                      << " vanilla=" << session.vanilla << " enabled=" << enabled
                      << " goal=" << goalMode << " gate=" << actual
                      << " expected=" << expected << " settingReads=" << reads << '\n';
            return 1;
        }
        ++cases;
    }
    std::cout << "PASS " << TEST_NAME << ": " << cases << " session/settings controls\n";
}
'''

ENDING_STATE = r'''
enum { ENTR_TEMPLE_OF_TIME_WARP_PAD=10, ENTR_CHAMBER_OF_THE_SAGES_0=11,
       SOUTH_CLOCK_TOWN=20, TERMINA_FIELD=21, TRANS_TRIGGER_START=1,
       TRANS_TYPE_FADE_WHITE=2, LINK_AGE_ADULT=0 };
#define ENTRANCE(scene,spawn) ((scene)*512+(spawn))
struct { int nextCutsceneIndex=0, fileNum=2; } gSaveContext;
struct PlayState { int nextEntranceIndex=-1, nextEntrance=-1, transitionTrigger=0,
                       transitionType=0, linkAgeOnLoad=-1; };
static int peerCalls, peerDead;
int (*gComboFinalBossDefeated)(int, int) = nullptr;
int PeerResult(int game, int slot) {
    if (game != GAME_ID || slot != 2) {
        std::cerr << "FAIL incorrect boss callback identity\n";
        std::exit(1);
    }
    ++peerCalls;
    return peerDead;
}
'''

ENDING_CHECKS = r'''
int main() {
    int cases=0;
    for (const auto& session : sessions) for (int callback : {0, 1, 2}) {
        gComboGameMode=session.mode; gComboVanillaMode=session.vanilla;
        gComboFinalBossDefeated=callback ? PeerResult : nullptr;
        peerCalls=0; peerDead=callback==2;
        gSaveContext.nextCutsceneIndex=0;
#ifdef COMBO_BUILD
        const bool nativeEnding=!session.seededGoal || callback==2;
        const int calls=session.seededGoal && callback ? 1 : 0;
#else
        const bool nativeEnding=true;
        const int calls=0;
#endif
        PlayState play;
        FinishBoss(&play);
        const int entrance=nativeEnding ? NATIVE_DESTINATION : PORTAL_DESTINATION;
        const int cutscene=nativeEnding ? NATIVE_CUTSCENE : 0xFFEF;
        if (play.DESTINATION != entrance || gSaveContext.nextCutsceneIndex != cutscene ||
            play.transitionTrigger != TRANS_TRIGGER_START || peerCalls != calls) {
            std::cerr << "FAIL " << TEST_NAME << ": session=" << session.mode
                      << " vanilla=" << session.vanilla << " callback=" << callback
                      << " destination=" << play.DESTINATION << " expected=" << entrance
                      << " peerCalls=" << peerCalls << " expectedCalls=" << calls << '\n';
            return 1;
        }
        ++cases;
    }
    std::cout << "PASS " << TEST_NAME << ": " << cases << " native ending/peer controls\n";
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-ref", help="Check committed goal branches against the same controls")
    args = parser.parse_args()

    def read(path):
        return (subprocess.check_output(["git", "show", args.source_ref + ":" + path], cwd=ROOT, text=True)
                if args.source_ref else (ROOT / path).read_text())

    programs = {}
    for host, path, signature, name in (
        ("mm", "mm/2s2h/FleetShipCombo/FleetComboGoal.cpp", "static bool BeatBothBosses()", "BeatBothBosses"),
        ("oot", "soh/soh/FleetShipCombo/FleetShipCombo.cpp", "int FleetCombo_BeatBothBosses(",
         "FleetCombo_BeatBothBosses"),
    ):
        source = read(path)
        macro = re.search(r"^#define CVAR_COMBO .+$", source, re.M)
        programs[host + "-predicate"] = (PREAMBLE + SETTINGS + (macro[0] + "\n" if macro else "") +
                                         function(source, signature) + "\n#define CHECK_GOAL " + name + "\n" +
                                         PREDICATE_CHECKS)

    ganon = read("soh/src/overlays/actors/ovl_Boss_Ganon2/z_boss_ganon2.c")
    start = ganon.index("// ComboShip: gate the ending on both bosses.")
    start = ganon.rindex("        case 20:", 0, start)
    end = ganon.index("\n    }", start)
    programs["oot-native-ending"] = (PREAMBLE + "#define GAME_ID 0\n" + ENDING_STATE +
        "void FinishBoss(PlayState* play) { switch (20) {\n" + ganon[start:end] + "\n} }\n" +
        "#define DESTINATION nextEntranceIndex\n#define NATIVE_DESTINATION ENTR_CHAMBER_OF_THE_SAGES_0\n"
        "#define PORTAL_DESTINATION ENTR_TEMPLE_OF_TIME_WARP_PAD\n#define NATIVE_CUTSCENE 0xFFF2\n" + ENDING_CHECKS)

    majora = read("mm/src/overlays/actors/ovl_Boss_07/z_boss_07.c")
    arrival = function(majora, "if (this->cutsceneTimer == (u32)(KREG(94) + 440))")
    arrival = arrival[arrival.index("{") + 1:-1]
    programs["mm-native-ending"] = (PREAMBLE + "#define GAME_ID 1\n" + ENDING_STATE +
        "void FinishBoss(PlayState* play) {\n" + arrival + "\n}\n" +
        "#define DESTINATION nextEntrance\n#define NATIVE_DESTINATION ENTRANCE(TERMINA_FIELD, 0)\n"
        "#define PORTAL_DESTINATION ENTRANCE(SOUTH_CLOCK_TOWN, 0)\n#define NATIVE_CUTSCENE 0xFFF7\n" + ENDING_CHECKS)

    failed = []
    with tempfile.TemporaryDirectory(prefix="combo-vanilla-goal-") as directory:
        folder = Path(directory)
        for name, program in programs.items():
            for combined in (True, False):
                variant = name + ("-combined" if combined else "-standalone")
                cpp, binary = folder / (variant + ".cpp"), folder / variant
                cpp.write_text('#define TEST_NAME "' + variant + '"\n' + program)
                subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                                *(["-DCOMBO_BUILD"] if combined else []), "-I" + str(ROOT), str(cpp),
                                "-o", str(binary)], check=True)
                if subprocess.run([str(binary)]).returncode:
                    failed.append(variant)
    if failed:
        raise SystemExit("Failing goal branches: " + ", ".join(failed))


if __name__ == "__main__":
    main()
