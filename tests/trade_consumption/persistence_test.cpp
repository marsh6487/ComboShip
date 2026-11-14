#include "z64save.h"
#include "z64item.h"
#include "mods/nei_save.h"
#include <nlohmann/json.hpp>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>

using json = nlohmann::json;

#ifndef ARRAY_COUNT
#define ARRAY_COUNT(value) (sizeof(value) / sizeof((value)[0]))
#endif

constexpr size_t kRevisionCount = 23;
constexpr uint32_t kAllTradeHistory = (1u << kRevisionCount) - 1;
constexpr uint64_t kCanary = UINT64_C(0xA9B81347CDEF0203);
static_assert(ARRAY_COUNT(NeiSaveData{}.tradeAdultRevision) == kRevisionCount);
static_assert(sizeof(NeiSaveData) - offsetof(NeiSaveData, tradeAdultRevision) -
                  sizeof(NeiSaveData{}.tradeAdultRevision) < alignof(NeiSaveData),
              "Trade revisions must remain appended after existing save fields");

struct GuardedNei {
    uint64_t before;
    NeiSaveData data;
    uint64_t after;
};
static GuardedNei storage;
static NeiSaveData& gNeiSave = storage.data;

static void ResetDestination() {
    // A legacy load must clear revisions left by a different save slot.
    std::memset(&gNeiSave, 0xA5, sizeof(gNeiSave));
    storage.before = storage.after = kCanary;
}

static void AssertCanaries() {
    assert(storage.before == kCanary && storage.after == kCanary);
}

#ifdef PERSISTENCE_MM
// RPG serialization is unrelated to these fields. Keep this boundary explicit:
// the production NeiSaveData codec still runs in full, including its memset.
namespace ComboRpg {
static json ToJson(const ComboRpgState&) { return json::object(); }
static void MergeJson(ComboRpgState&, const json&) {}
static void MigrateLegacySpeed(ComboRpgState&, uint8_t, uint8_t) {}
}
#else
// Controlled SaveManager boundary, backed by real nlohmann JSON. Array callbacks
// run once per requested element, including missing entries, so production's
// explicit default argument is what determines missing/short-array values.
class SaveManager {
  public:
    static SaveManager* Instance;
    json document = json::object();
    size_t savedRevisionCount = 0;
    size_t loadedRevisionCount = 0;

    template <class T> void SaveData(const char* name, const T& value) {
        if (*name)
            document[name] = value;
        else {
            assert(writeElement);
            *writeElement = value;
        }
    }

    template <class Callback> void SaveArray(const char* name, size_t count, Callback callback) {
        if (std::strcmp(name, "tradeAdultRevision") == 0)
            savedRevisionCount = count;
        json& array = document[name] = json::array();
        for (size_t i = 0; i < count; ++i) {
            array.push_back(nullptr);
            writeElement = &array[i];
            callback(i);
        }
        writeElement = nullptr;
    }

    template <class T> void LoadData(const char* name, T& destination, const T& fallback) {
        const json* value = readElement;
        if (*name) {
            const auto found = document.find(name);
            value = found == document.end() ? nullptr : &*found;
        }
        destination = value ? value->get<T>() : fallback;
    }

    template <class Callback> void LoadArray(const char* name, size_t count, Callback callback) {
        if (std::strcmp(name, "tradeAdultRevision") == 0)
            loadedRevisionCount = count;
        const auto found = document.find(name);
        const json* array = found != document.end() && found->is_array() ? &*found : nullptr;
        for (size_t i = 0; i < count; ++i) {
            readElement = array && i < array->size() ? &(*array)[i] : nullptr;
            callback(i);
        }
        readElement = nullptr;
    }

  private:
    json* writeElement = nullptr;
    const json* readElement = nullptr;
};
static SaveManager saveManager;
SaveManager* SaveManager::Instance = &saveManager;
namespace ItemGrantAudit {
struct Scope { explicit Scope(const char*) {} };
}
static void TradeItems_SyncWrite() {}
static void TradeItems_SyncRead() {}
static void Picto_SyncRead() {}
static void Bottle_WheelResetTracking() {}
static void RitoMask_MigrateLegacy() {}
#endif

// These are complete function bodies copied unchanged from the current source
// by persistence_run_tests.py; no revision or perk codec is reimplemented here.
// PRODUCTION_SERIALIZERS

