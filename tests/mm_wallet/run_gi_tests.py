"""Exercise MM's six native wallet draw routes and foreign recipe gates."""
from pathlib import Path
import os
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
native = (ROOT / "mm/2s2h/Rando/DrawItem.cpp").read_text()
donor = (ROOT / "combo/menu/ComboItemDrawMM.h").read_text()
receiver = (ROOT / "combo/menu/ComboForeignDrawMM.h").read_text()

wallet_draw = re.search(r"static void DrawWalletItem\([^\n]+\n.*?^}", native, re.M | re.S)[0]
wallet_model = re.search(r"static void MM_DrawZfgWalletModel\([^\n]+\n.*?^}", native, re.M | re.S)[0]
wallet_cases = re.search(
    r"(?:        case RI_(?:WALLET_[A-Z]+|INFINITE_WALLET):\n)+"
    r"            DrawWalletItem\(randoItemId\);\n            break;",
    native,
)[0]
describe_model = re.search(r"    if \(id == RI_WALLET_ZFG\) \{\s+CwDescribeZfgWallet\([^;]+;.*?^    }",
                           donor, re.M | re.S)[0]
describe_shimmer = re.search(r"static bool MM_FillWalletShimmer\([^\n]+\n.*?^}", donor, re.M | re.S)[0]
animated_gate = receiver[receiver.index("    if (rcStatic == 0 ||"):].split("{", 1)[0]
validation = re.search(r"    if \(\(raw.drawKind == CW_DRAW_KIND_MAGIC_MIRROR[^;]+;", receiver)[0]
foreign_case = re.search(r"        case CW_DRAW_KIND_ZFG_WALLET:\n[^;]+;\n            break;", receiver)[0]

