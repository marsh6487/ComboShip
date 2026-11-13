#include <cstdio>
#include <nlohmann/json.hpp>

void OotReset();
void OotSet(int which, int ammo, bool owned);
int OotAmmo(int which);
void OotBag(int which, bool owned);
void OotBombchuBag(int level, bool progressive, bool infinite);
void OotCapacitySnapshot(nlohmann::json &sh);
void OotApplyCapacity(const nlohmann::json &sh);
void OotLegacyReceipt();
void OotReplayReceipt();
int OotReplayGrants();
void OotReplayCapacity(int which);
int OotTier(int which);
void OotSetTier(int which, int tier);
void OotExtract(nlohmann::json &inv, int which);
void OotApply(const nlohmann::json &inv, int which);
void MmReset();
void MmSet(int which, int ammo, bool owned);
int MmAmmo(int which);
void MmBag(int which, bool owned);
void MmRepair();
void MmExtract(nlohmann::json &inv, int which);
void MmApply(const nlohmann::json &inv, int which);
void MmCapacitySnapshot(const nlohmann::json &sh);
void MmNativeBombBag(int level);
void MmSpendBombchu();
int MmIndependentCapacity();
int MmIndependentInfinite();
void MmLegacyReceipt();
void MmReplayReceipt();
int MmReplayGrants();
void MmReplayCapacity(int which);
int MmTier(int which);
void MmSetTier(int which, int tier);
bool MmHasBag(int which);

static int failures = 0;
static void Check(bool passed, const char *reason) {
  if (!passed) {
    std::fprintf(stderr, "FAIL %s\n", reason);
    ++failures;
  }
}

