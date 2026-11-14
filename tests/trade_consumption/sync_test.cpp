#include "combo/TradeItemSync.h"
#include <cassert>
#include <cstdio>
#include <cstring>

struct State {
    uint32_t tradeAdultOwned = 0;
    uint32_t tradeAdultRevision[23]{};
    uint8_t comboObtainedFc[512]{};
    uint8_t comboAppliedFc[512]{};
    uint8_t pendantOwned = 0;
    uint8_t pendantEffectOff = 1;
};

int main() {
    for (unsigned i = 0; i < 23; ++i) {
        State source{}, peer{};
        source.tradeAdultOwned = peer.tradeAdultOwned = 1u << i;
        source.tradeAdultRevision[i] = 1;
        nlohmann::json wire;
        ComboTrade::WriteShared(source, wire);
        ComboTrade::ReadShared(peer, wire);
        assert(peer.tradeAdultRevision[i] == 1);
        assert(peer.tradeAdultOwned == (1u << i));
        // A legacy save or stale peer can repeat its ownership and revision0.
        State stale{};
        stale.tradeAdultOwned = 1u << i;
        ComboTrade::WriteShared(stale, wire);
        ComboTrade::ReadShared(peer, wire);
        assert(peer.tradeAdultRevision[i] == 1);
        // A real newer pickup crosses once and cannot be undone by an older
        // consume packet, duplicate packets, or an out-of-order full resync.
        source.tradeAdultRevision[i] = 2;
        ComboTrade::WriteShared(source, wire);
        ComboTrade::ReadShared(peer, wire);
        ComboTrade::ReadShared(peer, wire);
        assert(peer.tradeAdultRevision[i] == 2);
        source.tradeAdultRevision[i] = 1;
        ComboTrade::WriteShared(source, wire);
        ComboTrade::ReadShared(peer, wire);
        assert(peer.tradeAdultRevision[i] == 2);
        source.tradeAdultRevision[i] = 3;
        ComboTrade::WriteShared(source, wire);
        ComboTrade::ReadShared(peer, wire);
        assert(peer.tradeAdultRevision[i] == 3);
        ComboTrade::ReadShared(peer, {{"tradeAdultOwned", 1u << i}});
        assert(peer.tradeAdultRevision[i] == 3);
        assert(peer.pendantEffectOff == 1);
        if (i == 19) assert(peer.pendantOwned == 1);
    }
    // Sparse old transport, truncated save fields and unrelated/invalid leaves
    // must not reset an item or abort the rest of a partial shared update.
    State state{};
    state.tradeAdultRevision[2] = 5;
    ComboTrade::ReadShared(state, {{"tradeAdultOwned", nullptr},
                                  {"tradeAdultRevision", {nullptr, 3, 1, -1, "x", 4294967296ULL}}});
    assert(state.tradeAdultRevision[1] == 3 && state.tradeAdultRevision[2] == 5);
    assert(state.tradeAdultRevision[3] == 0 && state.tradeAdultRevision[4] == 0 && state.tradeAdultRevision[5] == 0);
    ComboTrade::ReadShared(state, {{"tradeAdultRevision", nullptr}});
    ComboTrade::ReadShared(state, {{"tradeAdultRevision", nlohmann::json::object()}});
    assert(state.tradeAdultRevision[2] == 5);

    const struct { int fc; unsigned index; } trades[] = {
        {FCI_OOT_TRADE_POCKET_EGG,0},{FCI_OOT_TRADE_COJIRO,2},{FCI_OOT_TRADE_ODD_MUSHROOM,3},
        {FCI_OOT_TRADE_ODD_POTION,4},{FCI_OOT_TRADE_POACHERS_SAW,5},{FCI_OOT_TRADE_BROKEN_SWORD,6},
        {FCI_OOT_TRADE_PRESCRIPTION,7},{FCI_OOT_TRADE_EYEBALL_FROG,8},{FCI_OOT_TRADE_EYEDROPS,9},
        {FCI_OOT_TRADE_CLAIM_CHECK,10},{FCI_MM_MOONS_TEAR,11},{FCI_MM_DEED_LAND,12},
        {FCI_MM_DEED_SWAMP,13},{FCI_MM_DEED_MOUNTAIN,14},{FCI_MM_DEED_OCEAN,15},
        {FCI_MM_ROOM_KEY,16},{FCI_MM_LETTER_TO_KAFEI,17},{FCI_MM_LETTER_TO_MAMA,18},
        {FCI_PENDANT_OF_MEMORIES,19},{FCI_OOT_TRADE_WEIRD_EGG,20},{FCI_OOT_TRADE_ZELDAS_LETTER,22},
    };
    for (const auto& trade : trades) {
        state = State{};
        state.tradeAdultRevision[trade.index] = 1;
        state.comboObtainedFc[trade.fc] = 7;
        assert(ComboTrade::AcknowledgeConsumedGrant(state, trade.fc));
        assert(state.comboAppliedFc[trade.fc] == 7);
        assert(state.comboObtainedFc[trade.fc] == 7 && state.tradeAdultRevision[trade.index] == 1);
        state.tradeAdultRevision[trade.index] = 2;
        assert(!ComboTrade::AcknowledgeConsumedGrant(state, trade.fc));
    }
    state.comboObtainedFc[FCI_KOKIRI_SWORD] = 2;
    assert(!ComboTrade::AcknowledgeConsumedGrant(state, FCI_KOKIRI_SWORD));
    assert(state.comboAppliedFc[FCI_KOKIRI_SWORD] == 0);
    assert(!ComboTrade::AcknowledgeConsumedGrant(state, -1));
    assert(!ComboTrade::AcknowledgeConsumedGrant(state, FCI_MAX));
    puts("PASS trade sync: 23 item round trips, stale/partial messages, reacquisition, 21 real FC mappings");
}
