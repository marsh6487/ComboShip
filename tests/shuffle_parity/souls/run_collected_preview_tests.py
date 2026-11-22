#!/usr/bin/env python3
"""Exercise native soul obtainability, slime preview and tracker progress.

Only unrelated conversion families, actor graphics and save I/O are replaced.
The changed production bodies, real item/flag/option IDs and tracker model are
compiled unchanged, once with each game-build guard.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
parser = argparse.ArgumentParser()
parser.add_argument("--baseline", help="Read the old conversion/preview bodies from this Git ref")
args = parser.parse_args()


def source(path):
    if args.baseline:
        return subprocess.check_output(["git", "show", args.baseline + ":" + path], cwd=ROOT, text=True)
    return (ROOT / path).read_text()


def body(text, signature):
    start = text.index(signature)
    begin = text.index("{", start)
    end, depth = begin + 1, 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


conversion = source("mm/2s2h/Rando/ConvertItem.cpp")
prefix = body(conversion, "bool Rando::IsItemObtainable(").split("    if (const auto* key", 1)[0] + "    return true;\n}\n"
enemy = source("mm/2s2h/Rando/ActorBehavior/EnemyDrops.cpp")
slime = body(enemy, "COND_VB_SHOULD(VB_DRAW_SLIME_RANDO_ITEM").split("{", 1)[1].rsplit("}", 1)[0]
drop = (ROOT / "mm/2s2h/Rando/ActorBehavior/EnemyDrops.cpp").read_text()
record = body(drop, "void SpawnDropItem(").split("    CustomItem::Spawn(", 1)[0] + "    (void)position;\n}\n"
tracker = (ROOT / "mm/2s2h/Rando/CheckTracker/SoulTracker.cpp").read_text()
first_slain = body(tracker, "bool FirstSlainEnabled()") + "\n" + body(tracker, "int32_t ReadMmFirstSlain(")

test = r'''
#include "mm/2s2h/Rando/CheckTracker/SoulTrackerModel.h"
#include "mm/2s2h/Rando/EnemyDropProgress.h"
#include <array>
#include <cassert>
#include <cstdarg>
#include <set>
#include <cstdio>
#include <string>
#include <string_view>
using namespace Rando;
using namespace ComboRando::SoulTracker;
struct Check {
    bool obtained = false, cycleObtained = false, eligible = false, shuffled = false, skipped = false;
    RandoItemId randoItemId = RI_UNKNOWN;
};
struct RandoSave { Check randoSaveChecks[RC_MAX]; int randoSaveOptions[RO_MAX]; uint64_t firstSlainEnemies; };
struct { int fileNum = 0; struct { struct { RandoSave rando; } shipSaveInfo; } save; } gSaveContext{};
auto& rando = gSaveContext.save.shipSaveInfo.rando;
auto& checks = rando.randoSaveChecks;
bool isRando = true;
#define IS_RANDO isRando
#define RANDO_SAVE_CHECKS checks
#define RANDO_SAVE_OPTIONS rando.randoSaveOptions
namespace Rando::StaticData {
std::array<std::string, RC_MAX> CheckNames;
}
struct Vec3f {};
__RECORD__
__FIRST_SLAIN__

static void CheckFirstSlainProgress() {
    const auto capture = [](void* context, const char* name, FirstSlainStatus status) {
        assert(std::string_view(name) == "Chuchu");
        static_cast<std::vector<FirstSlainStatus>*>(context)->push_back(status);
    };
    std::vector<FirstSlainStatus> states;
    assert(!ReadMmFirstSlain(0, capture, &states) && states.empty());
    rando.randoSaveOptions[RO_SHUFFLE_ENEMY_DROPS] = 1;
    auto& chu = checks[RC_ENEMY_DROP_CHUCHU];
    chu = {};
    chu.shuffled = true;
    Rando::StaticData::CheckNames[RC_ENEMY_DROP_CHUCHU] = "Enemy Drop Chuchu";
    checks[RC_ENEMY_DROP_WOLFOS].skipped = checks[RC_ENEMY_DROP_WOLFOS].shuffled = true;
    SpawnDropItem({}, RC_ENEMY_DROP_WOLFOS);
    SpawnDropItem({}, RC_ENEMY_DROP_BIO_DEKU_BABA); // Unshuffled.
    assert(rando.firstSlainEnemies == 0);
    assert(!ReadMmFirstSlain(1, capture, &states));
    assert(ReadMmFirstSlain(0, capture, &states));
    SpawnDropItem({}, RC_ENEMY_DROP_CHUCHU);
    assert(rando.firstSlainEnemies == EnemyDropProgress::DefeatedBit(RC_ENEMY_DROP_CHUCHU));
    assert(!chu.eligible && !chu.obtained && !chu.cycleObtained);
    assert(ReadMmFirstSlain(0, capture, &states));
    chu.obtained = true;
    chu.cycleObtained = false;
    assert(ReadMmFirstSlain(0, capture, &states));
    assert((states == std::vector{ FirstSlainStatus::Unrecorded, FirstSlainStatus::Defeated,
                                   FirstSlainStatus::Collected }));
    isRando = false;
    assert(!ReadMmFirstSlain(0, capture, &states) && states.size() == 3);
}

unsigned short flags[(RANDO_INF_MAX + 15) / 16]{};
bool Flags_GetRandoInf(RandoInf flag) { return (flags[flag/16] >> (flag%16)) & 1; }
void Own(RandoInf flag, bool own) {
    if (own) flags[flag/16] |= 1 << (flag%16);
    else flags[flag/16] &= ~(1 << (flag%16));
}
namespace Rando {
bool IsItemObtainable(RandoItemId, RandoCheckId);
RandoItemId ConvertItem(RandoItemId, RandoCheckId = RC_UNKNOWN);
}
__CONVERSION__
struct Actor { struct { struct { short y = 0; } rot; } shape; };
struct EnSlime { Actor actor; };
constexpr int MTXMODE_APPLY = 0;
void Matrix_RotateYS(short, int) {}
void Matrix_Scale(float, float, float, int) {}
RandoCheckId convertedCheck = RC_UNKNOWN;
RandoItemId drawnItem = RI_UNKNOWN;
RandoItemId Rando::ConvertItem(RandoItemId item, RandoCheckId check) {
    convertedCheck = check;
    return IsItemObtainable(item, check) ? item : RI_RUPEE_GREEN;
}
namespace Rando {
void DrawItem(RandoItemId item, RandoCheckId check, Actor*) {
    assert(check == RC_ENEMY_DROP_CHUCHU);
    drawnItem = item;
}
}
void Preview(bool* should, ...) {
    va_list args;
    va_start(args, should);
    __SLIME__
    va_end(args);
}
int main() {
    // Every appended NPC/animal/misc soul must be one-time even though it has
    // no vanilla get-item entry. Also cover a shared grant from another game.
    for (const auto& soul : SoulShuffle::Catalog) {
        auto& check = checks[RC_ENEMY_DROP_CHUCHU];
        check = {};
        assert(IsItemObtainable(soul.item, RC_ENEMY_DROP_CHUCHU));
        Own(soul.flag, true);
        assert(!IsItemObtainable(soul.item, RC_ENEMY_DROP_CHUCHU));
        assert(!IsItemObtainable(soul.item, RC_UNKNOWN));
        check.obtained = true;
        check.cycleObtained = false; // Song of Time resets only this flag.
        assert(!IsItemObtainable(soul.item, RC_ENEMY_DROP_CHUCHU));
        Own(soul.flag, false); // removing a soul does not reopen its old check
        assert(!IsItemObtainable(soul.item, RC_ENEMY_DROP_CHUCHU));
        check = {};
    }
    // First-cycle slime matches the original drop, then must preview the
    // renewable conversion on every later cycle, even for progressives whose
    // ownership alone is insufficient to distinguish copies.
    auto& check = checks[RC_ENEMY_DROP_CHUCHU];
    check.randoItemId = SoulShuffle::Catalog[0].item;
    EnSlime slime;
    bool should = true;
    Preview(&should, &slime);
    assert(!should && drawnItem == check.randoItemId);
    assert(convertedCheck == RC_ENEMY_DROP_CHUCHU);
    check.obtained = true;
    check.cycleObtained = false;
    should = true;
    Preview(&should, &slime);
    assert(!should && drawnItem == RI_RUPEE_GREEN);
    assert(convertedCheck == RC_ENEMY_DROP_CHUCHU);
    check.cycleObtained = true;
    should = true;
    drawnItem = RI_UNKNOWN;
    Preview(&should, &slime);
    assert(should && drawnItem == RI_UNKNOWN); // vanilla item after this cycle's pickup

    // Tracker reads every actual native ownership flag once, and uses the
    // right seed option. No transient collected-location flag participates.
    std::set<int> items, ownership;
    int counts[5]{};
    SoulTracker::ForEach([&](const SoulTracker::Entry& entry) {
        assert(items.insert(entry.item).second);
        assert(ownership.insert(entry.flag).second);
        ++counts[static_cast<int>(entry.family)];
        if (const auto* soul = SoulShuffle::Find(entry.item)) {
            assert(entry.flag == soul->flag && entry.option == soul->option);
        } else {
            assert(entry.flag == entry.item - RI_SOUL_BOSS_GOHT + RANDO_INF_OBTAINED_SOUL_OF_BOSS_GOHT);
        }
    });
    assert(counts[0] == 5 && counts[1] == 47 && counts[2] == 53 && counts[3] == 4 && counts[4] == 2);
    auto npc = SoulTracker::Rows(SoulTracker::Family::Npc, [](RandoOptionId option) {
        return option == RO_SHUFFLE_NPC_SOULS;
    });
    assert(npc.size() == 53);
    assert(SoulTracker::Rows(SoulTracker::Family::Enemy, [](RandoOptionId) { return false; }).empty());
    assert(SoulTracker::PageCount(0) == 1 && SoulTracker::PageCount(12) == 1 && SoulTracker::PageCount(13) == 2);
    CheckFirstSlainProgress();
    puts("PASS native soul ownership, cycle-reset preview, and first-slay status before and after pickup");
}
'''.replace("__CONVERSION__", prefix).replace("__SLIME__", slime).replace("__RECORD__", record).replace(
    "__FIRST_SLAIN__", first_slain)

with tempfile.TemporaryDirectory(prefix="mm-soul-preview-") as folder:
    folder = Path(folder)
    cpp = folder / "native.cpp"
    cpp.write_text(test)
    for mode in ("original", "combo"):
        exe = folder / mode
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-Wall", "-Wextra", "-I" + str(ROOT),
                        *(["-DCOMBO_BUILD"] if mode == "combo" else []), str(cpp), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
