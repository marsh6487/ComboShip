#!/usr/bin/env python3
"""Exercise the production OoT transition bodies with Wolf's separate animator.

The Wolf asset loader/audio/camera/equipment are service boundaries. The real
LinkAnimation_Update dispatch and complete transition bodies are compiled here.
Run with --root to compare the original source against a candidate.
"""
from pathlib import Path
import argparse
import os
import re
import subprocess
import tempfile


def function(text, name):
    match = re.search(r"^.*\b" + name + r"\([^;{}]*\)\s*\{", text, re.M)
    if not match:
        raise ValueError(f"Missing production function: {name}")
    pos = text.index("{", match.start()) + 1
    depth = 1
    while depth:
        depth += (text[pos] == "{") - (text[pos] == "}")
        pos += 1
    return text[match.start():pos]


FIXTURE = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
using u8 = uint8_t; using s32 = int32_t; using f32 = float;
/* ENUMS */
struct PlayState;
struct LinkAnimationHeader {};
struct SkelAnime {
    float playSpeed = 1;
    union { s32 (*link)(PlayState*, SkelAnime*); } update{};
};
struct Vec { float x = 0, y = 0, z = 0; };
struct Player {
    struct { Vec velocity, projectedPos; struct { Vec rot; } shape; float yDistToWater = 0; } actor;
    uint32_t stateFlags1 = 0, stateFlags3 = 0;
    float linearVelocity = 0, yaw = 0;
    SkelAnime skelAnime;
};
struct PlayState { struct { struct { struct { uint16_t button; } press; } input[1]; } state; };
struct FormState {
    MmFormStateId state = MMFORM_STATE_INACTIVE;
    MmPlayerTransformation currentForm = MM_PLAYER_FORM_HUMAN, targetForm = MM_PLAYER_FORM_HUMAN;
    int cutsceneTimer = 0, cutscenePhase = 0, flashAlpha = 0, skeletonLoaded = 0;
    SkelAnime formSkelAnime;
} gFormState;
constexpr uint32_t PLAYER_STATE1_INPUT_DISABLED = 1, PLAYER_STATE1_IN_CUTSCENE = 2,
                   PLAYER_STATE1_IN_WATER = 4, PLAYER_STATE3_PAUSE_ACTION_FUNC = 1;
constexpr int MM_NA_SE_PL_PUT_OUT_ITEM = 1, MM_NA_SE_IT_SET_TRANSFORM_MASK = 2,
              MM_NA_SE_PL_FREEZE_S = 3, MM_NA_SE_IT_TRANSFORM_MASK_BROKEN = 4,
              MM_NA_SE_PL_TRANSFORM_VOICE = 5, MM_NA_SE_EV_LIGHTNING_HARD = 6,
              MM_NA_SE_PL_FACE_CHANGE = 7, MM_ANIM_CL_SETMASK = 1;