int main() {
  const char *names[] = {"stick", "nut", "bombchu"};
  for (int which = 0; which < 2; ++which) {
    OotReset();
    MmReset();
    OotSet(which, 9, true);
    nlohmann::json inv = nlohmann::json::object();
    OotExtract(inv, which);
    MmApply(inv, which);
    Check(MmAmmo(which) == 0, "missing MM bag cannot import usable ammo");
    MmRepair();
    MmExtract(inv, which);
    Check(!inv.contains(std::string(names[which]) + "Ammo"),
          "missing bag removes old authoritative ammo leaf");
    OotApply(inv, which);
    Check(OotAmmo(which) == 9,
          "missing MM bag must not erase OoT ammo on return");

    MmBag(which, true);
    OotSet(which, 9, true);
    inv = nlohmann::json::object();
    OotExtract(inv, which);
    MmApply(inv, which);
    MmRepair();
    Check(MmAmmo(which) == 9, "owned MM bag accepts native ammo");
    for (int remaining : {8, 0}) {
      MmSet(which, remaining, true);
      MmExtract(inv, which);
      OotApply(inv, which);
      Check(OotAmmo(which) == remaining,
            "real spending including zero stays spent");
    }
    OotBag(which, false);
    OotSet(which, 0, true);
    MmSet(which, 9, true);
    OotExtract(inv, which);
    Check(!inv.contains(std::string(names[which]) + "Ammo"),
          "missing OoT bag cannot author MM ammo");
    MmApply(inv, which);
    Check(MmAmmo(which) == 9, "missing OoT bag preserves usable MM ammo");
    MmExtract(inv, which);
    OotApply(inv, which);
    Check(OotAmmo(which) == 0, "missing OoT bag cannot import usable ammo");
  }
  for (int which = 0; which < 3; ++which) {
    OotReset();
    MmReset();
    OotSet(which, 7, true);
    MmSet(which, 7, true);
    nlohmann::json absent = {{names[which], false},
                             {std::string(names[which]) + "Ammo", 0}};
    OotApply(absent, which);
    MmApply(absent, which);
    Check(OotAmmo(which) == 7 && MmAmmo(which) == 7,
          "unowned peer item cannot clear local ammo");
    OotSet(which, 7, true);
    MmSet(which, 7, true);
    OotApply(nlohmann::json::object(), which);
    MmApply(nlohmann::json::object(), which);
    Check(OotAmmo(which) == 7 && MmAmmo(which) == 7,
          "partial delta preserves omitted ammo");
    if (which < 2)
      MmBag(which, true);
    for (int remaining : {6, 0}) {
      nlohmann::json delta = {{std::string(names[which]) + "Ammo", remaining}};
      OotApply(delta, which);
      MmApply(delta, which);
      Check(OotAmmo(which) == remaining && MmAmmo(which) == remaining,
            "ammo-only network delta updates owned items including spent zero");
    }
    OotSet(which, 0, false);
    MmSet(which, 0, false);
    nlohmann::json inv = {{std::string(names[which]) + "Ammo", 99}};
    OotExtract(inv, which);
    Check(!inv.contains(std::string(names[which]) + "Ammo"),
          "unowned OoT item cannot publish an ammo count");
    MmExtract(inv, which);
    Check(!inv.contains(std::string(names[which]) + "Ammo"),
          "unowned MM item cannot publish an ammo count");
    nlohmann::json delta = {{std::string(names[which]) + "Ammo", 5}};
    OotApply(delta, which);
    MmApply(delta, which);
    Check(OotAmmo(which) == 0 && MmAmmo(which) == 0,
          "ammo-only delta cannot grant an unowned item");
  }
  for (int level = 0; level <= 3; ++level) {
    for (bool progressive : {false, true}) {
      for (bool infinite : {false, true}) {
        OotReset();
        MmReset();
        OotSet(2, 0, true);
        OotBombchuBag(level, progressive, infinite);
        nlohmann::json sh;
        OotCapacitySnapshot(sh);
        MmCapacitySnapshot(sh);
        const int capacities[] = {0, 20, 30, 50};
        const int expected = infinite || !progressive ? 50 : capacities[level];
        Check(MmIndependentCapacity() == expected,
              "native OoT Bombchu capacity reaches MM exactly");
        Check(MmIndependentInfinite() == infinite,
              "independent Bombchu infinity reaches MM");
        MmCapacitySnapshot(sh);
        Check(MmIndependentCapacity() == expected,
              "capacity snapshot repeat is idempotent");
        MmCapacitySnapshot(nlohmann::json::object());
        Check(MmIndependentCapacity() == expected,
              "partial snapshot does not lower capacity");
      }
    }
  }
  OotReset();
  MmReset();
  OotBombchuBag(3, true, true);
  nlohmann::json sh;
  OotCapacitySnapshot(sh);
  MmCapacitySnapshot(sh);
  Check(MmIndependentCapacity() == 0 && !MmIndependentInfinite(),
        "no inventory ownership cannot publish a bag");
  MmSet(2, 20, true);
  MmCapacitySnapshot(nlohmann::json::object());
  Check(MmIndependentCapacity() == 0,
        "native MM Bombchus do not imply a foreign bag");
  MmLegacyReceipt();
  Check(MmIndependentCapacity() == 20,
        "legacy real foreign receipt repairs minimum capacity");
  MmSet(2, 0, true);
  MmLegacyReceipt();
  Check(MmAmmo(2) == 0, "legacy repair never refills spent ammo");
  for (int remaining : {19, 0}) {
    OotReset();
    MmReset();
    OotSet(2, remaining, true);
    MmSet(2, remaining, true);
    OotReplayReceipt();
    MmReplayReceipt();
    Check(
        OotAmmo(2) == remaining && MmAmmo(2) == remaining,
        "deferred foreign bag materialization preserves exact remaining ammo");
    OotReplayReceipt();
    MmReplayReceipt();
    Check(OotReplayGrants() == 1 && MmReplayGrants() == 1,
          "foreign bag receipt materializes exactly once");
  }
  OotReset();
  MmReset();
  OotReplayReceipt();
  MmReplayReceipt();
  Check(OotAmmo(2) == 20 && MmAmmo(2) == 20,
        "first receipt without existing item keeps native initial refill");
  for (int which = 0; which < 2; ++which) {
    for (bool countFirst : {false, true}) {
      for (bool mmBag : {false, true}) {
        OotReset();
        MmReset();
        MmBag(which, mmBag);
        OotSet(which, 0, true);
        MmSet(which, 0, true);
        if (countFirst) {
          OotReplayCapacity(which);
          MmReplayCapacity(which);
          Check(OotTier(which) == 1 && MmTier(which) == 1,
                "count-only capacity receipt waits for native state without "
                "guessing a tier");
        }
        OotSetTier(which, 2);
        MmSetTier(which, 2);
        OotReplayCapacity(which);
        MmReplayCapacity(which);
        Check(OotTier(which) == 2 && MmTier(which) == 2 &&
                  OotAmmo(which) == 0 && MmAmmo(which) == 0,
              "capacity receipt cannot increment already imported tier or "
              "refill spent ammo");
        OotReplayCapacity(which);
        MmReplayCapacity(which);
        Check(OotTier(which) == 2 && MmTier(which) == 2 &&
                  OotAmmo(which) == 0 && MmAmmo(which) == 0,
              "repeated capacity receipt remains idempotent");
        Check(MmHasBag(which) == mmBag,
              "capacity receipt preserves MM local base-bag ownership");
      }
    }
  }
  for (int level = 1; level <= 3; ++level) {
    const int capacities[] = {0, 20, 30, 50};
    OotReset();
    MmReset();
    MmNativeBombBag(0);
    OotBombchuBag(level, true, false);
    OotSet(2, capacities[level], true);
    nlohmann::json capacity;
    nlohmann::json inv;
    OotCapacitySnapshot(capacity);
    OotExtract(inv, 2);
    MmCapacitySnapshot(capacity);
    MmApply(inv, 2);
    MmSpendBombchu();
    Check(MmAmmo(2) == capacities[level] - 1,
          "native MM use consumes one independent Bombchu without a Bomb Bag");
    MmExtract(inv, 2);
    OotApply(inv, 2);
    Check(OotAmmo(2) == capacities[level] - 1,
          "native Bombchu consumption survives the complete OoT-MM-OoT helper round trip");
  }
  for (int remaining : {6, 0}) {
    OotReset();
    OotSet(2, remaining, true);
    nlohmann::json capacity;
    nlohmann::json inv;
    OotCapacitySnapshot(capacity);
    OotExtract(inv, 2);
    OotReset();
    OotBombchuBag(0, true, false);
    OotApplyCapacity(capacity);
    OotApply(inv, 2);
    Check(OotAmmo(2) == remaining, "first OoT bag import uses earned capacity before deferred receipt");
    OotReplayReceipt();
    Check(OotAmmo(2) == remaining, "first OoT bag receipt preserves imported six or spent zero");
    MmReset();
    MmNativeBombBag(0);
    MmCapacitySnapshot(capacity);
    MmApply(inv, 2);
    MmReplayReceipt();
    Check(MmAmmo(2) == remaining, "first MM bag receipt preserves imported six or spent zero");
    OotReset();
    OotBombchuBag(0, true, false);
    OotSet(2, 0, true);
    OotLegacyReceipt();
    const nlohmann::json delta = {{"bombchuAmmo", remaining}};
    OotApplyCapacity(nlohmann::json::object());
    OotApply(delta, 2);
    OotReplayReceipt();
    Check(OotAmmo(2) == remaining, "saved pending legacy bag receipt authorizes its later ammo delta");
  }
  std::printf("Ammo sync native-host checks: %d failures\n", failures);
  return failures != 0;
}
