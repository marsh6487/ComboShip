// Actual native grant arms and cane ownership functions; save access is a
// boundary.
#include "FleetShipCombo/FleetShipCombo.h"
#include "Rando/Types.h"
#include "global.h"
#include "mods/extended_inventory.h"
#include "vanilla/ComboVanillaMode.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

static int activeGame = -1;
extern "C" {
#ifdef COMBO_BUILD
int gComboGameMode = COMBO_GAME_MM;
int gComboVanillaMode = 0;
#endif
int FleetShipCombo_GetActiveGame() { return activeGame; }
}
#include "unified_pool.inc"

static NeiSaveData saved{};
extern "C" NeiSaveData *Nei_Save() { return &saved; }
extern "C" void Nei_SetOwnedItem(uint8_t slot, uint16_t item) {
  saved.ownedItems[slot - 24] = item;
}
/* PRODUCTION */
// Native obtainability is true for an eligible cane; execute its actual
// conversion arm.
static RandoItemId ConvertEligibleCane(RandoItemId item) {
  switch (item) {
    /* CONVERT_ARM */
  default:
    return item;
  }
}
static void Grant(RandoItemId item) {
  switch (item) {
    /* GRANT_ARMS */
  default:
    std::abort();
  }
}

static int failures;
static void expect(bool ok, const char *message) {
  if (!ok && failures++ < 12)
    std::fprintf(stderr, "FAIL: %s\n", message);
}
static void reset(unsigned mask) {
  saved = {};
  std::fill(std::begin(saved.ownedItems), std::end(saved.ownedItems),
            ITEM_NONE);
  saved.caneSkills = mask;
  saved.ootQuestItems = 0xA0000001;
  saved.twilightUpgrade = 1;
  saved.comboObtainedFc[0] = 3;
}

int main() {
  constexpr int order[] = {0, 3, 1, 4, 2, 5};
  int grants = 0;
  for (bool paired : {false, true})
    for (bool vanilla : {false, true}) {
#ifdef COMBO_BUILD
      gComboGameMode = paired ? COMBO_GAME_BOTH : COMBO_GAME_MM;
      gComboVanillaMode = vanilla;
      activeGame = 0;
#else
      activeGame = paired ? 0 : -1;
      (void)vanilla;
#endif
      for (bool converted : {false, true})
        for (unsigned mask = 0; mask < 64; ++mask) {
          reset(mask);
          for (int pickup = 0; pickup < 8; ++pickup) {
            const auto before = saved;
            unsigned expected = before.caneSkills;
            for (int skill : order) {
              if ((!paired && skill == 2) || (expected & (1u << skill)))
                continue;
              expected |= 1u << skill;
              break;
            }
            const auto item =
                converted ? ConvertEligibleCane(RI_OOT_NEI_CANE_OF_SOMARIA)
                          : RI_OOT_NEI_CANE_OF_SOMARIA;
            expect(paired || item != RI_OOT_NEI_CANE_SOMARIA_PLATFORM,
                   "MM-only converted progressive cane never resolves to "
                   "explicit Trirod");
            Grant(item);
            expect(saved.caneSkills == expected,
                   "progressive cane grants the next functional skill for this "
                   "mode");
            expect((saved.caneSkills & mask) == mask,
                   "previously owned skills, including legacy Trirod, remain "
                   "owned");
            auto allowed = before;
            allowed.caneSkills = expected;
            if (before.caneSkills == 0 && expected != 0) {
              expect(saved.ownedItems[SLOT_CANE_OF_SOMARIA - 24] ==
                         ITEM_CANE_OF_SOMARIA,
                     "first eligible skill grants the native cane cell");
              allowed.ownedItems[SLOT_CANE_OF_SOMARIA - 24] =
                  saved.ownedItems[SLOT_CANE_OF_SOMARIA - 24];
              allowed.caneType = saved.caneType;
              std::copy(std::begin(saved.caneSkillSel),
                        std::end(saved.caneSkillSel), allowed.caneSkillSel);
            }
            expect(std::memcmp(&allowed, &saved, sizeof(saved)) == 0,
                   "no unrelated save fields or saturated-chain writes");
            ++grants;
          }
        }
      reset(0);
      expect(ConvertEligibleCane(RI_OOT_NEI_CANE_SOMARIA_PLATFORM) ==
                 RI_OOT_NEI_CANE_SOMARIA_PLATFORM,
             "explicit legacy Trirod identity is not converted to a different "
             "item");
      Grant(RI_OOT_NEI_CANE_SOMARIA_PLATFORM);
      expect(saved.caneSkills == (1u << 2),
             "legacy explicit Trirod grants remain supported in every mode");
      Grant(RI_OOT_SONG_FUGUE_OF_HOME);
      Grant(RI_OOT_SONG_BALLAD_OF_THE_HERO);
      expect(saved.ootQuestItems ==
                 (0xA0000001u | (1u << OOT_QUEST_SONG_FUGUE_OF_HOME) |
                  (1u << OOT_QUEST_SONG_BALLAD_OF_HERO)),
             "legacy song IDs still grant their original saved ownership bits");
    }
  std::printf("%s native progressive cane: %d grants across all owned "
              "masks/modes; legacy direct grants retained\n",
              failures ? "FAIL" : "PASS", grants);
  return failures != 0;
}
