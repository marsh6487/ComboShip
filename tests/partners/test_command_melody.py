"""Execute production song acquisition and pool hooks without a game runtime."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from


class CommandMelody(unittest.TestCase):
    maxDiff = 400

    def run_cpp(self, source):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "test.cpp"
            path.write_text(source)
            binary = Path(tmp) / "test"
            subprocess.run(["c++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                            "-I" + str(ROOT / "mm"),
                            str(path), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)

    def test_vanilla_grant_never_unlocks_randomizer_or_dormant_saves(self):
        for game, field, bit in (("soh", "mmQuestItems", "FC_MMQ_SONG_TIME"),
                                 ("mm", "ootQuestItems", "(1u << OOT_QUEST_SONG_COMMAND_MELODY)")):
            with self.subTest(game=game):
                source = (ROOT / game / "mods/partners/Partners.cpp").read_text()
                signature = "static void LearnVanillaCommandMelody("
                self.assertTrue(signature in source, "vanilla song acquisition is missing")
                body = block_from(source, source.index(signature))
                tick = block_from(source, source.index("void Tick("))
                self.assertIn("LearnVanillaCommandMelody();", tick)
                self.run_cpp(r'''
#include <cassert>
#include <cstdint>
struct Save { uint32_t mmQuestItems=0, ootQuestItems=0; } save;
Save* Nei_Save() { return &save; }
bool active=true, randomizer=false;
bool Partners_IsActive() { return active; }
#define IS_RANDO randomizer
constexpr uint32_t FC_MMQ_SONG_TIME=1u<<12, OOT_QUEST_SONG_COMMAND_MELODY=16;
''' + body + r'''
int main() {
    active=false; LearnVanillaCommandMelody();
    assert(save.mmQuestItems==0 && save.ootQuestItems==0);
    active=true; randomizer=true; LearnVanillaCommandMelody();
    assert(save.mmQuestItems==0 && save.ootQuestItems==0);
    randomizer=false;
    save.mmQuestItems=1; save.ootQuestItems=2;
    LearnVanillaCommandMelody();
''' + f'assert(save.{field} & {bit});\n' + r'''
    const auto learned=save; LearnVanillaCommandMelody();
    assert(save.mmQuestItems==learned.mmQuestItems && save.ootQuestItems==learned.ootQuestItems);
    // Loading an unowned rando slot must not inherit a vanilla entitlement.
    save={}; randomizer=true; LearnVanillaCommandMelody();
    assert(save.mmQuestItems==0 && save.ootQuestItems==0);
}
''')

    def test_one_shuffled_song_with_combined_supply_owned_by_mm(self):
        source = (ROOT / "soh/soh/Enhancements/randomizer/3drando/item_pool.cpp").read_text()
        signature = "static void AddCommandMelodyToPool("
        self.assertTrue(signature in source, "solo OoT has no shuffled Command Melody")
        oot = block_from(source, source.index(signature))
        self.assertIn("AddCommandMelodyToPool();", block_from(source, source.index("bool GenerateItemPool(")))
        source = (ROOT / "mm/2s2h/Rando/Logic/GeneratePools.cpp").read_text()
        signature = "static void EnsureCommandMelodyInPool("
        self.assertTrue(signature in source, "MM song supply still depends on unrelated options")
        mm = block_from(source, source.index(signature))
        self.assertIn("EnsureCommandMelodyInPool(itemPool);", block_from(source, source.index("void GeneratePools(")))
        self.run_cpp(r'''
#include <algorithm>
#include <cassert>
#include <vector>
enum RandoItemId {RI_JUNK, RI_OOT_SONG_COMMAND_MELODY};
constexpr int RG_NEI_SONG_COMMAND_MELODY=5;
bool combined=false;
bool Combo_IsCombinedGame() { return combined; }
std::vector<int> itemPool;
void AddFixedItemToPool(int item, int count) { while(count--) itemPool.push_back(item); }
''' + oot + mm + r'''
int main() {
    AddCommandMelodyToPool();
    assert(itemPool==std::vector<int>{RG_NEI_SONG_COMMAND_MELODY});
    itemPool.clear(); combined=true; AddCommandMelodyToPool(); assert(itemPool.empty());
    std::vector<RandoItemId> mmPool{RI_JUNK};
    EnsureCommandMelodyInPool(mmPool); EnsureCommandMelodyInPool(mmPool);
    assert(mmPool.size()==2 && mmPool.front()==RI_JUNK);
    assert(std::count(mmPool.begin(),mmPool.end(),RI_OOT_SONG_COMMAND_MELODY)==1);
    // The combo song swap / solo quest-item option may already supply it.
    mmPool={RI_OOT_SONG_COMMAND_MELODY}; EnsureCommandMelodyInPool(mmPool);
    assert(mmPool.size()==1);
}
''')

    def test_mm_market_and_native_reward_share_the_learned_flag(self):
        # Exercise the native song grant and shop ownership switch arms together.
        # A wrong quest store/bit, or substituting check-completion for ownership,
        # makes the pre/post-grant assertions fail.
        case = "case RI_OOT_SONG_COMMAND_MELODY:"
        source = (ROOT / "mm/2s2h/Enhancements/Gameplay/BlackMarketItems.cpp").read_text()
        owned = source[source.index(case):source.index("case ", source.index(case) + len(case))]
        source = (ROOT / "mm/2s2h/Rando/GiveItem.cpp").read_text()
        grant = source[source.index(case):source.index("case ", source.index(case) + len(case))]
        self.run_cpp('#include "' + str(ROOT / 'mm/mods/nei_save.h') + '"\n' + r'''
#include <cassert>
enum {RI_OOT_SONG_COMMAND_MELODY=1};
NeiSaveData save{};
extern "C" NeiSaveData* Nei_Save() { return &save; }
bool Owned(int id) { const auto* nei=Nei_Save(); switch(id) {
''' + owned + r'''
default: return false; } }
void Grant(int id) { switch(id) {
''' + grant + r'''
default: break; } }
int main() {
    save.ootQuestItems=1; save.cojiroPartnerOwned=1;
    assert(!Owned(RI_OOT_SONG_COMMAND_MELODY));
    Grant(RI_OOT_SONG_COMMAND_MELODY);
    assert(Owned(RI_OOT_SONG_COMMAND_MELODY));
    assert(save.ootQuestItems==(1u | (1u << OOT_QUEST_SONG_COMMAND_MELODY)));
    assert(save.cojiroPartnerOwned==1);
    auto learned=save.ootQuestItems; Grant(RI_OOT_SONG_COMMAND_MELODY);
    assert(save.ootQuestItems==learned);
}
''')


if __name__ == "__main__":
    unittest.main()
