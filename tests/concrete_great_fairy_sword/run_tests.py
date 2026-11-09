#!/usr/bin/env python3
"""Execute concrete sword production fragments at controlled engine boundaries.

Compiles the unchanged bodies/branches for grants, ConvertItem's BGS resolver,
MM draw selection/legacy priority, and OoT donor selection/native GFS recipe.
Uses the real draw ABI POD declarations. Engine inventory/resource/render calls
are recording boundaries; this is not a full engine ABI/build or runtime test.
No network, archive or save access. Requires a C++20 compiler.
"""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    local = ROOT / path
    native = Path(os.environ.get("COMBOSHIP_NATIVE_SOURCE_ROOT", ROOT)) / path
    return (local if local.is_file() else native).read_text()


def block(text, marker):
    start = text.index(marker)
    opening = text.index("{", start)
    depth = 1
    for token in re.finditer(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/|[{}]',
                             text[opening + 1:]):
        if token.group() == "{":
            depth += 1
        elif token.group() == "}":
            depth -= 1
            if depth == 0:
                return text[start:opening + 1 + token.end()] + "\n"
    raise ValueError("Unclosed production block: " + marker)


def case(text, name, indent=8):
    start = text.index(" " * indent + "case " + name + ":")
    end = re.search(r"^" + " " * indent + r"(?:case |default:)", text[start + 1:], re.M)
    if not end:
        raise ValueError("Missing next case after " + name)
    return text[start:start + 1 + end.start()]


