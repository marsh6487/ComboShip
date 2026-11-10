#!/usr/bin/env python3
"""Run actual MM local/cross route adapters against native save/scene types."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def block(text, marker):
    start = text.index(marker)
    opening = text.index("{", start)
    depth, end = 1, opening + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


with tempfile.TemporaryDirectory(prefix="solo-mm-runtime-") as directory:
    temporary = Path(directory)
    logic = (ROOT / "mm/2s2h/Rando/Logic/Logic.cpp").read_text()
    native = "namespace Rando::Logic { std::map<RandoRegionId,RandoRegion> Regions;\n"
    native += block(logic, "RandoRegionId GetRegionIdFromEntrance") + "\n}\nvoid InitNativeRegions() {\n"
    for path in sorted((ROOT / "mm/2s2h/Rando/Logic/Regions").glob("*.cpp")):
        text = path.read_text()
        for match in re.finditer(r"Regions\[(RR_\w+)\]\s*=\s*RandoRegion\s*\{", text):
            region = block(text, match.group(0))
            prefix = "Rando::Logic::Regions[" + match.group(1) + "]"
            scene = re.search(r"\.sceneId\s*=\s*(SCENE_\w+)", region)
            if scene:
                native += prefix + ".sceneId=" + scene[1] + ";\n"
            exits = re.search(r"\.exits\s*=\s*\{(.*?)\n\s*\},", region, re.S)
            if exits:
                for target, back in re.findall(
                        r"EXIT\(\s*(ENTRANCE\(\w+,\s*\d+\)),\s*(ENTRANCE\(\w+,\s*\d+\)|ONE_WAY_EXIT)",
                        exits.group(1)):
                    native += prefix + ".exits[" + target + "]={" + back + ",[]{return true;},\"\"};\n"
            one_way = re.search(r"\.oneWayEntrances\s*=\s*\{(.*?)\}", region, re.S)
            if one_way:
                for entrance in re.findall(r"ENTRANCE\(\w+,\s*\d+\)", one_way.group(1)):
                    native += prefix + ".oneWayEntrances.insert(" + entrance + ");\n"
    native += "}\n"
    (temporary / "native_regions.inc").write_text(native)
    bridge = (ROOT / "mm/2s2h/GameInteractor/GameInteractor.cpp").read_text()
    lifecycle = "\n".join(block(bridge, "void " + name + "(") for name in (
        "GameInteractor_ExecuteOnSaveLoad", "GameInteractor_ExecuteOnSceneInit",
        "GameInteractor_ExecuteOnPlayDestroy", "GameInteractor_ExecuteBeforeEndOfCycleSave",
        "GameInteractor_ExecuteBeforeMoonCrash"))
    (temporary / "native_save_load.inc").write_text(lifecycle)
    benport = (ROOT / "mm/2s2h/BenPort.cpp").read_text()
    hooks = block(benport, "if (gComboGameMode == COMBO_GAME_MM) {\n        GameInteractor::Instance->RegisterGameHook")
    (temporary / "native_hooks.inc").write_text("void RegisterNativeHooks() {\n" + hooks + "\n}")
    spells = (ROOT / "mm/mods/items/logic/item_oot_spells.c").read_text()
    capture_start = spells.index("NeiSaveData* nei = Nei_Save();", spells.index("// warp-point-save countdown"))
    capture_end = spells.index("ComboFw_PublishPoint();", capture_start) + len("ComboFw_PublishPoint();")
    recall_start = spells.index("RespawnData* top = &gSaveContext.respawn[RESPAWN_MODE_TOP];",
                                spells.index("s32 result = ComboFw_RequestReturn();"))
    recall_end = spells.index("play->transitionType = TRANS_TYPE_FADE_WHITE;", recall_start) + len(
        "play->transitionType = TRANS_TYPE_FADE_WHITE;")
    physical_fw = "static bool sOotFwReturnPending;\nvoid NativeCapturePoint() {\n" + spells[capture_start:capture_end]
    physical_fw += "\n}\nvoid NativeRecallPoint(PlayState* play) {\nNeiSaveData* nei = Nei_Save();\n"
    physical_fw += spells[recall_start:recall_end] + "\n}\n"
    physical_fw += block(spells, "static s32 OotSpells_SceneAllowsFarores(")
    (temporary / "native_fw.inc").write_text(physical_fw)
    giants = (ROOT / "mm/2s2h/Enhancements/Cutscenes/StoryCutscenes/SkipGiantsChamber.cpp").read_text()
    (temporary / "native_giants.inc").write_text(block(giants, 'extern "C" int Combo_MM_GiantsEscape('))
    ranch = (ROOT / "mm/src/overlays/actors/ovl_En_Ma4/z_en_ma4.c").read_text()
    reloads = "struct EnMa4;\n" + re.sub(r"\bthis\b", "self", block(
        ranch, "void EnMa4_BeginHorsebackGame(EnMa4* this, PlayState* play) {"))
    curse = (ROOT / "mm/2s2h/Enhancements/Cutscenes/StoryCutscenes/SkipIkanaCurseCutscenes.cpp").read_text()
    reloads += "\n" + block(curse, "void skipHealingPamelasFather() {") + "\n"
    curse = curse[curse.index("// Healing Sharp's curse"):]
    event = block(curse, "GIEventTransition{")
    reloads += "\nGIEventTransition NativeCurseReload() { return " + event + "; }\n"
    tower = (ROOT / "mm/src/overlays/actors/ovl_Obj_Wturn/z_obj_wturn.c").read_text()
    inversion = block(tower, "if (this->unk_14A == 1) {")
    reloads += "void NativeTowerInversion(PlayState* play) " + inversion[inversion.index("{"):] + "\n"
    forest = (ROOT / "mm/2s2h/Enhancements/Gameplay/ComboSoloForest.cpp").read_text()
    reloads += block(forest, "void ForestTransition(PlayState* play, int entrance) {")
    (temporary / "native_reloads.inc").write_text(reloads)
    flags = ["-std=c++20", "-O0", "-DCOMBO_BUILD", "-DF3DEX_GBI_2", "-DCONTROLLERBUTTONS_T=uint32_t",
             "-DGBI_FLOAT_MTX", "-DGBI_S32_VTX", "-DFMT_HEADER_ONLY", "-I" + str(ROOT),
             "-I" + directory, "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
             "-include", str(ROOT / "tests/shuffle_parity/souls/headless.h")]
    for path in ("mm", "mm/2s2h", "mm/include", "mm/include/PR", "mm/src", "mm/assets",
                 "libultraship/include", "combo"):
        flags += ["-isystem", str(ROOT / path)]
    sources = [os.environ.get("COMBO_SOLO_MM_RUNTIME_TEST_SOURCE", str(Path(__file__).with_suffix(".cpp")))]
    core = ROOT / "mm/2s2h/Rando/SoloEntrances.cpp"
    if core.exists():
        sources.append(str(core))
    binary = temporary / "runtime"
    subprocess.run([os.environ.get("CXX", "c++"), *flags, *sources, "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
