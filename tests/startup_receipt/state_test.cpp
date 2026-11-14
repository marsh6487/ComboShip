#include <cassert>
#include <iostream>
#if __has_include("combo/rando/StartupCheckReceiptState.h")
#include "combo/rando/StartupCheckReceiptState.h"
int main() {
    using namespace ComboStartupReceipt;
    nlohmann::json document = {{"combo", {{"notes", "preserve"}}}, {"oot", {{"sentinel", 1}}},
                               {"mm", {{"sentinel", 2}}}};
    CwStartupReceipt receipt{};
    assert(!Peek(document, receipt));
    BeginFile(document, 7);
    assert(!Peek(document, receipt));
    receipt.version = CW_STARTUP_RECEIPT_VERSION;
    receipt.state = CW_STARTUP_READY;
    receipt.itemGame = 0;
    receipt.modIndex = 1;
    receipt.getItemId = 10;
    receipt.itemId = 5;
    receipt.category = 6;
    receipt.importance = CW_STARTUP_MAJOR;
    std::snprintf(receipt.itemName, sizeof(receipt.itemName), "%s", "Bow");
    std::snprintf(receipt.displayName, sizeof(receipt.displayName), "%s", "Fairy Bow");
    assert(Capture(document, receipt));
    CwStartupReceipt found{};
    assert(Peek(document, found) && found.generation == 7);
    assert(!Acknowledge(document, 8));
    assert(Acknowledge(document, 7));
    assert(!Peek(document, found));
    assert(!Acknowledge(document, 7));
    assert(document["combo"]["notes"] == "preserve");
    assert(document["oot"]["sentinel"] == 1 && document["mm"]["sentinel"] == 2);
    BeginFile(document, 8);
    receipt.itemGame = 1;
    receipt.state = CW_STARTUP_WAIT_MM;
    assert(Capture(document, receipt));
    assert(!Peek(document, found));
    assert(PendingMm(document, found) && found.generation == 8);
    found.state = CW_STARTUP_READY;
    assert(CompleteMm(document, found, {{"awarded", 1}}));
    assert(Peek(document, found));
    assert(!PendingMm(document, found));
    assert(!CompleteMm(document, found, {{"awarded", 2}}));
    assert(document["mm"]["awarded"] == 1);
    // Native MM custom items use GetItemDrawId::GID_NONE (-1). Their concrete
    // item ID drives presentation, so preserve this valid metadata on disk.
    auto noNativeDraw = found;
    noNativeDraw.drawItemId = -1;
    assert(Valid(noNativeDraw));
    CwStartupReceipt decoded{};
    assert(Decode(Encode(noNativeDraw), decoded) && decoded.drawItemId == -1);
    noNativeDraw.itemGame = 0;
    assert(!Valid(noNativeDraw));
    noNativeDraw.itemGame = 1;
    noNativeDraw.drawItemId = -2;
    assert(!Valid(noNativeDraw));
    noNativeDraw.drawItemId = 65536;
    assert(!Valid(noNativeDraw));
    document["combo"]["startupReceipt"]["version"] = 999;
    assert(!Peek(document, found));

    // Fresh containers, malformed metadata, and serialized numeric types are
    // real save boundaries. Rejected input must leave unrelated state alone.
    nlohmann::json fresh;
    assert(BeginFile(fresh, 11));
    assert(Capture(fresh, receipt));
    fresh = nlohmann::json::parse(fresh.dump());
    assert(PendingMm(fresh, found) && found.generation == 11);
    for (const auto& malformed : {nlohmann::json::array(), nlohmann::json(7),
                                  nlohmann::json{{"combo", "keep this invalid metadata"}}}) {
        auto rejected = malformed;
        assert(!BeginFile(rejected, 12) && rejected == malformed);
        assert(!Capture(rejected, receipt) && rejected == malformed);
        assert(!Peek(rejected, found));
    }
    nlohmann::json absent = nlohmann::json::object();
    assert(!Capture(absent, receipt) && absent.empty());
    const auto unchanged = fresh;
    assert(!BeginFile(fresh, 0) && fresh == unchanged);
    auto invalid = fresh;
    invalid["combo"]["startupReceipt"]["gi"] = std::numeric_limits<uint64_t>::max();
    assert(!PendingMm(invalid, found));
    invalid = fresh;
    invalid["combo"]["startupReceipt"]["itemName"] = std::string("bad\0name", 8);
    assert(!PendingMm(invalid, found));
    invalid = fresh;
    invalid["combo"]["startupReceipt"]["generation"] = -1;
    assert(!PendingMm(invalid, found));
    invalid = fresh;
    invalid["combo"]["startupReceipt"]["shown"] = 0;
    assert(!PendingMm(invalid, found));

    MmCompletion pending;
    assert(pending.Begin(fresh));
    assert(!pending.Stage(nlohmann::json::array()));
    pending.receipt.state = CW_STARTUP_READY;
    assert(pending.Stage({{"awarded", 1}}));
    assert(!pending.Commit(fresh, [](const auto&) { return true; }));
    assert(pending.StagePeer({{"sharedTier", 1}}));
    assert(!pending.Commit(fresh, [](const auto&) { return true; })); // Stale queued peer is not proof.
    pending.PreparePeerCapture();
    assert(!pending.Commit(fresh, [](const auto&) { return true; })); // Fresh writer must arrive.
    assert(pending.StagePeer({{"sharedTier", 1}}));
    int attempts = 0;
    assert(!pending.Commit(fresh, [&](const auto&) { ++attempts; return false; }));
    assert(fresh == unchanged && pending.Ready());
    assert(pending.Commit(fresh, [&](const auto& draft) {
        ++attempts;
        assert(draft["mm"]["awarded"] == 1);
        assert(draft["oot"]["sharedTier"] == 1);
        CwStartupReceipt resolved{};
        assert(Peek(draft, resolved) && resolved.generation == 11);
        return true;
    }));
    assert(attempts == 2 && Peek(fresh, found));
    assert(!pending.Commit(fresh, [](const auto&) { return true; }));
    assert(!pending.Matches(fresh));
    std::cout << "PASS startup receipt persistence, deferred MM completion, preserved state and one-time acknowledgement\n";
}
#else
int main() {
    std::cerr << "FAIL startup receipt persistence is not implemented\n";
    return 1;
}
#endif
