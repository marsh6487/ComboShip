"""Exercise both native Zora clip selectors with optional archive resources."""
import os
from pathlib import Path
import subprocess
import tempfile

from run_presentation_tests import function

ROOT = Path(__file__).resolve().parents[2]


def test_mm(directory):
    source = (ROOT / "mm/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
    selectors = source[source.index("typedef enum ZoraLocomotionSlot"):source.index("/**\n * Get the appropriate Idle")]
    selectors += function(source, "func_8082EEE0")
    assert "Player_GetWalkAnim(this)" in function(source, "func_8083F57C")
    assert "Player_GetWalkAnim(this)" in function(source, "func_8083F358")
    assert "Player_GetRunAnim(this)" in function(source, "func_8083A794")
    assert "ZORA_LOCOMOTION_IDLE" in function(source, "Player_GetIdleAnim")
    assert "ZoraLocomotion" not in function(source, "func_8083B73C")
    assert "gPlayerAnim_link_swimer_swim" in function(source, "func_8083B73C")
    fixture = r'''
#include <assert.h>
#include <string.h>
#include "global.h"
#include "2s2h/BenPort.h"
#include "combo/actors/ZeldaRun.h"
#include "combo/actors/ZoraLocomotion.h"

PlayerAnimationHeader* D_8085BE84[PLAYER_ANIMGROUP_MAX][PLAYER_ANIMTYPE_MAX];
static PlayerAnimationHeader stock, zeldaRun;
static PlayerAnimationHeader customIdle = { { 35 } }, customWalk = { { 29 } }, customRun = { { 20 } };
static int loads;
static bool packHasRun;

PlayerAnimationHeader* ResourceMgr_LoadPlayerAnimAsHeaderInPlaceResampled(const char* path, uint8_t stripY,
                                                                        int16_t frames) {
    ++loads;
    assert(!stripY);
    if (strcmp(path, ZORA_LOCOMOTION_IDLE_PATH) == 0) {
        assert(frames == 0);
        return &customIdle;
    }
    if (strcmp(path, ZORA_LOCOMOTION_WALK_PATH) == 0) {
        assert(frames == 29);
        return &customWalk;
    }
    assert(strcmp(path, ZORA_LOCOMOTION_RUN_PATH) == 0 && frames == 20);
    return packHasRun ? &customRun : NULL;
}

void* ZeldaRun_Select(Player* player, void* original) {
    return player->transformation == PLAYER_FORM_HUMAN ? &zeldaRun : original;
}

int ResourceMgr_OTRSigCheck(char* data) {
    return data && strncmp(data, "__OTR__", 7) == 0;
}
''' + selectors + r'''
int main(int argc, char** argv) {
    packHasRun = argc > 1;
    Player player = { 0 };
    player.actor.id = ACTOR_PLAYER;
    player.skelAnime.limbCount = PLAYER_LIMB_MAX;
    D_8085BE84[PLAYER_ANIMGROUP_walk][0] = &stock;
    D_8085BE84[PLAYER_ANIMGROUP_run][0] = &stock;
    D_8085BE84[PLAYER_ANIMGROUP_damage_run][0] = &stock;
    player.transformation = PLAYER_FORM_HUMAN;
    assert(Player_GetWalkAnim(&player) == &stock);
    assert(Player_GetRunAnim(&player) == &zeldaRun);
    player.transformation = PLAYER_FORM_ZORA;
    player.actor.id = ACTOR_EN_TEST3;
    assert(Player_GetWalkAnim(&player) == &stock);
    player.actor.id = ACTOR_PLAYER;
    --player.skelAnime.limbCount;
    assert(Player_GetWalkAnim(&player) == &stock && loads == 0);
    ++player.skelAnime.limbCount;
    assert(Player_GetWalkAnim(&player) == &customWalk);
    assert(Player_SelectZoraLocomotionAnim(&player, ZORA_LOCOMOTION_IDLE, &stock) == &customIdle);
    assert(Player_IdleAnimEquals(&customIdle, &customIdle));
    assert(!Player_IdleAnimEquals(&customIdle, &stock));
    player.unk_B64 = 1;
    assert(func_8082EEE0(&player) == &stock && loads == 2);
    player.unk_B64 = 0;
    PlayerAnimationHeader* expectedRun = packHasRun ? &customRun : &stock;
    assert(func_8082EEE0(&player) == expectedRun && loads == 3);
    assert(Player_GetWalkAnim(&player) == &customWalk && loads == 3);
    assert(Player_GetRunAnim(&player) == expectedRun && loads == 3);
}
'''
    path = directory / "mm_locomotion.c"
    path.write_text(fixture)
    binary = directory / "mm_locomotion"
    includes = ["-I" + str(ROOT / path) for path in
                (".", "mm/include", "mm/src", "mm", "mm/assets", "mm/2s2h", "libultraship/include")]
    subprocess.run([os.environ.get("CC", "cc"), "-std=gnu11", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0",
                    *includes, str(path), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
    subprocess.run([str(binary), "run-present"], check=True)


def test_oot(directory):
    source = (ROOT / "soh/mods/transformation_masks/mm_player_form.cpp").read_text()
    for slot in ("IDLE", "WALK", "RUN"):
        assert "MmForm_LoadLocomotionAnim(form, ZORA_LOCOMOTION_" + slot + "_PATH" in source
    fixture = r'''
#include <cassert>
#include <cstring>
#include "z64animation.h"
#include "mods/anim_translator/mm_anim_loader.h"
#include "mods/transformation_masks/transformation_masks.h"
#include "combo/actors/ZoraLocomotion.h"

static LinkAnimationHeader custom = { { 10 } }, shortClip = { { 1 } }, stock;
static int loads;
LinkAnimationHeader* ResourceMgr_LoadPlayerAnimAsHeaderInPlace(const char* path, u8 stripY) {
    ++loads;
    assert(!stripY);
    if (strcmp(path, ZORA_LOCOMOTION_IDLE_PATH) == 0) {
        return &custom;
    }
    return strcmp(path, ZORA_LOCOMOTION_WALK_PATH) == 0 ? &shortClip : nullptr;
}

LinkAnimationHeader* MmAnim_LoadByPath(const char* path, s16 frames, u8 limbs) {
    assert(strcmp(path, "stock") == 0 && frames == 17 && limbs == 22);
    return &stock;
}
''' + function(source, "MmForm_LoadLocomotionAnim") + r'''
int main() {
    assert(MmForm_LoadLocomotionAnim(MM_PLAYER_FORM_GORON, ZORA_LOCOMOTION_IDLE_PATH, "stock", 17, 22) == &stock);
    assert(loads == 0);
    assert(MmForm_LoadLocomotionAnim(MM_PLAYER_FORM_ZORA, ZORA_LOCOMOTION_IDLE_PATH, "stock", 17, 22) == &custom);
    assert(MmForm_LoadLocomotionAnim(MM_PLAYER_FORM_ZORA, ZORA_LOCOMOTION_WALK_PATH, "stock", 17, 22) == &stock);
    assert(MmForm_LoadLocomotionAnim(MM_PLAYER_FORM_ZORA, ZORA_LOCOMOTION_RUN_PATH, nullptr, 17, 22) == nullptr);
    assert(loads == 3);
}
'''
    path = directory / "oot_locomotion.cpp"
    path.write_text(fixture)
    binary = directory / "oot_locomotion"
    includes = ["-I" + str(ROOT / path) for path in
                (".", "soh/include", "soh", "soh/src", "libultraship/include")]
    subprocess.run([os.environ.get("CXX", "c++"), "-std=gnu++20", "-DF3DEX_GBI_2", *includes, str(path),
                    "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(prefix="zora-locomotion-") as temp:
        test_mm(Path(temp))
        test_oot(Path(temp))
    print("PASS MM and OoT Zora locomotion routing and fallbacks")
