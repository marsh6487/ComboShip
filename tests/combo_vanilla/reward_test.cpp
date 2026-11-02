#include "combo/vanilla/ComboVanillaRewards.h"
#include <cassert>
#include <iostream>

using namespace ComboVanilla;
using nlohmann::json;

static json NewFile() {
    return {{"combo", {{"mode", "vanilla-nei"}, {"vanilla", {{"version", 1}, {"collected", json::object()}}}}},
            {"oot", {{"fileType", 0}, {"version", 1}, {"sections", {{"base", {{"data", {{"rupees", 99}}}}}}}}},
            {"mm", {{"type", "2S2H_SAVE"}, {"version", 1}, {"newCycleSave", {{"save", {{"shipSaveInfo", {{"saveType", 0}}}}}}}}}};
}

int main() {
    static_assert(ValidRewardManifest());
    const auto* lantern = FindReward("vanilla.nei.lantern");
    assert(lantern && lantern->price == 40 && lantern->payload == 0xB5);
    const auto* rune = FindReward("vanilla.nei.slate.sensor");
    assert(rune && rune->kind == RewardKind::Rune && rune->payload == 4 && rune->price == 80);
    const auto* cape = FindReward("vanilla.nei.rocs_cape");
    assert(cape && cape->payload == 0x9F); // Exact Cape, never a progressive Feather.
    assert(!FindReward("native.master_sword"));
    json live = NewFile(), disk = live;
    auto writer = [&](const json& draft) { disk = draft; return true; };
    Receipt receipt;
    assert(RewardStatus(live, *lantern) == 0);
    assert(!receipt.Begin(live, *lantern, false, [](const json&) { return false; }));
    assert(!receipt.Active() && !live["combo"]["vanilla"].contains("pending"));
    assert(receipt.Begin(live, *lantern, false, writer));
    assert(disk["oot"]["sections"]["base"]["data"]["rupees"] == 99);
    assert(disk["combo"]["vanilla"]["pending"][lantern->checkId]["price"] == 40);
    // Interruption leaves the old native inventory/money and an exact receipt
    // that may be offered again without a second progressive interpretation.
    receipt.Abort();
    live = disk;
    Receipt recovered;
    assert(recovered.Begin(live, *lantern, false, writer));
    auto granted = live["oot"];
    granted["sections"]["base"]["data"]["rupees"] = 59;
    granted["sections"]["nei"]["data"]["ownedItems"] = json::array({0xB5});
    assert(!recovered.Stage(1, granted));
    assert(recovered.Stage(0, granted));
    assert(!recovered.Commit(live, [](const json&) { return false; }));
    assert(disk["oot"]["sections"]["base"]["data"]["rupees"] == 99);
    assert(RewardStatus(disk, *lantern) == 0 && recovered.Active());
    assert(recovered.Commit(live, writer));
    assert(!recovered.Active() && RewardStatus(disk, *lantern) == 1);
    assert(disk["oot"]["sections"]["base"]["data"]["rupees"] == 59);
    assert(disk["combo"]["vanilla"]["pending"].empty());
    assert(!receipt.Begin(live, *lantern, false, writer));
    // Copy and MM cycle saves retain the container-owned completion marker.
    auto copied = disk;
    copied["slot"] = 2;
    copied["mm"]["newCycleSave"]["save"]["saveInfo"]["playerData"]["threeDayResetCount"] = 8;
    assert(RewardStatus(copied, *lantern) == 1);
    // Aliases already owned in either game satisfy a shop without any debit.
    const auto* arrows = FindReward("vanilla.nei.bomb_arrows");
    assert(arrows && receipt.Begin(live, *arrows, true, writer));
    assert(receipt.Stage(0, live["oot"]) && receipt.Commit(live, writer));
    assert(disk["combo"]["vanilla"]["collected"][arrows->checkId]["price"] == 0);
    assert(disk["oot"]["sections"]["base"]["data"]["rupees"] == 59);
    // Refuse corrupt metadata, native rando halves and ambiguous payloads.
    auto corrupt = NewFile();
    corrupt["combo"]["vanilla"]["pending"] = json::array();
    assert(!receipt.Begin(corrupt, *lantern, false, writer));
    corrupt = NewFile();
    assert(receipt.Begin(corrupt, *lantern, false, [](const json&) { return true; }));
    auto rando = corrupt["oot"]; rando["fileType"] = 1;
    assert(!receipt.Stage(0, rando));
    assert(!receipt.Commit(corrupt, writer));
    receipt.Abort();
    corrupt = NewFile(); corrupt["combo"]["mode"] = "randomizer";
    assert(RewardStatus(corrupt, *lantern) == -1);
    assert(!receipt.Begin(corrupt, *lantern, false, writer));
    std::cout << "PASS manifest identities, interrupted receipts, atomic native/check commit, retry, copy/reset, debit once and seeded isolation\n";
}
