#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using nlohmann::json;
static int failures = 0;
static int dropWarnings = 0;
static bool active = true;
static json liveExtraction;
static std::vector<json> packets;
json sNetShared = json::object();
json sNetFlat = json::object();

// IO boundaries only: the production rescan consumes this host's extracted view.
void ExtractShared(json& shared) { shared = liveExtraction; }
bool FleetShipCombo_IsThisGameActive() { return active; }
void NetSend(const json& packet) { packets.push_back(packet); }
void NetPoisonScan(const char*) {}
#define FS_TRACE(...) ((void)0)
#define SPDLOG_ERROR(...) (++dropWarnings)
#include "production.inc"

static void Check(bool condition, const char* reason) {
    if (!condition) {
        std::fprintf(stderr, "FAIL %s\n", reason);
        ++failures;
    }
}

static void CheckSender() {
    NetDelta delta{ json::object(), json::object() };
    delta.leaves["/inv/bombchu"] = true;
    delta.leaves["/inv/bombchuAmmo"] = 19;
    // Force multiple scalar packets so both ordering and batching are exercised.
    for (int i = 0; i < 200; ++i)
        delta.leaves["/other/" + std::to_string(i)] = std::string(24, 'x');
    delta.arrays["/comboObtained"] = json::array();
    delta.arrays["/comboObtainedFc"] = json::array();
    for (int i = 0; i < 128; ++i)
        delta.arrays["/comboObtained"].push_back(i == 95 ? 20 : 0);
    for (int i = 0; i < 512; ++i)
        delta.arrays["/comboObtainedFc"].push_back(i == 7 ? 1 : 0);
    delta.arrays["/bottleSlots"] = { 20, 255 };
    delta.arrays["/ownedItems"] = { 158, 159 };
    packets.clear();
    NetSendDelta(delta);

    int firstScalar = -1;
    int lastScalar = -1;
    int scalarPackets = 0;
    json arrayPosition = json::object();
    json receivedLeaves = json::object();
    json receivedArrays = json::object();
    for (size_t i = 0; i < packets.size(); ++i) {
        const auto& packet = packets[i];
        Check(packet.value("op", "") == "delta", "sender preserves delta envelope");
        Check(packet.dump().size() <= 4095, "every emitted packet fits transport limit");
        if (packet.contains("d")) {
            if (firstScalar < 0)
                firstScalar = static_cast<int>(i);
            lastScalar = static_cast<int>(i);
            ++scalarPackets;
            for (auto it = packet["d"].begin(); it != packet["d"].end(); ++it) {
                Check(!receivedLeaves.contains(it.key()), "scalar leaf emitted exactly once");
                receivedLeaves[it.key()] = it.value();
            }
        }
        if (packet.contains("a")) {
            Check(packet["a"].size() == 1, "array remains a whole subtree in its own packet");
            for (auto it = packet["a"].begin(); it != packet["a"].end(); ++it) {
                Check(!receivedArrays.contains(it.key()), "array emitted exactly once");
                receivedArrays[it.key()] = it.value();
                arrayPosition[it.key()] = i;
            }
        }
    }
    Check(scalarPackets > 1, "test crosses scalar packet boundary");
    Check(arrayPosition.value("/comboObtained", packets.size()) < static_cast<size_t>(firstScalar),
          "capacity array arrives before ammo scalars");
    Check(arrayPosition.value("/comboObtainedFc", packets.size()) < static_cast<size_t>(firstScalar),
          "receipt array arrives before ammo scalars");
    Check(arrayPosition.value("/bottleSlots", 0) > lastScalar &&
              arrayPosition.value("/ownedItems", 0) > lastScalar,
          "unrelated arrays keep their original position after scalar batches");
    Check(receivedLeaves == delta.leaves && receivedArrays == delta.arrays,
          "packet batching preserves every exact input value");

    // The existing oversize failure remains a dropped whole array, never fragments.
    packets.clear();
    dropWarnings = 0;
    delta = { {{"/inv/bombchuAmmo", 0}}, {{"/comboObtained", json::array({std::string(5000, 'x')})}} };
    NetSendDelta(delta);
    Check(dropWarnings == 1 && packets.size() == 1 && packets[0].contains("d"),
          "oversize dependency array is logged and dropped without losing scalar packet");
}

static void Reset(const json& previous, const json& extracted, bool isActive = true) {
    sNetShared = previous;
    sNetFlat = previous.flatten();
    liveExtraction = extracted;
    active = isActive;
}

static void CheckRescan() {
    const json owned = {{"inv", {{"stick", true}, {"stickAmmo", 9}, {"nut", true}, {"nutAmmo", 0}}}};
    const json unavailable = {{"inv", {{"stick", true}, {"nut", true}}}};
    for (bool isActive : {false, true}) {
        Reset(owned, unavailable, isActive);
        const auto expectedHash = NetHash(sNetFlat);
        const auto changes = NetRescan();
        Check(changes.empty(), "omitted ammo emits no zero or replacement delta");
        Check(sNetFlat.value("/inv/stickAmmo", -1) == 9 && sNetFlat.value("/inv/nutAmmo", -1) == 0,
              "canonical cache retains prior positive and zero ammo exactly");
        Check(NetHash(sNetFlat) == expectedHash, "non-authoritative omission leaves canonical hash unchanged");
        Check(!sNetShared["inv"].contains("stickAmmo") && !sNetShared["inv"].contains("nutAmmo"),
              "fresh shared extraction still omits non-authoritative ammo");
        const auto full = NetSplitAll(sNetFlat);
        Check(full.leaves.value("/inv/stickAmmo", -1) == 9 && full.leaves.value("/inv/nutAmmo", -1) == 0,
              "full network snapshot retains last known authoritative ammo");
        Check(NetRescan().empty(), "repeated omission never becomes a local change");
    }

    Reset(unavailable, unavailable);
    Check(NetRescan().empty() && !sNetFlat.contains("/inv/stickAmmo"),
          "no previous count means no synthesized ammo");

    Reset({{"inv", {{"stick", true}, {"stickAmmo", 9}, {"powderKegCount", 2}}},
           {"unrelated", 17}, {"other", {{"fakeAmmo", 3}}}}, unavailable);
    NetRescan();
    Check(!sNetFlat.contains("/unrelated") && !sNetFlat.contains("/inv/powderKegCount") &&
              !sNetFlat.contains("/other/fakeAmmo"),
          "unrelated omitted keys are not retained");

    Reset(owned, owned);
    for (int remaining : {8, 0}) {
        liveExtraction["inv"]["stickAmmo"] = remaining;
        const auto changes = NetRescan();
        Check(changes.leaves.value("/inv/stickAmmo", -1) == remaining && changes.arrays.empty(),
              "active spending including zero publishes exact new ammo");
        Check(sNetFlat.value("/inv/stickAmmo", -1) == remaining,
              "active spent ammo replaces cached count");
    }

    Reset(owned, owned, false);
    liveExtraction["inv"]["stickAmmo"] = 0;
    Check(NetRescan().empty() && sNetFlat.value("/inv/stickAmmo", -1) == 9,
          "inactive native zero cannot replace authoritative peer ammo");
    liveExtraction = unavailable;
    Check(NetRescan().empty() && sNetFlat.value("/inv/stickAmmo", -1) == 9,
          "inactive later omission retains the peer count too");
}

int main() {
    CheckSender();
    CheckRescan();
    std::printf("FleetNet ammo dependency/cache checks: %d failures\n", failures);
    return failures != 0;
}
