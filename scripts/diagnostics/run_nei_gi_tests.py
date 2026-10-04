"""Compile the GI policy and production renderer against this checkout's real headers."""
import os
import json
import math
import hashlib
from pathlib import Path
import re
import subprocess
import tempfile
import sys
import struct
import xml.etree.ElementTree as ET
from run_time_pedestal_tests import functions

ROOT = Path(__file__).resolve().parents[2]
flags = ["-std=c++20", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0"]
flags += ["-I" + str(ROOT / p) for p in
          ("soh", "soh/include", "soh/src", "soh/assets", "soh/mods", "libultraship/include", "combo/menu")]
for config in ("CMake/soh-cvars.cmake", "CMake/lus-cvars.cmake"):
    for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / config).read_text()):
        flags.append(f'-D{key}="{value}"')
cc = os.environ.get("CXX", "c++")
with tempfile.TemporaryDirectory(prefix="nei-gi-tests-") as tmp:
    draw = functions((ROOT / "soh/src/code/z_draw.c").read_text())
    player = functions((ROOT / "soh/src/code/z_player_lib.c").read_text())
    shop = functions((ROOT / "soh/src/overlays/actors/ovl_En_GirlA/z_en_girla.c").read_text())
    custom = functions((ROOT / "soh/soh/Enhancements/randomizer/draw.cpp").read_text())
    (Path(tmp) / "nei_gi_dispatch.inc").write_text(draw["GetItemEntry_Draw"] + "\n" +
        re.sub(r"\bthis\b", "player", player["Player_DrawGetItemImpl"]) + "\n" +
        re.sub(r"\bthis\b", "shop", shop["EnGirlA_Draw"]) + "\n" +
        custom["Randomizer_DrawCaneSomariaUpgradeFlame"])
    fixtures = []
    bindings = (("ball_and_chain", "BallAndChain"), ("shovel", "Shovel"),
                           ("fire_rod", "FireRod"), ("ice_rod", "IceRod"), ("light_rod", "LightRod"),
                           ("hylia_grace", "HyliaGrace"), ("zonai_permafrost", "ZonaiPermafrost"),
                           ("demise_destruction", "DemiseDestruction"), ("time_gate", "TimeGate"),
                           ("switch_hook", "SwitchHook"), ("rocs_feather", "RocsFeatherSkijer"),
                           ("rocs_feather", "RocsFeather"), ("spinner", "Spinner"),
                           ("cane_of_somaria", "CaneOfSomaria"), ("cane_of_somaria", "CaneSomariaUpgrade"),
                           ("minish_cap", "MinishCap"), ("rocs_cape", "RocsCape"))
    bindings += (
        ("divine_shield", "ExtDivineShield"), ("sheikah_shield", "ExtSheikahShield"),
        ("shield_of_ikana", "ExtShieldOfIkana"), ("magic_cape", "ExtMagicCape"),
        ("spirit_breastplate", "ExtSpiritBreastplate"), ("sages_tunic", "ExtSagesTunic"),
        ("champions_tunic", "ExtChampionsTunic"), ("pegasus_anklet", "ExtPegasusAnklet"),
        ("trident", "ExtTrident"), ("climb_boots", "ExtClimbBoots"), ("roc_boots", "ExtRocBoots"),
        ("cane_of_byrna", "ExtCaneOfByrna"), ("four_sword", "ExtFourSword"),
        ("pendant_of_memories", "ExtPendantOfMemories"), ("elemental_wand", "ElementalWand"),
        ("sand_rod", "ElementalWand", "RG_WAND_SAND_ROD"),
        ("tornado_rod", "ElementalWand", "RG_WAND_TORNADO_ROD"),
        ("water_rod", "ElementalWand", "RG_WAND_WATER_ROD"),
        ("meteor_rod", "ElementalWand", "RG_WAND_METEOR_ROD"),
        ("storm_rod", "ElementalWand", "RG_WAND_STORM_ROD"),
        ("shadow_scepter", "ElementalWand", "RG_WAND_SHADOW_SCEPTER"),
        ("sheikah_slate", "NeiSheikahSlate"), ("slate_bomb", "SlateRuneBomb"),
        ("slate_master_cycle", "SlateRuneMasterCycle"), ("slate_stasis", "SlateRuneStasis"),
        ("slate_cryonis", "SlateRuneCryonis"), ("slate_sensor", "SlateRuneSensor"),
        ("phantom_hourglass", "NeiPhantomHourglass"), ("shadow_crystal", "NeiShadowCrystal"),
        ("rod_of_seasons", "NeiRodOfSeasons"), ("kokiri_sword", "ProgressiveKokiriSword"),
        ("razor_sword", "RazorSword"), ("gilded_sword", "GildedSword"),
        ("master_sword", "MasterSword"), ("true_master_sword", "TrueMasterSword"),
        ("biggoron_sword", "ProgressiveBGS"), ("great_fairy_sword", "GreatFairySword"),
        ("iron_knuckle_axe", "IronKnuckleAxe"))
    for binding in bindings:
        slug, callback = binding[:2]
        identity = binding[2] if len(binding) > 2 else "0"
        root = ROOT / "soh/assets/custom/objects/nei_gi_redesign" / slug
        # Include the translucent shell: it is lower than the opaque spell core.
        vertices = [tuple(int(v.get(axis)) for axis in ("X", "Y", "Z"))
                    for path in root.glob("mesh_*_vtx") for v in ET.parse(path).getroot()]
        words = struct.unpack_from("<16I", (root / "scale_mtx").read_bytes(), 64)
        scale = ((words[0] >> 16) * 65536 + (words[8] >> 16)) / 65536
        radius = max(math.hypot(p[0], p[2]) for p in vertices) * scale
        meta = json.loads((ROOT / "tools/nei_gi/CHECKPOINTS" / slug / "checkpoint.json").read_text())
        low = ", ".join(f"{min(p[axis] for p in vertices)*scale}f" for axis in range(3))
        high = ", ".join(f"{max(p[axis] for p in vertices)*scale}f" for axis in range(3))
        fixtures.append(f'{{Randomizer_Draw{callback}, "{slug}", {json.dumps(meta["name"])}, "{callback}", '
                        f'{{{low}}}, {{{high}}}, {2*radius}f, {float(meta["draw_scale"])}f, '
                        f'{str((root / "gi_xlu_dl").exists()).lower()}, {identity}}},')
    (Path(tmp) / "nei_gi_bounds.inc").write_text("\n".join(fixtures))
    # Every bundled model is checked, including the eight formerly omitted
    # catalog entries and both real-engine owners of the Kokiri GI.
    all_frames = []
    for asset in sorted((ROOT / "soh/assets/custom/objects/nei_gi_redesign").iterdir()):
        if not asset.is_dir():
            continue
        vertices = [tuple(int(v.get(axis)) for axis in ("X", "Y", "Z"))
                    for path in asset.glob("mesh_*_vtx") for v in ET.parse(path).getroot()]
        words = struct.unpack_from("<16I", (asset / "scale_mtx").read_bytes(), 64)
        scale = ((words[0] >> 16) * 65536 + (words[8] >> 16)) / 65536
        meta = json.loads((ROOT / "tools/nei_gi/CHECKPOINTS" / asset.name / "checkpoint.json").read_text())
        low = min(p[1] for p in vertices)*scale
        high = max(p[1] for p in vertices)*scale
        width = 2*max(math.hypot(p[0],p[2]) for p in vertices)*scale
        all_frames.append(f'{{"{asset.name}",{low}f,{high}f,{width}f,{float(meta["draw_scale"])}f,'
                          f'{str((asset/"gi_xlu_dl").exists()).lower()}}},')
    assert len(all_frames) == 61
    (Path(tmp) / "nei_all_frame_bounds.inc").write_text("\n".join(all_frames))
    names = ["nei_gi/effect_policy", "nei_gi/presentation"]
    if "--held" in sys.argv:
        names.append("nei_held/presentation")
        names.extend(("nei_gi/lantern_policy", "nei_gi/lantern_presentation"))
    for name in names:
        source = ROOT / "tests" / (name + "_test.cpp")
        out = str(Path(tmp) / name.replace("/", "_"))
        extra_flags = []
        if name == "nei_gi/presentation" and "--combo" in sys.argv:
            mm_source = (ROOT / "mm/2s2h/Rando/NeiGiPresentation.cpp").read_text()
            mm_functions = functions(mm_source)
            renderer = functions((ROOT / "mm/2s2h/Rando/DrawItem.cpp").read_text())["DrawSong"] + "\n" + mm_functions["HasMmLegacyGiMod"] + "\n" + mm_functions["MM_DrawNeiGi"] + "\n" + mm_functions["MM_DescribeNeiGi"] + "\n" + mm_functions["MM_TryDrawNeiGi"]
            # MM's item enum and binding table are copied verbatim so origin
            # selection is exercised without importing the unrelated MM engine.
            mm_types = (ROOT / "mm/2s2h/Rando/Types.h").read_text()
            item_enum = re.search(r"typedef enum \{\s*RI_UNKNOWN,.*?\} RandoItemId;", mm_types, re.S)[0]
            bindings = re.search(r"struct Binding \{.*?const Binding kBindings\[\] = \{.*?\n\};", mm_source, re.S)[0]
            fallback = mm_source[mm_source.index("MM_NeiGiFallbackShimmer::MM_NeiGiFallbackShimmer"):]
            fallback_class = (ROOT / "mm/2s2h/Rando/NeiGiPresentation.h").read_text()
            fallback_class = fallback_class[fallback_class.index("class MM_NeiGiFallbackShimmer"):]
            foreign_source = (ROOT / "combo/menu/ComboForeignDrawOOT.h").read_text()
            foreign_info = re.search(r"struct ComboForeignDrawInfo \{.*?\n\};", foreign_source, re.S)[0]
            foreign_draw = functions(foreign_source)["OOT_DrawComboForeign"]
            foreign_shop = functions(foreign_source.replace('extern "C" ', ''))["OOT_DrawComboForeignShop"]
            foreign_wrapper = functions((ROOT / "soh/soh/Enhancements/randomizer/draw.cpp").read_text()
                                        .replace('extern "C" ', ''))["Randomizer_DrawComboForeign"]
            routing_source = (ROOT / "mm/2s2h/Rando/NeiResourceRouting.cpp").read_text()
            route = routing_source[routing_source.index('extern "C" const char* NeiResource_Route'):]
            shim = """
#include <algorithm>
#include <unordered_set>
#include "ComboMaskShimmer.h"
extern "C" { PlayState* gPlayState = &Fixture::play; }
bool ownerAlt = false;
extern "C" int32_t OOT_NeiAltAssetsEnabled(void) { return ownerAlt; }
extern "C" int32_t OOT_NeiResourceExists(const char* path) {
    return path && (ResourceMgr_FileExists(path) || (ownerAlt && ResourceMgr_FileAltExists(path)));
}
void* Combo_ResolveSym(const char* owner, const char* name) {
    assert(std::strcmp(owner, "soh") == 0);
    if (std::strcmp(name, "OOT_GetNeiGiDrawInfo") == 0)
        return reinterpret_cast<void*>(OOT_GetNeiGiDrawInfo);
    if (std::strcmp(name, "OOT_NeiResourceExists") == 0)
        return reinterpret_cast<void*>(OOT_NeiResourceExists);
    assert(false && "unexpected bridge symbol");
    return nullptr;
}
void DrawOotSlateRuneFlame(u8 r, u8 g, u8 b) { Fixture::flameColors.push_back({r,g,b}); }
void MM_DrawNeiGi(const CwItemDrawInfo&,bool shop=false);
#define Gfx_SetupDL25_Opa Gfx_SetupDL_25Opa
#define Gfx_SetupDL25_Xlu Gfx_SetupDL_25Xlu
#define Matrix_RotateYF Matrix_RotateY
#define MATRIX_FINALIZE_AND_LOAD(pkt, gfx) gSPMatrix(pkt, Matrix_NewMtx(gfx, (char*)__FILE__, __LINE__), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH)
"""
            foreign_shim = """
const ComboForeignDrawInfo* selectedForeignInfo = nullptr;
int foreignFallbackCalls = 0;
RandomizerCheck OOT_GetQueuedDrawCheck() { return RC_UNKNOWN_CHECK; }
const ComboForeignDrawInfo* ComboResolveForeignDrawInfo(RandomizerCheck) { return selectedForeignInfo; }
bool ComboForeignAnim_Draw(const CwItemAnimDrawInfo*, const char*, PlayState*) { return false; }
#define FOREIGN_DRAW_STUB(name) \\
    void name(PlayState*, const ComboForeignDrawInfo*) { ++foreignFallbackCalls; Matrix_Scale(7,7,7,MTXMODE_APPLY); }
FOREIGN_DRAW_STUB(OOT_DrawForeignGoronSword)
FOREIGN_DRAW_STUB(OOT_DrawForeignMasterSword)
FOREIGN_DRAW_STUB(OOT_DrawForeignCustomGi)
FOREIGN_DRAW_STUB(OOT_DrawForeignDekuNuts)
FOREIGN_DRAW_STUB(OOT_DrawForeignRecoveryHeart)
FOREIGN_DRAW_STUB(OOT_DrawForeignFish)
FOREIGN_DRAW_STUB(OOT_DrawForeignPotion)
FOREIGN_DRAW_STUB(OOT_DrawForeignBlueFire)
FOREIGN_DRAW_STUB(OOT_DrawForeignPoes)
FOREIGN_DRAW_STUB(OOT_DrawForeignFairyBottle)
bool OOT_DrawForeignFairyContainer(PlayState*, const ComboForeignDrawInfo*) { ++foreignFallbackCalls; return true; }
FOREIGN_DRAW_STUB(OOT_DrawForeignSoulFlame)
FOREIGN_DRAW_STUB(OOT_DrawForeignOps)
FOREIGN_DRAW_STUB(OOT_DrawForeignSimple)
#undef FOREIGN_DRAW_STUB
void ComboDrawSpinAttackGi(PlayState*, const char*, const char*, float, const uint8_t[4], const char*) {
    ++foreignFallbackCalls;
}
"""
            tested_bridge = (shim + route + "\n" + item_enum + "\n#include \"ComboSongDrawMM.h\"\n" + bindings + "\n" + fallback_class + "\n" +
                             renderer + "\n" + fallback + "\n" + foreign_info + "\n" +
                             foreign_shim + foreign_draw + "\n" + foreign_wrapper + "\n" + foreign_shop)
            candidate = source.read_text().replace("int main() {", tested_bridge + "\nint main() {", 1)
            checks = (ROOT / "tests/mm_presentation/gi_bridge_checks.inc").read_text()
            checks += (ROOT / "tests/mm_presentation/foreign_sword_checks.inc").read_text()
            checks += (ROOT / "tests/mm_presentation/legacy_mod_checks.inc").read_text()
            candidate = candidate.replace("  using namespace Fixture;\n", "  using namespace Fixture;\n" + checks, 1)
            source = Path(tmp) / "combo_gi_presentation.cpp"
            source.write_text("#define COMBO_BUILD 1\n" + candidate)
            extra_flags = ["-I" + str(ROOT), "-I" + str(ROOT / "combo"), "-I" + str(ROOT / "combo/menu")]
        subprocess.run([cc, *flags, *extra_flags, "-I" + tmp, str(source), "-o", out], check=True)
        subprocess.run([out], check=True)

