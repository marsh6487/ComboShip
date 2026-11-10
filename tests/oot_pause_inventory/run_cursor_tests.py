#!/usr/bin/env python3
"""Exercise real OoT pause navigation and all sixteen cursor vertices on empty cells.

The supplied sparse inventory trapped analog navigation at Light Arrow when
Pause Any Cursor was enabled. Neutral frames synthesized rightward input, and
empty cells retained three cursor corners from the inactive quest page.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

from run_tests import CHECKS, PREFIX, ROOT, function


PREFIX = PREFIX.replace("#define IS_RANDO 0", "#define IS_RANDO testIsRando")
PREFIX = PREFIX.replace(
    "SaveContext gSaveContext;",
    "static int testIsRando, pauseAnySetting, dpadNavigation, sInDungeonScene;\nSaveContext gSaveContext;",
)
PREFIX = PREFIX.replace(
    "    return fallback;",
    '    if (!strcmp(key,CVAR_ENHANCEMENT("PauseAnyCursor"))) return pauseAnySetting;\n'
    '    if (!strcmp(key,CVAR_SETTING("DPadOnPause"))) return dpadNavigation;\n'
    "    return fallback;",
)
PREFIX = PREFIX.replace(
    "void KaleidoScope_SetCursorVtx(PauseContext* pause, u16 index, Vtx* vertices) {}\n", ""
)
SETUP = CHECKS[:CHECKS.index("int main(")].replace("cursorVertices[4]", "cursorVertices[20]")

CASES = r'''
static void seed(int page, int policy, int rando, int dpad) {
    reset();
    sExtInvState.currentPage=page;
    pauseAnySetting=policy; testIsRando=rando; dpadNavigation=dpad;
    const int slots[]={0,1,7,16,18,20,21};
    const u16 items[]={ITEM_STICK,ITEM_NUT,ITEM_OCARINA_TIME,ITEM_ARROW_LIGHT,ITEM_BOTTLE,ITEM_NET,ITEM_BUG};
    for (int i=0;i<7;i++) {
        if (page==0) gSaveContext.inventory.items[slots[i]]=items[i];
        else nei.ownedItems[(page-1)*24+slots[i]]=items[i];
    }
    for (int slot=0;slot<24;slot++) {
        vertices[slot*4].v.ob[0]=-94+32*(slot%6);
        vertices[slot*4].v.ob[1]=56-32*(slot/6);
    }
    play.pauseCtx.cursorPoint[PAUSE_ITEM]=play.pauseCtx.cursorSlot[PAUSE_ITEM]=16;
    play.pauseCtx.cursorX[PAUSE_ITEM]=4; play.pauseCtx.cursorY[PAUSE_ITEM]=2;
    play.pauseCtx.cursorItem[PAUSE_ITEM]=ITEM_ARROW_LIGHT;
}

static void frame(int x, int y, u16 buttons) {
    play.pauseCtx.stickRelX=x; play.pauseCtx.stickRelY=y;
    play.state.input[0].press.button=buttons;
    Step(&play);
}

static void direction(int x, int y) {
    if (dpadNavigation) {
        frame(0,0,(x<0?BTN_DLEFT:x>0?BTN_DRIGHT:0)|(y<0?BTN_DDOWN:y>0?BTN_DUP:0));
    } else frame(x*40,y*40,0);
}

static void selected(int point, int item) {
    assert(play.pauseCtx.cursorSpecialPos==0);
    assert(play.pauseCtx.cursorPoint[PAUSE_ITEM]==point);
    assert(play.pauseCtx.cursorSlot[PAUSE_ITEM]==point);
    assert(play.pauseCtx.cursorX[PAUSE_ITEM]==point%6);
    assert(play.pauseCtx.cursorY[PAUSE_ITEM]==point/6);
    assert(play.pauseCtx.cursorItem[PAUSE_ITEM]==item);
}

static void stationary_empty(void) {
    for (int page=0;page<3;page++) for (int dpad=0;dpad<2;dpad++) for (int rando=0;rando<2;rando++) {
        seed(page,rando?PAUSE_ANY_CURSOR_RANDO_ONLY:PAUSE_ANY_CURSOR_ALWAYS_ON,rando,dpad);
        direction(-1,0); selected(15,PAUSE_ITEM_NONE);
        for (int i=0;i<20;i++) {
            frame(0,0,0);
            assert(play.pauseCtx.cursorPoint[PAUSE_ITEM]==15 && "empty-cell selection must not synthesize rightward input");
            selected(15,PAUSE_ITEM_NONE);
        }
        frame(0,0,BTN_CLEFT);
        assert(equipCalls==0 && "empty cell must not equip the previous Light Arrow");
    }
}

static void reach_owned_items(void) {
    for (int page=0;page<3;page++) for (int dpad=0;dpad<2;dpad++) for (int rando=0;rando<2;rando++) {
        seed(page,rando?PAUSE_ANY_CURSOR_RANDO_ONLY:PAUSE_ANY_CURSOR_ALWAYS_ON,rando,dpad);
        direction(-1,0); selected(15,PAUSE_ITEM_NONE);
        direction(-1,0); selected(14,PAUSE_ITEM_NONE);
        direction(-1,0); selected(13,PAUSE_ITEM_NONE);
        direction(-1,0); selected(12,PAUSE_ITEM_NONE);
        direction(0,-1); selected(18,ITEM_BOTTLE);
        frame(0,0,BTN_CLEFT);
        assert(equipCalls==1 && equippedItem==ITEM_BOTTLE && equippedSlot==page*24+18);
        direction(0,1); selected(12,PAUSE_ITEM_NONE);
        direction(0,1); selected(6,PAUSE_ITEM_NONE);
        direction(0,1); selected(0,ITEM_STICK);
        direction(1,0); selected(1,ITEM_NUT);
        direction(0,-1); selected(7,ITEM_OCARINA_TIME);
        direction(0,-1); selected(13,PAUSE_ITEM_NONE);
        direction(0,-1); selected(19,PAUSE_ITEM_NONE);
        direction(1,0); selected(20,ITEM_NET);
        direction(1,0); selected(21,ITEM_BUG);
    }
}

static void vertical_empty(void) {
    for (int dpad=0;dpad<2;dpad++) {
        seed(0,PAUSE_ANY_CURSOR_ALWAYS_ON,0,dpad);
        direction(-1,0); selected(15,PAUSE_ITEM_NONE);
        direction(0,-1); selected(21,ITEM_BUG);
        direction(0,1); selected(15,PAUSE_ITEM_NONE);
        direction(0,1); selected(9,PAUSE_ITEM_NONE);
    }
}

static void aligned_empty_cursor(void) {
    for (int page=0;page<3;page++) {
        seed(page,PAUSE_ANY_CURSOR_ALWAYS_ON,0,0);
        // The inactive quest page is drawn before the active item page. Its
        // default medallion vertex is (74,38); exercise the real cursor helpers.
        Vtx questVertex={0}; questVertex.v.ob[0]=74; questVertex.v.ob[1]=38;
        KaleidoScope_SetCursorVtx(&play.pauseCtx,0,&questVertex);
        direction(-1,0); selected(15,PAUSE_ITEM_NONE);
        // Hand-checked positions for the empty item cell in row 3, column 4.
        const s16 want[16][2]={
            {1,-7},{17,-7},{1,-23},{17,-23},
            {15,-7},{31,-7},{15,-23},{31,-23},
            {1,-21},{17,-21},{1,-37},{17,-37},
            {15,-21},{31,-21},{15,-37},{31,-37}
        };
        for (int i=0;i<16;i++) for (int axis=0;axis<2;axis++)
            assert(cursorVertices[i].v.ob[axis]==want[i][axis] && "all four cursor corners must follow the selected empty cell");
    }
}

static void disabled_policy(void) {
    for (int page=0;page<3;page++) for (int dpad=0;dpad<2;dpad++) for (int rando=0;rando<2;rando++) {
        seed(page,rando?PAUSE_ANY_CURSOR_ALWAYS_OFF:PAUSE_ANY_CURSOR_RANDO_ONLY,rando,dpad);
        direction(-1,0); selected(21,ITEM_BUG);
        frame(0,0,0); selected(21,ITEM_BUG);
        direction(-1,0); selected(20,ITEM_NET);
        direction(-1,0); selected(18,ITEM_BOTTLE);
        direction(0,1); selected(0,ITEM_STICK);
        direction(1,0); selected(1,ITEM_NUT);
    }
}

static void empty_page_cursor(void) {
    seed(0,PAUSE_ANY_CURSOR_ALWAYS_OFF,0,0);
    Vtx questVertex={0}; questVertex.v.ob[0]=74; questVertex.v.ob[1]=38;
    KaleidoScope_SetCursorVtx(&play.pauseCtx,0,&questVertex);
    frame(0,0,BTN_L);
    assert(ExtInv_GetCurrentPage()==1 && play.pauseCtx.cursorItem[PAUSE_ITEM]==PAUSE_ITEM_NONE);
    for (int i=0;i<16;i++) {
        assert(cursorVertices[i].v.ob[0]==0 && cursorVertices[i].v.ob[1]==-200 &&
               "a disabled empty cursor must hide all four corners after a page switch");
    }
    assert(equipCalls==0);
}

int main(int argc, char** argv) {
    assert(argc==2);
    if (!strcmp(argv[1],"idle")) stationary_empty();
    else if (!strcmp(argv[1],"reachability")) reach_owned_items();
    else if (!strcmp(argv[1],"vertical")) vertical_empty();
    else if (!strcmp(argv[1],"geometry")) aligned_empty_cursor();
    else if (!strcmp(argv[1],"disabled")) disabled_policy();
    else if (!strcmp(argv[1],"empty-page")) empty_page_cursor();
    else return 2;
    printf("PASS OoT item cursor: %s\n",argv[1]);
    return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-ref")
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()

    def read(path):
        if args.source_ref:
            return subprocess.check_output(["git", "show", args.source_ref + ":" + path], cwd=ROOT, text=True)
        return (ROOT / path).read_text()

    inventory = read("soh/mods/extended_inventory.c")
    forms = read("soh/mods/transformation_masks/custom_forms.cpp").replace('extern "C" ', '')
    ui = read("soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_item.c")
    scope = read("soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_scope_PAL.c")
    draw = function(ui, "KaleidoScope_DrawItemSelect")
    locals_ = draw[draw.index("    Input* input"):draw.index("    OPEN_DISPS")]
    input_ = draw[draw.index("    // Update extended inventory pagination timer"):draw.index("    gDPSetCombineLERP(OVERLAY_DISP")]
    production = "\n".join(function(inventory, name) for name in (
        "TravelTools_IsOwned", "MagicMirror_IsOwned", "ExtInv_Reset", "ExtInv_Update", "ExtInv_CanSwitchPage",
        "ExtInv_SwitchPage", "ExtInv_GetCurrentPage", "ExtInv_GetMaxPages", "ExtInv_IsCustomItemsEnabled",
        "ExtInv_IsMmMasksEnabled", "ExtInv_GetInventorySlot",
    ))
    production += "\n" + function(forms, "RitoMask_IsOwned")
    production += "\n" + function(scope, "KaleidoScope_MoveCursorToSpecialPos")
    production += "\n" + function(scope, "KaleidoScope_UpdateCursorSize")
    production += "\n" + function(ui, "KaleidoScope_SetCursorVtx")
    production += "\nstatic void Step(PlayState* play) {\n" + locals_ + input_ + "\n}\n"
    flags = ["-std=gnu11", "-w", "-DF3DEX_GBI_2", "-DCONTROLLERBUTTONS_T=uint32_t",
             '-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"', '-DCVAR_PREFIX_SETTING="gSettings"',
             "-Werror=implicit-function-declaration", "-Werror=incompatible-pointer-types"]
    if args.sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", "-g"]
    includes = ["-I" + str(ROOT / path) for path in (
        "soh", "soh/include", "soh/include/PR", "soh/src", "soh/assets", "soh/soh",
        "soh/src/overlays/misc/ovl_kaleido_scope", "libultraship/include", "combo", "combo/menu",
    )]
    env = {**os.environ, "ASAN_OPTIONS": "detect_leaks=0", "UBSAN_OPTIONS": "halt_on_error=1"}
    failures = []
    with tempfile.TemporaryDirectory(prefix="oot-item-cursor-") as temporary:
        source = Path(temporary) / "test.c"
        source.write_text(PREFIX + production + SETUP + CASES)
        for combined in (False, True):
            variant = "combo" if combined else "solo"
            binary = Path(temporary) / variant
            subprocess.run([os.environ.get("CC", "cc"), *flags, *(["-DCOMBO_BUILD"] if combined else []),
                            *includes, str(source), "-o", str(binary)], check=True)
            for mode in ("idle", "reachability", "vertical", "geometry", "disabled", "empty-page"):
                if subprocess.run([str(binary), mode], env=env).returncode:
                    failures.append(variant + ":" + mode)
    if failures:
        print("FAIL: " + ", ".join(failures))
    return int(bool(failures))


if __name__ == "__main__":
    raise SystemExit(main())
