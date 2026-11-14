#!/usr/bin/env python3
"""Run OoT Blast Mask controls, HUD, text and native bomb collision regressions.

ROM services (graphics playback, icon loading and actor allocation) are boundary
doubles. Production formatter/codec/font, A arbitration, B HUD display commands,
input filtering, shield setup, damage-recovery timing, bomb lifetime and native
attack collision/damage execute without a running game.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/black_market"))
from run_text_codec_tests import common_flags, compile_and_run, function, render_controls
sys.path.insert(0, str(ROOT / "scripts/ci"))
from native_test_abi import ABI_FLAGS


def flags():
    includes = [ROOT, *[ROOT / "soh" / p for p in
                ("include", "include/PR", "", "soh", "src", "assets", "mods")],
                *[ROOT / p for p in ("combo", "combo/menu", "libultraship/include", "libultraship")]]
    includes += [Path(path) for path in os.environ.get("COMBO_NEI_FIXTURE_CPATH", "/usr/include/imgui").split(os.pathsep) if path]
    return common_flags(includes) + list(ABI_FLAGS) + [
        '-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"', '-DCVAR_PREFIX_CHEAT="gCheats"',
        '-DCVAR_PREFIX_COSMETIC="gCosmetics"']


def definition(source, signature):
    """Extract the definition, skipping matching forward declarations."""
    match = re.search(re.escape(signature) + r"[^;{]*\{", source)
    if not match:
        raise RuntimeError("Missing native definition: " + signature)
    return function(source[match.start():], signature)


def c_flags():
    # The native C port relies on address/integer and leading-Actor pointer
    # conversions that recent C23 compilers otherwise promote to errors.
    return flags() + ["-Wno-error=int-conversion", "-Wno-error=incompatible-pointer-types"]


def blast_code(mask, transform):
    helpers = definition(transform, "u8 TransformMasks_AButtonIsOffered(")
    for signature in ('extern "C" s32 MmMaskWear_CanUseBlastA(',
                      'extern "C" void MmMaskWear_FilterBlastA(',
                      'extern "C" s32 MmMaskWear_CanUseBlastB(',
                      'extern "C" void MmMaskWear_FilterBlastB(',
                      'static bool MmMaskWear_BlastShieldRaised('):
        if signature in mask:
            helpers += "\n" + definition(mask, signature)
    helpers += "\n" + definition(transform, "void TransformMasks_FilterB(")
    return helpers, definition(mask, "static void MmMaskWear_UpdateBlast(")


def codec(folder):
    native = (ROOT / "soh/src/code/z_message_PAL.c").read_text()
    source = (ROOT / "soh/mods/extended_player.c").read_text()
    row = re.search(r"Randomizer_DrawMmMask, RG_MM_MASK_BLAST,\s*(.*?)\s*\},", source, re.S)
    if not row:
        raise RuntimeError("Missing Blast Mask receipt row")
    fixture = (ROOT / "tests/blast_mask/tutorial_test.cpp").read_text()
    fixture = fixture.replace("/* TUTORIAL_TEXT */", "static const char* tutorials[] = {" + row[1] + "};")
    util = (ROOT / "soh/soh/util.cpp").read_text()
    fixture = fixture.replace("/* NATIVE_COPY */", function(util, "size_t SohUtils::CopyStringToCharBuffer("))
    decoder = r'''
#include "global.h"
#include "message_data_static.h"
#include "ComboItemReceiptPresentation.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "textures/message_static/message_static.h"
#include "textures/message_texture_static/message_texture_static.h"
#include <stdlib.h>
#include <string.h>
static u16 sTextBoxNum, sNextTextId;
static u8 sTextboxSkipped;
static bool sDisplayNextMessageAsEnglish;
static s16 sTextIsCredits, sTextFade;
static CwItemReceiptPresentation sItemReceiptPresentation;
static void Message_ApplyItemReceiptLayout(PlayState* play) { (void)play; }
static int Message_HasItemReceiptIcon(void) { return 0; }
static void Message_DecodeJPN(PlayState* play) { (void)play; abort(); }
static void Message_LoadItemIcon(PlayState* play, u16 item, s16 y) {
    (void)y; if(item != ITEM_MM_MASK_BLAST) abort();
    // The native icon loader also consumes the icon operand and sets choiceNum.
    // Keep those codec side effects; only resource copying is outside this test.
    ++play->msgCtx.msgBufPos;
    play->msgCtx.choiceNum = 1;
}
bool Message_DecodeName(PlayState* play, s16* decoded, s32* texture) { (void)play; (void)decoded; (void)texture; abort(); }
void osSyncPrintfUnused(const char* format, ...) { (void)format; }
void gSPInvalidateTexCache(Gfx* packet, uintptr_t texture) { (void)packet; (void)texture; }
bool GameInteractor_Should(GIVanillaBehavior behavior, uint32_t result, ...) { (void)behavior; return result; }
'''
    decoder += function(native, "void Message_Decode(PlayState*") + "\n" + function(native, "u8 Message_GetState(")
    c_path = folder / "decoder.c"
    c_path.write_text(decoder)
    units = []
    options = flags()
    for path in (c_path, ROOT / "soh/src/code/z_kanfont.c",
                 ROOT / "soh/soh/Enhancements/custom-message/CustomMessageManager.cpp"):
        obj = folder / (path.stem + ".o")
        compiler = os.environ.get("CXX", "c++") if path.suffix == ".cpp" else os.environ.get("CC", "cc")
        standard = "-std=c++20" if path.suffix == ".cpp" else "-std=gnu2x"
        compile_options = options if path.suffix == ".cpp" else c_flags()
        subprocess.run([compiler, standard, *compile_options, "-c", str(path), "-o", str(obj)], check=True)
        units.append(str(obj))
    # The shared render fixture's lifecycle hooks are unused and removed by GC.
    units.append(render_controls(folder, "oot", native, c_flags()))
    compile_and_run(folder, "tutorial", fixture, options, units)


def action(folder):
    player = (ROOT / "soh/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
    interface = (ROOT / "soh/src/code/z_parameter.c").read_text()
    transform = (ROOT / "soh/mods/transformation_masks/transformation_masks.c").read_text()
    mask = (ROOT / "soh/mods/transformation_masks/mm_mask_wear.cpp").read_text()
    fixture = (ROOT / "tests/blast_mask/action_test.cpp").read_text()
    table = re.search(r"static const char\* actionsTbl\[\] = \{.*?\n\};", interface, re.S)[0]
    interface_code = table + "\n" + "\n".join(function(interface, signature) for signature in (
        "void Interface_LoadActionLabel(", "void Interface_SetDoAction("))
    hud_source = (ROOT / "tests/blast_mask/hud_native.c").read_text()
    hud_defines = "\n".join(re.search(r"^#define " + re.escape(name) + r"[^\n]*", interface, re.M)[0]
                            for name in ("PLAYER_ON_REAL_HORSE", "DO_ACTION_TEX_WIDTH", "DO_ACTION_TEX_HEIGHT"))
    hud_end = interface.index("        // C-Left Button Icon & Ammo Count", interface.index("void Interface_Draw("))
    hud_start = interface.rfind("        s32 blastMaskB =", 0, hud_end)
    if hud_start < 0:
        hud_start = interface.rfind("        if (!interfaceCtx->unk_1FA)", 0, hud_end)
    if hud_start < 0:
        raise RuntimeError("Missing native B HUD branch")
    hud_source = hud_source.replace("/* NATIVE_HUD_DEFINES */", hud_defines)
    hud_source = hud_source.replace("/* NATIVE_B_HUD */", interface[hud_start:hud_end])
    player_code = "\n".join(definition(player, signature) for signature in (
        "s32 Player_FriendlyLockOnOrParallel(", "int Player_IsZTargeting(",
        "void Player_UpdateInterface(", "void Player_SetupTalk("))
    player_lib = (ROOT / "soh/src/code/z_player_lib.c").read_text()
    player_code = function(player_lib, "s32 Player_InBlockingCsMode(") + "\n" + player_code
    # C's player-local identifier 'this' is reserved in the C++ fixture.
    player_code = re.sub(r"\bthis\b", "player", player_code)
    helpers, blast_update = blast_code(mask, transform)
    fixture = fixture.replace("/* INTERFACE */", interface_code).replace("/* PLAYER_INTERFACE */", player_code)
    fixture = fixture.replace("/* BLAST_HELPERS */", helpers).replace("/* BLAST_UPDATE */", blast_update)
    # Execute the C interface code as C, preserving its original conversions.
    interface_path = folder / "interface.c"
    interface_path.write_text('#include "global.h"\n#include "textures/do_action_static/do_action_static.h"\n'
                              '#include "assets/soh_assets.h"\n#include <string.h>\n'
                              '#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"\n' + interface_code
                              + "\n" + hud_source)
    obj = folder / "interface.o"
    subprocess.run([os.environ.get("CC", "cc"), "-std=gnu2x", *c_flags(), "-c", str(interface_path), "-o", str(obj)], check=True)
    compile_and_run(folder, "action", fixture.replace(interface_code, ""), flags(), [str(obj)])


def collision(folder):
    collision = (ROOT / "soh/src/code/z_collision_check.c").read_text()
    math3d = (ROOT / "soh/src/code/sys_math3d.c").read_text()
    lib = (ROOT / "soh/src/code/z_lib.c").read_text()
    bomb = (ROOT / "soh/src/overlays/actors/ovl_En_Bom/z_en_bom.c").read_text()
    player = (ROOT / "soh/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
    player_lib = (ROOT / "soh/src/code/z_player_lib.c").read_text()
    native = (ROOT / "tests/blast_mask/collision_native.c").read_text()
    collider_names = (
        "s32 Collider_InitBase(", "s32 Collider_SetBase(",
        "void Collider_ResetATBase(", "void Collider_ResetACBase(",
        "s32 Collider_InitTouch(", "s32 Collider_InitBump(",
        "s32 Collider_SetTouch(", "s32 Collider_SetBump(",
        "s32 Collider_InitInfo(", "s32 Collider_SetInfo(", "void Collider_ResetATInfo_Unk(",
        "void Collider_ResetATInfo(", "void Collider_ResetACInfo(",
        "s32 Collider_InitJntSphElementDim(", "s32 Collider_SetJntSphElementDim(",
        "s32 Collider_InitJntSphElement(", "s32 Collider_SetJntSphElement(",
        "s32 Collider_ResetJntSphElementAT(", "s32 Collider_InitJntSph(", "s32 Collider_SetJntSph(",
        "s32 Collider_ResetJntSphAT(",
        "s32 Collider_InitCylinderDim(", "s32 Collider_SetCylinderDim(",
        "s32 Collider_InitCylinder(", "s32 Collider_SetCylinder(", "s32 Collider_ResetCylinderAC(",
        "void Collider_UpdateCylinder(")
    native = native.replace("/* NATIVE_COLLIDER */", "\n".join(definition(collision, x) for x in collider_names))
    math_code = definition(math3d, "s32 Math3D_SphVsCylOverlapCenterDist(")
    math_code += "\n" + "\n".join(definition(lib, x) for x in (
        "void Math_Vec3f_Copy(", "s32 Math_StepToF(", "f32 Math_SmoothStepToF("))
    native = native.replace("/* NATIVE_MATH */", math_code)
    collision_code = "\n".join(definition(collision, x) for x in (
        "s32 CollisionCheck_SkipTouch(", "s32 CollisionCheck_SkipBump(",
        "s32 CollisionCheck_NoSharedFlags(", "void CollisionCheck_SetBounce(",
        "s32 CollisionCheck_SetATvsAC(", "void CollisionCheck_AC_JntSphVsCyl("))
    dispatch = re.search(r"static ColChkVsFunc sACVsFuncs\[4\]\[4\] = \{.*?\n\};", collision, re.S)[0]
    # Preserve native indexing. Unexercised collider shapes abort rather than
    # introduce unrelated physics dependencies or silently succeed.
    dispatch = re.sub(r"CollisionCheck_AC_(?!JntSphVsCyl\b)\w+", "BlastTest_UnsupportedShape", dispatch)
    collision_code += "\ntypedef void (*ColChkVsFunc)(PlayState*, CollisionCheckContext*, Collider*, Collider*);\n"
    collision_code += "static void BlastTest_UnsupportedShape(PlayState* p, CollisionCheckContext* c, Collider* a, Collider* b) { abort(); }\n"
    collision_code += dispatch + "\n" + "\n".join(definition(collision, x) for x in (
        "void CollisionCheck_AC(", "void CollisionCheck_AT(",
        "void CollisionCheck_ApplyDamage(", "void CollisionCheck_ApplyDamageCyl("))
    native = native.replace("/* NATIVE_COLLISION */", collision_code)
    bomb_data = bomb[bomb.index("static ColliderCylinderInit sCylinderInit"):bomb.index("void EnBom_SetupAction(")]
    bomb_code = "void EnBom_Move(EnBom*, PlayState*);\nvoid EnBom_WaitForRelease(EnBom*, PlayState*);\n"
    bomb_code += bomb_data + "\n" + "\n".join(definition(bomb, x) for x in (
        "void EnBom_SetupAction(", "void EnBom_Init(", "void EnBom_Move(",
        "void EnBom_WaitForRelease(", "void EnBom_Explode(", "void EnBom_Update("))
    native = native.replace("/* NATIVE_BOMB */", bomb_code)
    shield_code = "\n".join(definition(player_lib, x) for x in (
        "s32 Player_CheckHostileLockOn(", "s32 Player_IsChildWithHylianShield(",
        "s32 Player_HoldsTwoHandedWeapon(", "void Player_SetModelsForHoldingShield("))
    shield_code += "\n" + "\n".join(definition(player, x) for x in (
        "s32 Player_FriendlyLockOnOrParallel(", "int Player_IsZTargeting(",
        "void Player_SetUpperActionFunc(", "void func_80834894(", "s32 func_80834B5C(",
        "LinkAnimationHeader* Player_StartUpperBodyShield(",
        "s32 Player_TryRaiseUpperBodyShield(", "s32 Player_ActionHandler_11("))
    native = native.replace("/* NATIVE_SHIELD */", shield_code)
    recovery = re.search(r"typedef enum PlayerActionInterruptResult \{.*?\} PlayerActionInterruptResult;", player, re.S)[0]
    recovery += "\n" + re.search(r"^#define SPEED_MODE_CURVED[^\n]*", player, re.M)[0]
    recovery += "\n" + "\n".join(definition(player, x) for x in (
        "s32 Player_TryActionInterrupt(", "void Player_Action_8084370C("))
    native = native.replace("/* NATIVE_DAMAGE_RECOVERY */", recovery)
    body = re.search(r"static ColliderCylinderInit D_80854624 = \{.*?\n\};", player, re.S)[0]
    native = native.replace("/* NATIVE_PLAYER_BODY */", body)
    native_path = folder / "collision_native.c"
    native_path.write_text(native)
    obj = folder / "collision_native.o"
    # Native C uses Player* as its leading Actor* for sound and uses signed
    # (1 << 31) damage-flag constants. Preserve the production bodies while
    # allowing those legacy conventions; keep ASan and all other UBSan checks.
    subprocess.run([os.environ.get("CC", "cc"), "-std=gnu2x", *c_flags(), "-fno-sanitize=shift-base",
                    "-c", str(native_path), "-o", str(obj)], check=True)
    mask = (ROOT / "soh/mods/transformation_masks/mm_mask_wear.cpp").read_text()
    transform = (ROOT / "soh/mods/transformation_masks/transformation_masks.c").read_text()
    helpers, blast_update = blast_code(mask, transform)
    fixture = (ROOT / "tests/blast_mask/collision_test.cpp").read_text()
    fixture = fixture.replace("/* BLAST_HELPERS */", helpers).replace("/* BLAST_UPDATE */", blast_update)
    compile_and_run(folder, "collision", fixture, flags(), [str(obj)])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--part", choices=("codec", "action", "collision", "all"), default="all")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="blast-mask-") as directory:
        folder = Path(directory)
        if args.part in ("codec", "all"):
            codec(folder)
        if args.part in ("action", "all"):
            action(folder)
        if args.part in ("collision", "all"):
            collision(folder)


if __name__ == "__main__":
    main()