fixture = r'''
#include "global.h"
#include "Rando/WalletGi.h"
#include "ComboItemDrawABI.h"
#include "ComboCollectibleShimmer.h"
#include "tests/test_require.h"
#include <cstdio>
#include <map>

struct WalletRecord { s16 drawId; };
namespace Rando::StaticData {
std::map<RandoItemId, WalletRecord> Items = {
    {RI_WALLET_CHILD, {GID_WALLET_ADULT}}, {RI_WALLET_ADULT, {GID_WALLET_ADULT}},
    {RI_WALLET_GIANT, {GID_WALLET_GIANT}}, {RI_INFINITE_WALLET, {GID_WALLET_GIANT}},
};
}

static PlayState play;
static GraphicsContext graphics;
PlayState* gPlayState = &play;
static int calls;
static int overflowDraws;
static unsigned geometry;
static uint8_t seen[4];
static bool enabled;
static constexpr unsigned kTycoonModel = 1001;
extern "C" int32_t CVarGetInteger(const char* name, int32_t fallback) {
    REQUIRE(!strcmp(name, "gEnhancements.SkijerNEI.ItemEffects") && fallback == 0);
    return enabled;
}
namespace ComboZfgWallet {
static void Draw(PlayState* current, const char* owner) {
    REQUIRE(current == &play && current->state.gfxCtx == &graphics && !strcmp(owner, "mm"));
    REQUIRE(calls++ == 0);
    geometry = GID_WALLET_GIANT;
    ++overflowDraws;
}
}
static void DrawTycoonWallet() {
    REQUIRE(calls++ == 0);
    geometry = kTycoonModel;
}
extern "C" void GetItem_Draw(PlayState* current, s16 drawId) {
    REQUIRE(current == &play);
    REQUIRE(calls++ == 0);
    geometry = drawId;
}
static void NeiGi_DrawTintedShimmerOverlay(PlayState* current, const uint8_t color[4], const char*) {
    REQUIRE(current == &play);
    REQUIRE(calls++ == 1);
    memcpy(seen, color, sizeof(seen));
}

/* NATIVE_MODEL */
/* NATIVE_DRAW */
static void DrawNative(RandoItemId randoItemId) {
    switch (randoItemId) {
/* NATIVE_CASES */
        default: REQUIRE(false);
    }
}

/* DESCRIBE_SHIMMER */
static int Describe(RandoItemId id, CwItemDrawInfo* out) {
/* DESCRIBE_MODEL */
    return MM_FillWalletShimmer(id, out);
}
enum class ComboForeignResolveOOT { Unknown, Ok };
static ComboForeignResolveOOT Validate(const CwItemDrawInfo& raw) {
    const int rcStatic = 1;
/* ANIMATED_GATE */ {
        return ComboForeignResolveOOT::Unknown;
    }
/* RECIPE_VALIDATION */
    return ComboForeignResolveOOT::Ok;
}
static void DrawForeign(int kind) {
    switch (kind) {
/* FOREIGN_CASE */
        default: REQUIRE(false);
    }
}

int main() {
    play.state.gfxCtx = &graphics;
    const RandoItemId items[] = {
        RI_WALLET_CHILD, RI_WALLET_ADULT, RI_WALLET_GIANT, RI_WALLET_TYCOON, RI_WALLET_ZFG, RI_INFINITE_WALLET,
    };
    const unsigned models[] = { GID_WALLET_ADULT, GID_WALLET_ADULT, GID_WALLET_GIANT, kTycoonModel, GID_WALLET_GIANT,
                                GID_WALLET_GIANT };
    static_assert(sizeof(items) / sizeof(items[0]) == CW_WALLET_COUNT);
    for (int tier = 0; tier < CW_WALLET_COUNT; ++tier) {
        uint8_t expected[4];
        REQUIRE(ComboWalletGi_ShimmerColor(tier, expected));
        for (bool effects : {false, true}) {
            enabled = effects;
            calls = overflowDraws = 0;
            DrawNative(items[tier]);
            REQUIRE(calls == (enabled ? 2 : 1) && geometry == models[tier]);
            REQUIRE(overflowDraws == (tier == CW_WALLET_ZFG));
            if (enabled) {
                REQUIRE(!memcmp(seen, expected, sizeof(seen)));
            }

            CwItemDrawInfo exported{};
            REQUIRE(Describe(items[tier], &exported) == 1 && exported.stateDependent == 2);
            REQUIRE(exported.itemShimmer == (enabled ? CW_ITEM_SHIMMER_TINTED : CW_ITEM_SHIMMER_NONE));
            REQUIRE(!memcmp(exported.itemShimmerColor, expected, sizeof(seen)));
            if (tier == CW_WALLET_ZFG) {
                REQUIRE(exported.drawKind == CW_DRAW_KIND_ZFG_WALLET);
                REQUIRE(exported.dlistCount == 0 && exported.opCount == 0);
                REQUIRE(CwMinDlistsForKind(exported.drawKind) == 0);
                REQUIRE(Validate(exported) == ComboForeignResolveOOT::Ok);
                calls = overflowDraws = 0;
                DrawForeign(exported.drawKind);
                REQUIRE(calls == 1 && geometry == GID_WALLET_GIANT && overflowDraws == 1);
                exported.dlistCount = 1;
                REQUIRE(Validate(exported) == ComboForeignResolveOOT::Unknown);
                exported.dlistCount = 0;
                exported.opCount = 1;
                REQUIRE(Validate(exported) == ComboForeignResolveOOT::Unknown);
            }
        }
    }
    puts("PASS MM concrete wallet draw and shimmer palettes, ZFG donor and zero-resource receipt dispatch");
}
'''
for key, value in {
    "NATIVE_MODEL": wallet_model,
    "NATIVE_DRAW": wallet_draw,
    "NATIVE_CASES": wallet_cases,
    "DESCRIBE_MODEL": describe_model,
    "DESCRIBE_SHIMMER": describe_shimmer,
    "ANIMATED_GATE": animated_gate,
    "RECIPE_VALIDATION": validation,
    "FOREIGN_CASE": foreign_case,
}.items():
    fixture = fixture.replace(f"/* {key} */", value)

with tempfile.TemporaryDirectory(prefix="mm-wallet-gi-") as directory:
    build = Path(directory)
    source = build / "wallet_gi.cpp"
    source.write_text(fixture)
    flags = ["-std=c++20", "-DF3DEX_GBI_2", "-DCONTROLLERBUTTONS_T=uint32_t",
             "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie", "-no-pie"]
    for path in ("", "mm", "mm/include", "mm/include/PR", "mm/src", "mm/2s2h",
                 "mm/assets", "combo", "combo/menu", "libultraship/include"):
        flags += ["-I" + str(ROOT / path)]
    cxx = shlex.split(os.environ.get("CXX", "c++"))
    for mode in ([], ["-DCOMBO_BUILD"]):
        exe = build / "run"
        subprocess.run([*cxx, *flags, *mode, str(source), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