# Check the actual C dispatch boundary, using the same CVar definitions as CMake.
cflags = ["-std=gnu2x", "-fsyntax-only", "-Werror=implicit-function-declaration",
          "-Wno-incompatible-pointer-types", "-Wno-int-conversion", "-Wno-pointer-to-int-cast"]
cflags += flags[1:]
sources = ["soh/src/code/z_draw.c", "soh/src/code/z_player_lib.c",
           "soh/src/overlays/actors/ovl_En_GirlA/z_en_girla.c"]
if "--held" in sys.argv:
    sources += ["soh/src/overlays/actors/ovl_player_actor/z_player.c", "soh/src/overlays/actors/ovl_Arms_Hook/z_arms_hook.c"]
    for source in ("NeiHeldPresentation.cpp", "NeiLanternPresentation.cpp"):
        subprocess.run([cc, *flags, "-fsyntax-only", str(ROOT / "soh/soh/Enhancements/randomizer" / source)], check=True)
for source in sources:
    subprocess.run([os.environ.get("CC", "cc"), *cflags, str(ROOT / source)], check=True)
print("PASS: real-header common, overhead and shop GI C translation units")
if "--held" in sys.argv:
    print("PASS: held C++ renderers, custom-items unity and hook actor C translation units")

if preview_path := os.environ.get("NEI_SHOP_PREVIEW_EXPORT"):
    path = Path(preview_path)
    preview = json.loads(path.read_text())
    sources = ("soh/soh/Enhancements/randomizer/NeiGiPresentation.cpp",
               "soh/soh/Enhancements/randomizer/NeiGiRender.h",
               "soh/soh/Enhancements/randomizer/NeiGiShopFit.h",
               "soh/soh/Enhancements/randomizer/draw.cpp",
               "soh/src/overlays/actors/ovl_En_GirlA/z_en_girla.c",
               "tests/nei_gi/presentation_test.cpp",
               "scripts/diagnostics/run_nei_gi_tests.py")
    preview["metadata"]["source_sha256"] = {
        source: hashlib.sha256((ROOT / source).read_bytes()).hexdigest() for source in sources
    }
    for item in preview["items"]:
        asset = ROOT / "soh/assets/custom/objects/nei_gi_redesign" / item["slug"]
        item["asset_sha256"] = {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                                for p in sorted(asset.iterdir()) if p.is_file()}
        checkpoint = ROOT / "tools/nei_gi/CHECKPOINTS" / item["slug"]
        item["checkpoint_sha256"] = {
            name: hashlib.sha256((checkpoint / name).read_bytes()).hexdigest()
            for name in ("checkpoint.json", item["slug"] + ".glb")
        }
    path.write_text(json.dumps(preview, indent=2) + "\n")
    print(f"PASS: production shop poses exported to {path}")
