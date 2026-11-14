#!/usr/bin/env python3
"""Run the fixed-reward adapter across native OoT talk/message/GI boundaries.

No game is launched. Native message opening/decoding, player talk startup,
accepted-talk consumption, item offering and receipt transactions are real.
Graphics/audio, animation completion, storage and unrelated gameplay services
are controlled boundaries; explicit frame schedules cover immediate and delayed
NPC put-away. See the fixture for the exact ownership and persistence assertions.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/nei_item_stow"))
from run_ballchain_tests import flags as native_flags


def function(source, signature):
    start = source.index(signature)
    while True:
        brace = source.index("{", start)
        if ";" not in source[start:brace]:
            break
        start = source.index(signature, start + len(signature))
    depth, cursor = 1, brace + 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[start:cursor]


def run(*args, **kwargs):
    subprocess.run([str(arg) for arg in args], cwd=ROOT, check=True,
                   env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"}, **kwargs)


parser = argparse.ArgumentParser()
parser.add_argument("--case", default="all")
parser.add_argument("--sanitize", action="store_true")
args = parser.parse_args()
adapter = (ROOT / "soh/soh/Enhancements/Gameplay/ComboVanillaRewards.cpp").read_text()
startup = (ROOT / "soh/soh/Enhancements/Gameplay/ComboStartupReceipt.cpp").read_text()
message = (ROOT / "soh/src/code/z_message_PAL.c").read_text()
player = (ROOT / "soh/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
actor = (ROOT / "soh/src/code/z_actor.c").read_text()
inventory = (ROOT / "soh/mods/extended_inventory.c").read_text()
save = (ROOT / "soh/mods/nei_save.cpp").read_text()
registry = (ROOT / "soh/mods/extended_player.c").read_text()
items = (ROOT / "soh/soh/Enhancements/randomizer/item_list.cpp").read_text()
item_class = (ROOT / "soh/soh/Enhancements/randomizer/item.cpp").read_text()
randomizer = (ROOT / "soh/soh/Enhancements/randomizer/randomizer.cpp").read_text()
fixture = (ROOT / "tests/combo_vanilla/oot_pickup_flow_test.cpp").read_text()

# Keep the merged player's startup-family decision real. Its presentation-only
# acknowledgement must never run for the fixed Vanilla receipts exercised here.
fixture = fixture.replace("/* NATIVE_STARTUP_CLASSIFIER */", "\n".join(function(startup, name) for name in (
    "bool IsReceipt(", 'extern "C" bool ComboStartupOot_IsReceipt(')))
fixture = fixture.replace("/* NATIVE_COPY */", function(
    (ROOT / "soh/soh/util.cpp").read_text(), "size_t SohUtils::CopyStringToCharBuffer("))
fixture = fixture.replace("/* NATIVE_SAVE */", "\n".join(function(save, name) for name in (
    'extern "C" NeiSaveData* Nei_Save(', 'extern "C" uint16_t Nei_GetOwnedItem(',
    'extern "C" void Nei_SetOwnedItem(')))
fixture = fixture.replace("/* NATIVE_ROD */", function(inventory, "void Seasons_GrantRod("))
# Preserve registry identities and native lookup. Only graphics/actor callbacks
# and their unused metadata are omitted from this fixture's registry storage.
registry_rows = []
presentation_ids = set(re.findall(r"\bRG_[A-Z0-9_]+\b", function(adapter, "static RandomizerGet PresentationId(")))
for row in re.findall(r"\{\s*(?:EXT_)?ITEM_[A-Z0-9_]+\s*,[^{}]*?\}", registry, re.S):
    fields = row[1:-1].split(",", 10)
    if len(fields) != 11:
        continue
    item, slot, rg = fields[0].strip(), fields[3].strip(), fields[9].strip()
    if not re.fullmatch(r"(?:RG_[A-Z0-9_]+|NEI_NO_RG)", rg):
        continue
    registry_rows.append(f"{{{item}, 0, 0, {slot}, 0, nullptr, nullptr, nullptr, nullptr, {rg}, nullptr, nullptr, nullptr}}")
    presentation_ids.add(rg)
assert registry_rows
fixture = fixture.replace("/* NATIVE_REGISTRY */", "extern \"C\" {\nstatic const NeiItem sNeiItems[] = {\n" +
    ",\n".join(registry_rows) + "};\n#define NEI_ITEMS_COUNT std::size(sNeiItems)\n" +
    function(registry, "const NeiItem* Nei_FindByItem(") + "\n}")
item_rows = []
for match in re.finditer(r"itemTable\[(RG_[A-Z0-9_]+)\]\s*=\s*Item\(.*?;",
                         re.sub(r"/\*.*?\*/", "", items, flags=re.S), re.S):
    if match[1] in presentation_ids:
        # Icon asset addresses do not affect receipt identity or native grants.
        item_rows.append(re.sub(r"\.CustomIcon\(.*\);$", ";", match[0], flags=re.S))
assert any("itemTable[RG_ROD_OF_SEASONS]" in row for row in item_rows)
fixture = fixture.replace("/* NATIVE_ITEM_TABLE */", "namespace Rando {\n" +
    function(item_class, "Item::Item()") + "\n" +
    function(item_class, "Item::Item(const RandomizerGet randomizerGet_, Text name_, const ItemType type_,") +
    "\n}\nstd::array<Rando::Item, RG_MAX> Rando::StaticData::itemTable;\n" +
    "void Rando::StaticData::InitItemTable() {\n" + "\n".join(item_rows) + "\n}\n" +
    function(items, "Item& Rando::StaticData::RetrieveItem("))

# The actual native randomizer entrypoint through the fixed-receipt dispatch is
# retained verbatim. A hard failure replaces all unrelated randomizer grants,
# so a missed receipt hook cannot silently pass via a fake grant implementation.
dispatch = randomizer[randomizer.index('extern "C" u16 Randomizer_Item_Give('):
                      randomizer.index("    // A live fixed Black Market receipt")]
fixture = fixture.replace("/* NATIVE_RANDOMIZER_DISPATCH */", dispatch +
    '\n#endif\n    Check(false, "fixed receipt fell through to unrelated randomizer grant");\n    return 0;\n}\n')
fixture = fixture.replace("/* ADAPTER_HELPERS */", adapter[
    adapter.index("// BEGIN TESTED NATIVE REWARD HELPERS"):
    adapter.index("// END TESTED NATIVE REWARD HELPERS")])
fixture = fixture.replace("/* ADAPTER_STATE */", adapter[
    adapter.index("constexpr int kReceiptBit"):
    adapter.index("static bool Active()")])
fixture = fixture.replace("/* ADAPTER_FLOW */", "\n".join(function(adapter, name) for name in (
    "static bool Active(", "static int Host(", "static bool Safe(", "static RandomizerGet PresentationId(",
    "static bool BuildEntry(", "static void CancelReceipt(", "static bool AcknowledgeOwned(",
    "static bool BeginReceipt(", "static bool FixedReceiptGrant(", "static void MonitorReceipt(",
    "static const Reward* ActorReward(", "static Actor* CurrentTalkActor(",
    "static void RewardText(", "static void RewardInit(",
    "static void RewardUpdate(")))

engine = r'''
#include "global.h"
#include "libultraship/bridge/consolevariablebridge.h"
#include "message_data_static.h"
#include "ComboItemReceiptPresentation.h"
#include "soh/OTRGlobals.h"
#include "soh/z_message_OTR.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/Enhancements/Gameplay/ComboStartupReceipt.h"
#include "audio/DinsFireSfx.h"
#include "render/DinsFireNative.h"
#include "textures/message_static/message_static.h"
#include "textures/message_texture_static/message_texture_static.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "mods/extended_inventory.h"
#include "sword_element_message.h"
#include "BlackMarketFairy.h"
#include "overlays/actors/ovl_En_Box/z_en_box.h"
#include "overlays/actors/ovl_Bg_Toki_Swd/z_bg_toki_swd.h"
#include "overlays/misc/ovl_kaleido_scope/z_kaleido_scope.h"
#include "vt.h"
#include <stdlib.h>
#include <string.h>
static u16 sTextBoxNum, sNextTextId;
static u8 sTextboxSkipped, sMessageStartFrameCount, sLastLanguage;
static bool sDisplayNextMessageAsEnglish;
static s16 sTextIsCredits, sTextFade, sMessageHasSetSfx;
static CwItemReceiptPresentation sItemReceiptPresentation;
static Input* sControlInput;
static s32 sSavedCurrentMask;
/* NATIVE_DINS_STATE */
void Fixture_BindInput(PlayState* play) {
    sControlInput = &play->state.input[0];
    sSavedCurrentMask = GET_PLAYER(play)->currentMask;
}
extern const char* msgStaticTbl[];
extern MessageTableEntry *sNesMessageEntryTablePtr, *sGerMessageEntryTablePtr, *sFraMessageEntryTablePtr;
extern MessageTableEntry *sJpnMessageEntryTablePtr, *sStaffMessageEntryTablePtr;
static void Message_ApplyItemReceiptLayout(PlayState* play) { (void)play; }
static int Message_HasItemReceiptIcon(void) { return 0; }
static void Message_DecodeJPN(PlayState* play) { (void)play; abort(); }
bool Message_DecodeName(PlayState* play, s16* decoded, s32* texture) { (void)play; (void)decoded; (void)texture; abort(); }
void Message_SetItemReceiptPresentation(const CwItemReceiptPresentation* value) {
    memset(&sItemReceiptPresentation, 0, sizeof(sItemReceiptPresentation));
    if (value) sItemReceiptPresentation = *value;
}
void Message_SetCapeVisibilityChoice(int enabled) { if (enabled) abort(); }
void osSyncPrintfUnused(const char* format, ...) { (void)format; }
void lusprintf(const char* file, int line, int level, const char* format, ...) {
    (void)file; (void)line; (void)level; (void)format;
}
void gSPInvalidateTexCache(Gfx* packet, uintptr_t texture) { (void)packet; (void)texture; }
Vec3f gSfxDefaultPos;
f32 gSfxDefaultFreqAndVolScale;
s8 gSfxDefaultReverb;
void Audio_PlaySfxGeneral(u16 id, Vec3f* pos, u8 token, f32* freq, f32* volume, s8* reverb) {
    (void)id; (void)pos; (void)token; (void)freq; (void)volume; (void)reverb;
}
void Player_Action_WaitForPutAway(Player*, PlayState*);
void Player_Action_Talk(Player*, PlayState*);
void Player_Action_8084E3C4(Player*, PlayState*);
void Player_Action_808507F4(Player*, PlayState*);
void Player_SetupTalk(PlayState*, Player*);
void Player_SetupActionPreserveAnimMovement(PlayState*, Player*, PlayerActionFunc, s32);
s32 Player_SetupAction(PlayState*, Player*, PlayerActionFunc, s32);
s32 Player_PutAwayHeldItem(PlayState*, Player*);
void Player_AnimPlayOnceAdjusted(PlayState*, Player*, LinkAnimationHeader*);
void Player_AnimPlayLoop(PlayState*, Player*, LinkAnimationHeader*);
LinkAnimationHeader* Player_GetIdleAnim(Player*);
void Player_StartAnimMovement(PlayState*, Player*, s32);
void Player_AnimChangeLoopSlowMorph(PlayState*, Player*, LinkAnimationHeader*);
void Player_FinishAnimMovement(Player*);
s32 Player_UpdateUpperBody(Player*, PlayState*);
s32 func_808332B8(Player*);
void func_80832224(Player*);
void func_80832340(PlayState*, Player*);
void func_808326F0(Player*);
void func_80834644(PlayState*, Player*);
void Player_SetTurnAroundCamera(PlayState*, s16);
void Player_StartTalking(PlayState*, Actor*);
void Player_Action_8084E6D4(Player*, PlayState*);
void Player_Action_Idle(Player*, PlayState*);
void Player_Action_8084CC98(Player*, PlayState*);
void Player_Action_8084D610(Player*, PlayState*);
void Player_AnimPlayLoopAdjusted(PlayState*, Player*, LinkAnimationHeader*);
void Player_AnimChangeOnceMorph(PlayState*, Player*, LinkAnimationHeader*);
void Player_AnimPlayOnce(PlayState*, Player*, LinkAnimationHeader*);
void Player_DetachHeldActor(PlayState*, Player*);
void func_8083AE40(Player*, s16);
void func_8083A434(PlayState*, Player*);
void func_8083A0F4(PlayState*, Player*);
void Player_InitItemAction(PlayState*, Player*, s8);
void Player_UseItem(PlayState*, Player*, s32);
void func_8083E4C4(PlayState*, Player*, GetItemEntry*);
void func_808328EC(Player*, u16);
s32 func_8084B4D4(PlayState*, Player*);
s32 func_8084B3CC(PlayState*, Player*);
s32 Player_StartCsAction(PlayState*, Player*);
void func_8083A360(PlayState*, Player*);
void func_80838F18(PlayState*, Player*);
s16 func_8083DB98(Player*, s32);
void func_80853080(Player*, PlayState*);
void Player_ApplyYawFromAnim(Player*);
void func_8083C0E8(Player*, PlayState*);
void Player_SetPendingFlag(Player*, PlayState*);
s32 Player_ActionHandler_2(Player*, PlayState*);
/* ANIM_SFX_TYPE */
void Player_ProcessAnimSfxList(Player* who, AnimSfxEntry* entries) { (void)who; (void)entries; }
void func_80837C0C(PlayState*, Player*, s32, f32, f32, s16, s32);
void func_808332F4(Player*, PlayState*);
void func_8084DF6C(PlayState*, Player*);
void func_8084DFAC(PlayState*, Player*);
static AnimSfxEntry D_808549E0[] = {{ 0, 0 }};
'''
# Player_SetupAction now owns Din interruption cleanup. Preserve its real
# inactive initial state and ownership helper; this fixture has no spell cast.
engine = engine.replace("/* NATIVE_DINS_STATE */", "\n".join(
    re.search(rf"^static [^\n;]+\b{name}\s*=.*?;", player, re.M)[0]
    for name in ("sDinsFireRemakePendingMagic", "sDinsFireRemakeAudioHandle")))
engine = engine.replace("/* ANIM_SFX_TYPE */", re.search(r"typedef struct AnimSfxEntry \{.*?\} AnimSfxEntry;", player, re.S)[0])
engine += "\n".join(function(message, name) for name in (
    "void Message_FindMessageJPN(", "void Message_FindMessage(", "void Message_FindCreditsMessage(",
    "void Message_OpenText(", "void Message_StartTextbox(", "void Message_ContinueTextbox(",
    "void Message_CloseTextbox(", "void Message_LoadItemIcon(", "void Message_Decode(PlayState*",
    "u8 Message_GetState(", "u8 Message_ShouldAdvance(", "u8 Message_ShouldAdvanceSilent("))
engine += "\n" + "\n".join(function(actor, name) for name in (
    "void Actor_SetFocus(", "void Actor_SetScale(", "u32 Actor_ProcessTalkRequest(",
    "s32 Actor_OfferTalkExchange(", "s32 Actor_OfferTalkExchangeEquiCylinder(", "s32 Actor_OfferTalk(",
    "s32 GiveItemEntryWithoutActor("))
engine += "\n" + "\n".join(function(player, name) for name in (
    "s32 Player_SetupAction(", "void Player_SetupActionPreserveAnimMovement(",
    "s32 Player_SetupWaitForPutAway(", "void Player_Action_WaitForPutAway(",
    "void Player_SetupTalk(", "void Player_StartTalking(", "s32 Player_ActionHandler_Talk(",
    "void Player_Action_Talk(", "void func_80853080(", "void func_8083C0E8(", "void Player_SetPendingFlag(",
    "s32 Player_ActionHandler_2(", "void func_8083A434(", "s32 func_8084DFF4(",
    "void Player_Action_8084E6D4(", "void func_8084DF6C(", "void func_8084DFAC("))
engine += "\n" + function((ROOT / "soh/src/code/z_parameter.c").read_text(), "u8 Return_Item_Entry(")

with tempfile.TemporaryDirectory(prefix="vanilla-oot-pickup-flow-") as directory:
    work = Path(directory)
    (work / "spdlog").mkdir()
    (work / "spdlog/spdlog.h").write_text("#pragma once\n#define SPDLOG_DEBUG(...) ((void)0)\n")
    (work / "soh/Enhancements").mkdir(parents=True)
    (work / "soh/Enhancements/gameconsole.h").write_text(
        '#pragma once\n#include "z64.h"\n#ifdef __cplusplus\nextern "C" {\n#endif\n'
        'extern PlayState* gPlayState;\n#ifdef __cplusplus\n}\n#endif\n')
    cpp, c = work / "flow.cpp", work / "native.c"
    cpp.write_text(fixture)
    c.write_text(engine)
    common = ["-I" + str(work), *[flag for flag in native_flags()
                                 if not flag.startswith("-std=") and flag != "-Wno-incompatible-pointer-types"]]
    common += ["-w", "-g", "-O0", "-ffunction-sections", "-fdata-sections", "-fno-pie",
               "-DCOMBO_BUILD", "-DCONTROLLERBUTTONS_T=uint32_t", "-I" + str(ROOT),
               "-I" + str(ROOT / "combo"), "-I" + str(ROOT / "combo/menu")]
    if args.sanitize:
        common += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"]
    units = []
    for source in (c, ROOT / "soh/src/code/z_kanfont.c",
                   ROOT / "soh/soh/Enhancements/custom-message/CustomMessageManager.cpp",
                   ROOT / "soh/soh/Enhancements/custom-message/text.cpp"):
        obj = work / (source.stem + ".o")
        is_cpp = source.suffix == ".cpp"
        cflags = ["-Werror=implicit-function-declaration", "-Wno-int-conversion",
                  "-Wno-incompatible-pointer-types"]
        if args.sanitize:
            # Native C headers define high-bit flags as signed (1 << 31).
            # Keep address/remaining UB checks, without rewriting those headers.
            cflags += ["-fno-sanitize=shift-base"]
        run(os.environ.get("CXX", "c++") if is_cpp else os.environ.get("CC", "cc"),
            "-std=c++20" if is_cpp else "-std=gnu2x", *common,
            *([] if is_cpp else cflags), "-c", source, "-o", obj)
        units.append(obj)
    binary = work / "flow"
    run(os.environ.get("CXX", "c++"), "-std=c++20", *common, cpp, *units,
        "-Wl,--gc-sections", "-no-pie", "-o", binary)
    run(binary, args.case)