static json EncodeNei() {
    json encoded;
#ifdef PERSISTENCE_MM
    to_json(encoded, gNeiSave);
#else
    saveManager.document = json::object();
    saveManager.savedRevisionCount = 0;
    SaveContext context{};
    NeiSave_Save(context, 0, true);
    encoded = saveManager.document;
    assert(saveManager.savedRevisionCount == kRevisionCount);
#endif
    assert(encoded.contains("tradeAdultRevision"));
    assert(encoded.at("tradeAdultRevision").is_array());
    assert(encoded.at("tradeAdultRevision").size() == kRevisionCount);
    // Exercise the actual JSON text representation, including UINT32_MAX.
    return json::parse(encoded.dump());
}

static void Load(const json& encoded) {
    ResetDestination();
#ifdef PERSISTENCE_MM
    from_json(encoded, gNeiSave);
#else
    saveManager.document = encoded;
    saveManager.loadedRevisionCount = 0;
    NeiSave_Load();
    assert(saveManager.loadedRevisionCount == kRevisionCount);
#endif
    AssertCanaries();
}

static void AssertRevisions(const std::array<uint32_t, kRevisionCount>& expected) {
    for (size_t i = 0; i < kRevisionCount; ++i) {
        assert(gNeiSave.tradeAdultRevision[i] == expected[i]);
        assert((gNeiSave.tradeAdultRevision[i] & 1u) == (expected[i] & 1u));
    }
}

static void AssertPerks(uint8_t effectOff) {
    assert(gNeiSave.pendantOwned == 1);
    assert(gNeiSave.pendantEffectOff == effectOff);
    assert(gNeiSave.cojiroPartnerOwned == 1);
    assert(gNeiSave.cojiroPartnerDismissed == 1);
}

static void TestAllRevisionSlotsRoundTrip() {
    // Every index must preserve both consumed and reacquired parity. Mixed and
    // sparse earned history must not be replaced by the availability revisions.
    for (uint32_t pass = 0; pass < 2; ++pass) {
        for (uint8_t effectOff : {uint8_t(0), uint8_t(1)}) {
            gNeiSave = {};
            const uint32_t history = pass ? UINT32_C(0x555555) : kAllTradeHistory;
            gNeiSave.tradeAdultOwned = history;
            std::array<uint32_t, kRevisionCount> expected{};
            for (size_t i = 0; i < kRevisionCount; ++i) {
                expected[i] = 1000u + 10u * i + ((i + pass) & 1u);
                gNeiSave.tradeAdultRevision[i] = expected[i];
            }
            gNeiSave.pendantOwned = 1;
            gNeiSave.pendantEffectOff = effectOff;
            gNeiSave.cojiroPartnerOwned = 1;
            gNeiSave.cojiroPartnerDismissed = 1;
            const json encoded = EncodeNei();
            assert(encoded.at("tradeAdultOwned").get<uint32_t>() == history);
            for (size_t i = 0; i < kRevisionCount; ++i)
                assert(encoded.at("tradeAdultRevision").at(i).get<uint32_t>() == expected[i]);
            Load(encoded);
            AssertRevisions(expected);
            assert(gNeiSave.tradeAdultOwned == history);
            AssertPerks(effectOff);
        }
    }
}

static void TestUint32Boundaries() {
    const uint32_t values[] = {0, 1, 2, 3, UINT32_C(0x7FFFFFFF), UINT32_C(0x80000000),
                               UINT32_MAX - 1u, UINT32_MAX};
    for (uint32_t value : values) {
        gNeiSave = {};
        gNeiSave.tradeAdultOwned = kAllTradeHistory;
        std::array<uint32_t, kRevisionCount> expected{};
        expected.fill(value);
        for (auto& revision : gNeiSave.tradeAdultRevision)
            revision = value;
        const json encoded = EncodeNei();
        Load(encoded);
        AssertRevisions(expected);
        assert(gNeiSave.tradeAdultOwned == kAllTradeHistory);
    }
}

static void TestLegacyAndMissingData() {
    const std::array<uint32_t, kRevisionCount> zero{};
    // Loading an old save after a consumed save must not inherit its revisions.
    json legacy = {{"tradeAdultOwned", kAllTradeHistory}, {"pendantOwned", 1},
                   {"pendantEffectOff", 1}, {"cojiroPartnerOwned", 1},
                   {"cojiroPartnerDismissed", 1}};
    Load(legacy);
    AssertRevisions(zero);
    assert(gNeiSave.tradeAdultOwned == kAllTradeHistory);
    AssertPerks(1);
    Load(json::object());
    AssertRevisions(zero);
    assert(gNeiSave.tradeAdultOwned == 0);
    assert(gNeiSave.pendantOwned == 0 && gNeiSave.pendantEffectOff == 0);
    for (auto item : gNeiSave.ownedItems)
        assert(item == 0xFF);
    for (auto item : gNeiSave.bottleSlots)
        assert(item == 0xFF);
}