constexpr float ZORA_SWIM_THRESHOLD = 20;
static u8 sNativeMaskInstantFallback = 0, sForceInstantTransform = 0, sCutsceneMaskAnim = 0;
static bool selected = true, instant = false, loadOk = true;
static int loads = 0, soundsIn = 0, soundsOut = 0, formTicks = 0, restricted = 0, restored = 0;
#define MMFORM_LOG(...) ((void)0)
#define GET_ACTIVE_CAM(play) (0)
#define MMFORM_IS_ZORA_SWIM() (gFormState.currentForm == MM_PLAYER_FORM_ZORA)
int CVarGetInteger(const char*, int) { return instant; }
u8 WolfLinkForm_IsSelected() { return selected; }
void WolfLinkForm_PlayTransformSfx(u8 in) { in ? ++soundsIn : ++soundsOut; }
int Camera_GetCamDirYaw(int) { return 0; }
bool MmSfx_IsAvailable() { return true; }
void MmSfx_PlayTransformFlash() {}
void MmSfx_PlayAtPos(int, const void*) {}
int ValidFormUpdate(PlayState*, SkelAnime*) { ++formTicks; return 0; }
u8 MmForm_LoadFormSkeleton(PlayState*, MmPlayerTransformation form) {
    ++loads;
    if (!loadOk) return 0;
    // Wolf's SSBB animation service does not initialize this MM skeleton.
    if (form != MM_PLAYER_FORM_PIKACHU) gFormState.formSkelAnime.update.link = ValidFormUpdate;
    gFormState.skeletonLoaded = 1;
    return 1;
}
void ExtEquip_UnequipForTransform() {}
void ExtEquip_RestoreFromTransform() {}
void MmForm_ApplyFormProperties(Player*, MmPlayerTransformation) {}
void Player_SetBootData(PlayState*, Player*) {}
void MmForm_SaveAndRestrictEquips(PlayState*) { ++restricted; }
void MmForm_RestoreEquips(PlayState*) { ++restored; }
void MmForm_RestoreOotState(Player*) {}
void MmForm_EnterSwimIdle(Player*, PlayState*) {}
LinkAnimationHeader* MmAnim_Load(int) { return nullptr; }
void LinkAnimation_PlayOnce(PlayState*, SkelAnime*, LinkAnimationHeader*) {}
u8 MmForm_UpdateNativeMaskCutscene(Player*, PlayState*, int) { return 0; }
/* NATIVE_FORM_CHECK */
/* ANIMATION_DISPATCH */
/* CONTROLLERS */
static int Run(Player& player, PlayState& play, bool entering) {
    for (int tick = 1; tick <= 100; ++tick) {
        if (entering) MmForm_UpdateTransforming(&player, &play);
        else MmForm_UpdateDetransforming(&player, &play);
        if (gFormState.state != MMFORM_STATE_TRANSFORMING &&
            gFormState.state != MMFORM_STATE_DETRANSFORMING) return tick;
    }
    assert(false && "transition never released control");
    return 0;
}
int main(int argc, char** argv) {
    assert(argc == 2);
    const char* test = argv[1];
    Player player; PlayState play{};
    if (!std::strcmp(test, "entry") || !std::strcmp(test, "buttons") || !std::strcmp(test, "repeat")) {
        const int count = !std::strcmp(test, "repeat") ? 20 : 1;
        for (int cycle = 0; cycle < count; ++cycle) {
            gFormState.cutsceneTimer = gFormState.cutscenePhase = gFormState.flashAlpha = 0;
            gFormState.state = MMFORM_STATE_TRANSFORMING;
            gFormState.targetForm = MM_PLAYER_FORM_PIKACHU;
            // A/B/C/Start input may arrive during a transition with no cutscene.
            play.state.input[0].press.button = !std::strcmp(test, "buttons") ? 0xF00F : 0;
            assert(Run(player, play, true) == 5);
            assert(gFormState.state == MMFORM_STATE_ACTIVE);
            assert(gFormState.currentForm == MM_PLAYER_FORM_PIKACHU);
            assert(gFormState.flashAlpha == 0 && !(player.stateFlags1 & PLAYER_STATE1_INPUT_DISABLED));
            assert(formTicks == 0 && soundsIn == cycle + 1 && restricted == cycle + 1);
            if (count == 1) break;
            gFormState.state = MMFORM_STATE_DETRANSFORMING;
            gFormState.cutsceneTimer = gFormState.cutscenePhase = 0;
            assert(Run(player, play, false) == 5);
            assert(gFormState.state == MMFORM_STATE_INACTIVE);
            assert(gFormState.currentForm == MM_PLAYER_FORM_HUMAN);
            assert(formTicks == 0 && soundsOut == cycle + 1 && restored == cycle + 1);
        }
    } else if (!std::strcmp(test, "exit") || !std::strcmp(test, "exit-after-reload")) {
        gFormState.state = MMFORM_STATE_DETRANSFORMING;
        gFormState.currentForm = MM_PLAYER_FORM_PIKACHU;
        gFormState.targetForm = !std::strcmp(test, "exit-after-reload") ? MM_PLAYER_FORM_FIERCE_DEITY
                                                                      : MM_PLAYER_FORM_PIKACHU;
        assert(Run(player, play, false) == 5);
        assert(gFormState.state == MMFORM_STATE_INACTIVE && gFormState.currentForm == MM_PLAYER_FORM_HUMAN);
        assert(formTicks == 0 && soundsOut == 1 && restored == 1);
        assert(!(player.stateFlags1 & PLAYER_STATE1_INPUT_DISABLED));
    } else if (!std::strcmp(test, "load-failure")) {
        loadOk = false;
        gFormState.state = MMFORM_STATE_TRANSFORMING;
        gFormState.targetForm = MM_PLAYER_FORM_PIKACHU;
        assert(Run(player, play, true) == 1);
        assert(gFormState.state == MMFORM_STATE_INACTIVE && restored == 1 && restricted == 0);
        assert(!(player.stateFlags1 & PLAYER_STATE1_INPUT_DISABLED) && formTicks == 0);
    } else if (!std::strcmp(test, "rito")) {
        gFormState.state = MMFORM_STATE_TRANSFORMING;
        gFormState.targetForm = MM_PLAYER_FORM_RITO;
        assert(Run(player, play, true) > 5);
        assert(gFormState.state == MMFORM_STATE_ACTIVE && formTicks > 0 && soundsIn == 0);
    } else if (!std::strcmp(test, "pokeball-owner")) {
        selected = false;
        gFormState.state = MMFORM_STATE_TRANSFORMING;
        gFormState.targetForm = MM_PLAYER_FORM_PIKACHU;
        for (int i = 0; i < 5; ++i) MmForm_UpdateTransforming(&player, &play);
        assert(gFormState.state == MMFORM_STATE_TRANSFORMING && loads == 0 && soundsIn == 0);
    } else return 2;
    std::printf("PASS %s\n", test);
}
'''


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--case", action="append")
    args = parser.parse_args()
    form = (args.root / "soh/mods/transformation_masks/mm_player_form.cpp").read_text()
    header = (args.root / "soh/mods/transformation_masks/transformation_masks.h").read_text()
    native = (args.root / "soh/mods/transformation_masks/mm_native_transform.inc.h").read_text()
    animation = (args.root / "soh/src/code/z_skelanime.c").read_text()
    enums = []
    for text, name in ((form, "MmFormStateId"), (header, "MmPlayerTransformation")):
        match = re.search(r"typedef enum " + name + r"\s*\{.*?\}\s*" + name + ";", text, re.S)
        assert match, name
        enums.append(match.group(0))
    dispatch = function(animation, "LinkAnimation_Update")
    source = FIXTURE.replace("/* ENUMS */", "\n".join(enums))
    source = source.replace("/* NATIVE_FORM_CHECK */", function(native, "MmForm_IsNativeMaskForm"))
    source = source.replace("/* ANIMATION_DISPATCH */", dispatch)
    source = source.replace("/* CONTROLLERS */", "\n".join(function(form, name) for name in
                            ("MmForm_UpdateTransforming", "MmForm_UpdateDetransforming")))
    with tempfile.TemporaryDirectory(prefix="wolf-transformation-") as folder:
        path = Path(folder)
        (path / "sequence.cpp").write_text(source)
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-g", "-Wall", "-Wextra",
                        "-Werror", "-fsanitize=undefined", "-fno-sanitize-recover=all",
                        str(path / "sequence.cpp"), "-o", str(path / "sequence")], check=True)
        failed = []
        for case in args.case or ("entry", "exit", "exit-after-reload", "buttons", "repeat", "load-failure", "rito", "pokeball-owner"):
            result = subprocess.run([str(path / "sequence"), case], capture_output=True, text=True)
            print(result.stdout, end="")
            if result.returncode:
                print(f"FAIL {case}: exit {result.returncode}\n{result.stderr}", flush=True)
                failed.append(case)
        if failed:
            raise SystemExit(f"{len(failed)} failing case(s): {', '.join(failed)}")


if __name__ == "__main__":
    main()
