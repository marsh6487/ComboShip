#!/usr/bin/env python3
"""Exercise real Pictograph mode, hover-name routing, and selector invalidation."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile
from run_tests import function

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--cycle-only", action="store_true")
    args = parser.parse_args()
    inventory = (ROOT / "soh/mods/extended_inventory.c").read_text()
    picto = (ROOT / "soh/mods/items/logic/picto_box.c").read_text()
    kaleido = (ROOT / "soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_item.c").read_text()
    resolver = function(inventory, "void* ExtInv_GetCustomItemNameTex(")
    names = sorted(set(re.findall(r"\b(?:ITEM_|EXT_ITEM_)[A-Z0-9_]+", resolver)) | {"ITEM_LENS", "ITEM_NONE"})
    names = [name for name in names if name not in ("ITEM_MM_MASK_POSTMAN", "ITEM_MM_MASK_FIERCE_DEITY")]
    enums = "enum { " + ",".join(names) + ", ITEM_MM_MASK_POSTMAN=5000, ITEM_MM_MASK_FIERCE_DEITY=5024 };\n"
    fixture = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
using u8 = uint8_t; using s32 = int;
enum { SLOT_LENS, PAUSE_ITEM_NONE=65535, NA_SE_SY_CURSOR };
static struct { bool pictoboxOwned{}, quartzOwned{}; } save;
static auto Nei_Save() { return &save; }
static struct { struct { int items[1]; } inventory; } gSaveContext;
static u8 sPictoOnLens;
struct PlayState { struct { int namedItem; } pauseCtx; };
static int gSfxDefaultPos, gSfxDefaultFreqAndVolScale, gSfxDefaultReverb;
static void Audio_PlaySoundGeneral(int, int*, int, int*, int*, int*) {}
static int Slate_RuneCount() { return 0; }
static int Slate_GetRune() { return 0; }
static void* Slate_RuneNameTex(int) { return nullptr; }
static int Wand_GetMode() { return 0; }
static void* Wand_ModeNameTex(int) { return nullptr; }
static bool Nei_CaneOwned() { return false; }
static int Nei_CaneGetType() { return 0; }
static const char* MmMasks_GetNamePath(uint16_t) { return nullptr; }
static void* MmAssets_GetChateauIconPath() { return nullptr; }
struct CustomItemAsset { void* nameTex; };
static const CustomItemAsset* ExtInv_FindCustomItemAsset(uint16_t) { return nullptr; }
#define MAGIC_MIRROR_NAME_TEX "mirror"
'''
    fixture += function(picto, "u8 Picto_IsOnLensActive(") + "\n"
    fixture += function(picto, "void Picto_SetOnLensActive(") + "\n"
    fixture += function(picto, "u8 Picto_IsOwned(") + "\n"
    fixture += resolver + "\n" + function(kaleido, "static void Picto_Cycle(") + "\n"
    fixture += r'''
int main() {
    PlayState play{};
    save.pictoboxOwned = true;
    gSaveContext.inventory.items[SLOT_LENS] = ITEM_LENS;
    sPictoOnLens = 0;
    play.pauseCtx.namedItem = ITEM_LENS;
    Picto_Cycle(&play, 1);
    assert(Picto_IsOnLensActive() == 1);
    assert(play.pauseCtx.namedItem == PAUSE_ITEM_NONE);
    play.pauseCtx.namedItem = ITEM_LENS;
    Picto_Cycle(&play, -1);
    assert(Picto_IsOnLensActive() == 0);
    assert(play.pauseCtx.namedItem == PAUSE_ITEM_NONE);
'''
    if not args.cycle_only:
        fixture += r'''
    for (int language = 0; language != 4; ++language) {
        for (int state = 0; state != 5; ++state) {
            save.pictoboxOwned = state != 0;
            gSaveContext.inventory.items[SLOT_LENS] = state == 1 ? ITEM_NONE : ITEM_LENS;
            sPictoOnLens = state >= 3;
            const char* name = static_cast<const char*>(ExtInv_GetCustomItemNameTex(ITEM_LENS, language));
            if (state == 1 || state >= 3) {
                assert(name && !strcmp(name, "__OTR__item_name_static/gItemNamePictographBoxENGTex"));
            } else {
                assert(!name); // no override: native Lens name remains authoritative
            }
        }
        assert(!ExtInv_GetCustomItemNameTex(ITEM_NONE, language));
    }
'''
    fixture += "}\n"
    with tempfile.TemporaryDirectory(prefix="picto-name-") as directory:
        path = Path(directory)
        (path / "test.cpp").write_text(enums + fixture)
        flags = ["-std=c++20", "-O1", "-g"]
        if args.sanitize:
            flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-no-pie"]
        subprocess.run([os.environ.get("CXX", "g++"), *flags, str(path / "test.cpp"), "-o", str(path / "test")], check=True)
        subprocess.run([str(path / "test")], check=True)
    print("PASS: actual Pictograph selector invalidates hover name; Lens/Pictograph mode routes correctly")


if __name__ == "__main__":
    main()