static void TestShortAndExtraArrays() {
    for (size_t size : {size_t(0), size_t(1), size_t(12), size_t(22), size_t(23), size_t(40)}) {
        json revisions = json::array();
        std::array<uint32_t, kRevisionCount> expected{};
        for (size_t i = 0; i < size; ++i) {
            const uint32_t value = i == 0 ? UINT32_MAX : uint32_t(400 + i);
            revisions.push_back(value);
            if (i < expected.size())
                expected[i] = value;
        }
        const json encoded = {{"tradeAdultOwned", kAllTradeHistory}, {"tradeAdultRevision", revisions},
                              {"pendantOwned", 1}, {"pendantEffectOff", 1},
                              {"cojiroPartnerOwned", 1}, {"cojiroPartnerDismissed", 1}};
        Load(encoded);
        AssertRevisions(expected);
        assert(gNeiSave.tradeAdultOwned == kAllTradeHistory);
        AssertPerks(1);
    }
}

#ifdef PERSISTENCE_MM
static void TestMmMalformedRevisionContainers() {
    const std::array<uint32_t, kRevisionCount> zero{};
    for (const json& invalid : {json(nullptr), json(true), json(1), json(-1), json(2.0),
                               json("[1,2,3]"), json::object({{"0", 1}, {"22", 3}})}) {
        Load(json{{"tradeAdultOwned", kAllTradeHistory}, {"tradeAdultRevision", invalid},
                  {"pendantOwned", 1}, {"pendantEffectOff", 1}, {"cojiroPartnerOwned", 1},
                  {"cojiroPartnerDismissed", 1}});
        AssertRevisions(zero);
        assert(gNeiSave.tradeAdultOwned == kAllTradeHistory);
        AssertPerks(1);
    }
}

static void TestMmSparseAndInvalidEntries() {
    // Null holes are retained at their original indices; rejected entries must
    // neither wrap to a valid revision nor shift later entries to another item.
    const json cases[] = {nullptr, -1, int64_t(INT64_MIN), uint64_t(UINT32_MAX) + 1,
                          UINT64_MAX, true, false, 3.0, -0.5, "3", json::object(), json::array()};
    for (const json& invalid : cases) {
        for (size_t rejected = 0; rejected < kRevisionCount; ++rejected) {
            json revisions = json::array();
            std::array<uint32_t, kRevisionCount> expected{};
            for (size_t i = 0; i < kRevisionCount; ++i) {
                expected[i] = 600u + i;
                revisions.push_back(expected[i]);
            }
            revisions[rejected] = invalid;
            expected[rejected] = 0;
            Load(json{{"tradeAdultOwned", kAllTradeHistory}, {"tradeAdultRevision", revisions},
                      {"pendantOwned", 1}, {"pendantEffectOff", 1}, {"cojiroPartnerOwned", 1},
                      {"cojiroPartnerDismissed", 1}});
            AssertRevisions(expected);
            assert(gNeiSave.tradeAdultOwned == kAllTradeHistory);
            AssertPerks(1);
        }
    }
    json sparse = json::array();
    sparse[0] = 1;
    sparse[11] = 2;
    sparse[22] = UINT32_MAX;
    std::array<uint32_t, kRevisionCount> expected{};
    expected[0] = 1;
    expected[11] = 2;
    expected[22] = UINT32_MAX;
    Load(json{{"tradeAdultRevision", sparse}});
    AssertRevisions(expected);
    assert(gNeiSave.tradeAdultOwned == 0); // revisions alone never invent earned history
}
#endif

int main() {
    TestAllRevisionSlotsRoundTrip();
    TestUint32Boundaries();
    TestLegacyAndMissingData();
    TestShortAndExtraArrays();
#ifdef PERSISTENCE_MM
    TestMmMalformedRevisionContainers();
    TestMmSparseAndInvalidEntries();
    std::puts("PASS: MM native NEI JSON round trips, 23 revisions, legacy defaults, sparse/invalid data and permanent perks");
#else
    std::puts("PASS: OoT native NEI save/load functions, 23 revisions, JSON round trips, legacy defaults and permanent perks");
#endif
}
