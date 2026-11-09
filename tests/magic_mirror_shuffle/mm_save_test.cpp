#include "2s2h/BenJsonConversions.hpp"
#include "tests/test_require.h"
#include <memory>
int main() {
  auto original = std::make_unique<RandoSaveInfo>();
  original->finalSeed = 123456;
  original->randoSaveOptions[RO_SHUFFLE_MAGIC_MIRROR] = RO_GENERIC_ON;
  auto &stump = original->randoSaveChecks[RC_LOST_WOODS_MAGIC_MIRROR_STUMP];
  stump = {RI_BOW, true, false, true, true, false, 0};
  original->randoSaveChecks[0] = {RI_RUPEE_PURPLE, true, false, true, true,
                                  false,           40};
  json current = *original;
  auto restored = std::make_unique<RandoSaveInfo>();
  current.get_to(*restored);
  REQUIRE(restored->randoSaveOptions[RO_SHUFFLE_MAGIC_MIRROR] == RO_GENERIC_ON);
  REQUIRE(
      restored->randoSaveChecks[RC_LOST_WOODS_MAGIC_MIRROR_STUMP].randoItemId ==
      RI_BOW);
  REQUIRE(restored->randoSaveChecks[RC_LOST_WOODS_MAGIC_MIRROR_STUMP].obtained);
  json legacy = current;
  // A pre-Mirror save ends at the Mirror's stable native ordinal. Newer
  // features append more checks/options, so removing one current tail entry
  // no longer constructs the historical array shape.
  legacy["randoSaveChecks"].erase(
      legacy["randoSaveChecks"].begin() + RC_LOST_WOODS_MAGIC_MIRROR_STUMP,
      legacy["randoSaveChecks"].end());
  legacy["randoSaveOptions"].erase(
      legacy["randoSaveOptions"].begin() + RO_SHUFFLE_MAGIC_MIRROR,
      legacy["randoSaveOptions"].end());
  REQUIRE(legacy["randoSaveChecks"].size() == RC_LOST_WOODS_MAGIC_MIRROR_STUMP);
  REQUIRE(legacy["randoSaveOptions"].size() == RO_SHUFFLE_MAGIC_MIRROR);
  // Every omitted addition starts dirty so the complete native reset, not
  // zero initialization, must clear the newer option/check tail.
  for (size_t i = RO_SHUFFLE_MAGIC_MIRROR; i < RO_MAX; ++i)
    restored->randoSaveOptions[i] = RO_GENERIC_ON;
  for (size_t i = RC_LOST_WOODS_MAGIC_MIRROR_STUMP; i < RC_MAX; ++i)
    restored->randoSaveChecks[i] = {RI_BOW, true, true, true, true, true, 99};
  legacy.get_to(*restored); // Reuse a nonzero live destination.
  for (size_t i = RO_SHUFFLE_MAGIC_MIRROR; i < RO_MAX; ++i)
    REQUIRE(restored->randoSaveOptions[i] == RO_GENERIC_OFF);
  for (size_t i = RC_LOST_WOODS_MAGIC_MIRROR_STUMP; i < RC_MAX; ++i) {
    const auto &check = restored->randoSaveChecks[i];
    REQUIRE(check.randoItemId == RI_UNKNOWN && !check.shuffled && !check.eligible &&
            !check.cycleObtained && !check.obtained && !check.skipped && check.price == 0);
  }
  REQUIRE(restored->randoSaveOptions[RO_SHUFFLE_MAGIC_MIRROR] ==
          RO_GENERIC_OFF);
  REQUIRE(
      !restored->randoSaveChecks[RC_LOST_WOODS_MAGIC_MIRROR_STUMP].obtained);
  REQUIRE(
      !restored->randoSaveChecks[RC_LOST_WOODS_MAGIC_MIRROR_STUMP].eligible);
  REQUIRE(
      !restored->randoSaveChecks[RC_LOST_WOODS_MAGIC_MIRROR_STUMP].shuffled);
  REQUIRE(
      restored->randoSaveChecks[RC_LOST_WOODS_MAGIC_MIRROR_STUMP].randoItemId ==
      RI_UNKNOWN);
  REQUIRE(restored->randoSaveChecks[0].randoItemId == RI_RUPEE_PURPLE);
  REQUIRE(restored->randoSaveChecks[0].obtained &&
          restored->randoSaveChecks[0].price == 40);
  REQUIRE(restored->finalSeed == 123456);
  current["randoSaveChecks"].push_back(current["randoSaveChecks"][0]);
  current["randoSaveOptions"].push_back(99);
  current.get_to(*restored);
  REQUIRE(restored->randoSaveOptions[RO_SHUFFLE_MAGIC_MIRROR] == RO_GENERIC_ON);
  REQUIRE(
      restored->randoSaveChecks[RC_LOST_WOODS_MAGIC_MIRROR_STUMP].randoItemId ==
      RI_BOW);
  REQUIRE(restored->randoSaveChecks[RC_LOST_WOODS_MAGIC_MIRROR_STUMP].obtained);
  puts("PASS native MM save JSON: current assigned/collected stump roundtrip; "
       "old shorter arrays default OFF/uncollected; future tails ignored");
}