def fixture():
    draw = read("mm/2s2h/Rando/DrawItem.cpp")
    give = read("mm/2s2h/Rando/GiveItem.cpp")
    convert = read("mm/2s2h/Rando/ConvertItem.cpp")
    mm = read("mm/2s2h/Rando/NeiGiPresentation.cpp")
    oot = read("combo/menu/ComboItemDrawOOT.h")
    owner = read("soh/soh/Enhancements/randomizer/NeiGiPresentation.cpp")
    abi = read("combo/menu/ComboItemDrawABI.h")
    # Symbols model engine declaration boundaries, not their binary ABI values.
    ids = sorted(set(re.findall(r"\bRI_[A-Z0-9_]+\b", mm + draw + give + convert)))
    rgs = sorted(set(re.findall(r"\bRG_[A-Z0-9_]+\b", oot)))
    parts = [r'''
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_set>
using u8 = uint8_t;
using RandoCheckId = int;
constexpr int RC_UNKNOWN = 0, FC_OOT_SWORD_BIGGORON = 0;
enum { GID_SWORD_BGS = 11, GID_SWORD_GREAT_FAIRY = 12 };
namespace NeiGi { enum class Kind { Neutral, KokiriSword, RazorSword, GildedSword,
 MasterSword, SwordAura, BiggoronSword, GiantsKnife, GreatFairySword, Pokeball, Pacci, Last };
bool IsSword(Kind k) { return k >= Kind::KokiriSword && k <= Kind::GreatFairySword; }
bool IsEquipment(Kind) { return false; }
}
using NeiGi::Kind;
struct Color_RGB8 { uint8_t r,g,b; };
#define CVAR_ENHANCEMENT(name) name
bool altAssets = true, dinSword = false, authoredAvailable = true;
bool resourcesAvailable = true, ownerFallbackCalled = false;
std::unordered_set<std::string> modRoots;
int OOT_NeiAltAssetsEnabled() { return altAssets; }
int OOT_NeiResourceExists(const char*) { return resourcesAvailable; }
bool ResourceMgr_IsAltAssetsEnabled() { return altAssets; }
int ResourceMgr_IsModAssetForGame(const char* host, const char* path) {
 if (std::string(host) != "mm") std::abort();
 return modRoots.count(path);
}
int ResourceMgr_IsIkanaShieldGiAvailable() { return 0; }
int CVarGetInteger(const char* name, int fallback) { return std::string(name) == "DinFireSword" ? dinSword : fallback; }
struct NeiSaveData { int comboObtained[1]{}; int weaponUpgrades{}; } save;
NeiSaveData* Nei_Save() { return &save; }
void WeaponUpgrade_SetGreatFairy(u8 on) { if (on) save.weaponUpgrades |= 16; }
void* gPlayState = &save;
int nativeItem = -1, nativeGid = -1, inventoryCalls = 0, bgsDrawCalls = 0;
void Item_Give(void*, int item) { nativeItem = item; ++inventoryCalls; }
void GetItem_Draw(void*, int gid) { nativeGid = gid; }
bool DrawOotBiggoronSwordReal() { ++bgsDrawCalls; return false; }
void DrawOotBiggoronSwordTier() { ++bgsDrawCalls; nativeGid = GID_SWORD_BGS; }
''', "enum RandoItemId {" + ",".join(ids) + "};\n",
             "enum RandomizerGet {" + ",".join(rgs) + "};\n"]
    parts.append(abi[abi.index("#define CW_DRAW_MAX_DLISTS"):abi.index("} CwItemDrawInfo;") + len("} CwItemDrawInfo;")])
    parts.append(r'''
using Fn_GetNeiGiDrawInfo = int32_t (*)(const char*, CwItemDrawInfo*);
using Fn_GetNeiGiDrawInfoForAssets = int32_t (*)(const char*, int32_t, CwItemDrawInfo*);
struct Presentation {
 const char* opaque; const char* translucent; float scale; Kind effect;
 struct { float x,y,z; } effectCenter{};
 bool alwaysShimmer = true; void (*draw)() = nullptr; Kind shimmer = Kind::Neutral;
};
void Randomizer_DrawCaneSomariaUpgrade() {}
#define CVAR_NEI_GI_EFFECTS "test.ItemEffects"
bool HasLegacyGiMod(const Presentation&, bool) { return false; }
bool HasRedesignGiMod(const Presentation&) { return false; }
const char* NeiGi_BaseSwordPath(const char* path) { return path; }
''')
    for name in ("Kind PresentationShimmer(", "const char* SelectedSwordPath(", "bool HasSelectedSword(",
                 "static bool NeiGi_FillCrossGameInfo("):
        parts.append(block(owner, name))
    parts.append(r'''
int DescribeAuthored(const char* slug, int32_t alt, CwItemDrawInfo* out) {
 if (std::string(slug) != "great_fairy_sword" || bool(alt) != altAssets) std::abort();
 if (!authoredAvailable) {
  out->neiShimmer = int(Kind::GreatFairySword) + 1; return 0;
 }
 const Presentation gfs{"__OTR__objects/nei_gi_redesign/great_fairy_sword/gi_dl",
                        "__OTR__objects/nei_gi_redesign/great_fairy_sword/gi_xlu_dl", 1.f, Kind::GreatFairySword};
 return NeiGi_FillCrossGameInfo(gfs, out, alt);
}
void* Combo_ResolveSym(const char*, const char* name) {
 return std::string(name) == "OOT_GetNeiGiDrawInfoForAssets" ? reinterpret_cast<void*>(DescribeAuthored) : nullptr;
}
const char* NeiResource_Route(const char* path) { return path; }
bool GetSelectedOwnerGi(RandoItemId, CwItemDrawInfo*) { ownerFallbackCalled = true; return false; }
''')
    start = mm.index("struct Binding {")
    parts.append(mm[start:mm.index("// Only roots", start)])
    parts.extend(block(mm, name) for name in ("bool HasMmLegacyGiMod(", "bool MM_DescribeNeiGi("))
    parts.append(r'''
bool descriptorDrawn = false;
bool MM_TryDrawNeiGi(RandoItemId item, bool, int) {
 if (item != RI_GREAT_FAIRY_SWORD) return false; // Other model families are outside this fixture.
 CwItemDrawInfo info{}; descriptorDrawn = MM_DescribeNeiGi(item, &info); return descriptorDrawn;
}
Kind observedFit = Kind::Neutral;
struct MM_NeiGiFallbackShimmer {
 MM_NeiGiFallbackShimmer(RandoItemId item, bool, int, Kind fit = Kind::Neutral) {
  observedFit = fit == Kind::Neutral && item == RI_GREAT_FAIRY_SWORD ? Kind::GreatFairySword : fit;
 }
};
enum { ACTOR_EN_GIRLA, ACTOR_EN_ITEM00, PLAYER_FORM_GORON };
namespace CustomItem { enum { CALLED_ACTION=1, GIVE_ITEM_CUTSCENE=2, GIVE_OVERHEAD=4 }; }
int playerForm = 0;
#define GET_PLAYER_FORM playerForm
struct Actor { int id; struct { struct { int x; } rot; } home; };
namespace Rando {
namespace StaticData { struct Item { int itemId; }; Item Items[''' + str(len(ids)) + r''']{}; }
RandoItemId ConvertItem(RandoItemId item, RandoCheckId) {
 switch (item) {
''')
    parts.append(case(convert, "RI_OOT_PROGRESSIVE_BGS", 12))
    parts.append(" default: return item; } }\nvoid DrawResolvedItem(RandoItemId, RandoCheckId, Actor*);\n")
    parts.append("void GiveFragment(RandoItemId randoItemId) { switch (randoItemId) {\n")
    for name in ("RI_GREAT_FAIRY_SWORD", "RI_OOT_BIGGORON_SWORD", "RI_OOT_PROGRESSIVE_BGS"):
        parts.append(case(give, name))
    parts.append("default: std::abort(); } }\n")
    parts.append(block(draw, "void Rando::DrawItem(").replace("void Rando::DrawItem(", "void DrawItem("))
    start = draw.index("    const bool shop = actor && actor->id == ACTOR_EN_GIRLA;")
    # This fixture selects only the authored/legacy sword preamble. Its outer
    # COMBO_BUILD starts before the fragment; retain nested guards verbatim and
    # stop at that block's actual end, before unrelated native dungeon routing.
    depth = 1
    lines = []
    for line in draw[start:].splitlines(keepends=True):
        if re.match(r"\s*#\s*if(?:def|ndef|\s)", line):
            depth += 1
        elif re.match(r"\s*#\s*endif\b", line):
            depth -= 1
            if depth == 0:
                break
        lines.append(line)
    if depth != 0:
        raise ValueError("Missing authored sword COMBO_BUILD preamble boundary")
    preamble = "".join(lines)
    parts.append("void DrawResolvedItem(RandoItemId randoItemId, RandoCheckId, Actor* actor) {\n" + preamble)
    parts.append("switch (randoItemId) {\n")
    for name in ("RI_GREAT_FAIRY_SWORD", "RI_OOT_BIGGORON_SWORD"):
        parts.append(case(draw, name))
    parts.append("default: std::abort(); } }\n} // namespace Rando\n")
    for name in ("static int32_t CwSimple(", "static int32_t CwCustomGi(",
                 "static int32_t CwNativeMmSword(", "static int32_t CwAltSwordGi("):
        parts.append(block(oot, name))
    parts.append("int OOT_DescribeCustomDraw(RandomizerGet rg, CwItemDrawInfo* out) { switch (rg) {\n")
    parts.append(case(block(oot, "static int32_t OOT_DescribeCustomDraw("), "RG_GREAT_FAIRY_SWORD"))
    parts.append("default: return 0; } }\nstruct Entry {};\n")
    parts.append(r'''
int NeiGi_DescribeEntry(Entry*, CwItemDrawInfo* out) {
 if (!authoredAvailable) return 0;
 return DescribeAuthored("great_fairy_sword", altAssets, out);
}
int DonorFragment(RandomizerGet effRg, CwItemDrawInfo* out) { Entry gi{};
''')
    start = oot.index("    if (CwAltSwordGi(effRg, out))")
    parts.append(oot[start:oot.index("    // Concrete skills", start)])
    parts.append("return 0; }\n")
    parts.append(r'''
int failures = 0;
void check(bool value, const char* name) {
 std::cout << (value ? "PASS " : "FAIL ") << name << '\n'; failures += !value;
}
void reset() {
 save = {}; nativeItem = nativeGid = -1; inventoryCalls = bgsDrawCalls = 0;
 observedFit = Kind::Neutral; descriptorDrawn = ownerFallbackCalled = false;
 modRoots.clear(); altAssets = resourcesAvailable = authoredAvailable = true; dinSword = false;
 Rando::StaticData::Items[RI_GREAT_FAIRY_SWORD].itemId = 88;
}
int main() {
 for (int prior : {0,1}) {
  reset(); save.comboObtained[0] = prior;
  Rando::GiveFragment(RI_GREAT_FAIRY_SWORD);
  check(save.comboObtained[0] && (save.weaponUpgrades & 16) && nativeItem == 88 && inventoryCalls == 1,
        prior ? "fixed GFS grant with BGS" : "fixed GFS grant without BGS (Black Market bypass)");
  Rando::GiveFragment(RI_GREAT_FAIRY_SWORD);
  check(save.comboObtained[0] && (save.weaponUpgrades & 16) && nativeItem == 88 && inventoryCalls == 2,
        "repeated concrete GFS remains native GFS");
  reset(); save.comboObtained[0] = prior;
  authoredAvailable = false;
  Rando::DrawItem(RI_GREAT_FAIRY_SWORD, RC_UNKNOWN, nullptr);
  check(nativeGid == GID_SWORD_GREAT_FAIRY && bgsDrawCalls == 0,
        prior ? "MM concrete GFS native model with BGS" : "MM concrete GFS native model without BGS");
  check(observedFit == Kind::GreatFairySword, "MM concrete GFS legacy fit matches its model");
 }
 reset();
 check(Rando::ConvertItem(RI_OOT_PROGRESSIVE_BGS,0) == RI_OOT_BIGGORON_SWORD, "progressive first tier resolves BGS");
 Rando::GiveFragment(Rando::ConvertItem(RI_OOT_PROGRESSIVE_BGS,0));
 check(save.comboObtained[0] && !(save.weaponUpgrades & 16) && inventoryCalls == 0, "resolved BGS grant stays base only");
 check(Rando::ConvertItem(RI_OOT_PROGRESSIVE_BGS,0) == RI_GREAT_FAIRY_SWORD, "progressive second tier resolves GFS");
 Rando::GiveFragment(Rando::ConvertItem(RI_OOT_PROGRESSIVE_BGS,0));
 check((save.weaponUpgrades & 16) && nativeItem == 88, "resolved progressive upgrade grants native GFS");
 reset(); Rando::GiveFragment(RI_OOT_PROGRESSIVE_BGS);
 check(save.comboObtained[0] && !save.weaponUpgrades && !inventoryCalls, "raw progressive grant keeps first base tier");
 Rando::GiveFragment(RI_OOT_PROGRESSIVE_BGS);
 check(save.weaponUpgrades & 16, "raw progressive grant keeps second upgrade tier");
 reset(); authoredAvailable = false;
 Rando::DrawItem(RI_OOT_PROGRESSIVE_BGS,0,nullptr);
 check(nativeGid == GID_SWORD_BGS && bgsDrawCalls == 1, "progressive preview first tier draws BGS");
 save.comboObtained[0] = 1; nativeGid = -1; bgsDrawCalls = 0;
 Rando::DrawItem(RI_OOT_PROGRESSIVE_BGS,0,nullptr);
 check(nativeGid == GID_SWORD_GREAT_FAIRY && !bgsDrawCalls, "progressive preview second tier draws native GFS");
 reset(); save.weaponUpgrades = 16; Rando::GiveFragment(RI_OOT_BIGGORON_SWORD);
 check(save.comboObtained[0] && save.weaponUpgrades == 16 && !inventoryCalls, "direct BGS preserves existing upgrade");
 for (bool din : {false,true}) {
  reset(); dinSword = din; CwItemDrawInfo info{};
  check(DonorFragment(RG_GREAT_FAIRY_SWORD,&info) == 1 && info.dlists[0] &&
        std::string(info.dlists[0]) == "__OTR__objects/nei_gi_redesign/great_fairy_sword/gi_dl",
        "OoT GFS reaches authored descriptor ahead of custom longsword/Din BGS");
  authoredAvailable = false; info = {};
  check(DonorFragment(RG_GREAT_FAIRY_SWORD,&info) == 1 && info.dlistCount == 2 && info.xluStartIndex == 1 &&
        info.scale == 0.f && std::string(info.dlists[0]) == "__OTR__@mm:objects/object_gi_sword_4/gGiGreatFairysSwordBladeDL" &&
        std::string(info.dlists[1]) == "__OTR__@mm:objects/object_gi_sword_4/gGiGreatFairysSwordHiltEmblemDL",
        "OoT GFS fallback retains MM blade/emblem owner and OPA/XLU recipe");
  info = {}; check(CwAltSwordGi(RG_BIGGORON_SWORD,&info) && info.dlists[0] && std::string(info.dlists[0]) ==
        (din ? "__OTR__objects/din_fire_sword/progressive/bgs/SwordDL" : "__OTR__alt/objects/object_custom_equip/gCustomLongswordDL"),
        "BGS custom longsword and Din selections preserved");
 }
 for (const char* root : {"objects/object_gi_sword_4/gGiGreatFairysSwordBladeDL",
                          "objects/object_gi_sword_4/gGiGreatFairysSwordHiltEmblemDL"}) {
  reset(); modRoots.insert(root); Rando::DrawItem(RI_GREAT_FAIRY_SWORD,0,nullptr);
  check(!descriptorDrawn && !ownerFallbackCalled && nativeGid == GID_SWORD_GREAT_FAIRY,
        "MM native GFS replacement retains priority (blade or emblem override)");
 }
 reset(); Rando::DrawItem(RI_GREAT_FAIRY_SWORD,0,nullptr);
 check(descriptorDrawn && nativeGid == -1, "MM authored GFS takes priority without legacy replacement");
 reset();
 modRoots.insert("objects/object_gi_sword_4/gGiGreatFairysSwordBladeDL");
 modRoots.insert("objects/object_gi_sword_4/gGiGreatFairysSwordHiltEmblemDL");
 Rando::DrawItem(RI_GREAT_FAIRY_SWORD,0,nullptr);
 check(!descriptorDrawn && nativeGid == GID_SWORD_GREAT_FAIRY && observedFit == Kind::GreatFairySword,
       "complete MM GFS replacement pair retains native callback and GFS fit");
 reset(); resourcesAvailable = false; CwItemDrawInfo info{};
 check(!CwAltSwordGi(RG_BIGGORON_SWORD,&info), "missing custom BGS resource declines safely");
 altAssets = false;
 check(!CwAltSwordGi(RG_GREAT_FAIRY_SWORD,&info), "Alt off GFS bypasses custom longsword");
 std::cout << "Concrete GFS native fragment failures: " << failures << '\n';
 return failures ? 1 : 0;
}
''')
    return "\n".join(parts)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    flags = ["-std=c++20", "-Wall", "-Wextra", "-Wno-unused-function", "-Wno-unused-parameter"]
    if args.sanitize:
        flags += ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                  "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    with tempfile.TemporaryDirectory(prefix="concrete-gfs-") as temporary:
        source = Path(temporary) / "test.cpp"
        source.write_text(fixture())
        binary = Path(temporary) / "test"
        subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), *flags, str(source), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
