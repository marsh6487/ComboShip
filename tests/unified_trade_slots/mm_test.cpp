#include "z64.h"
#include "mods/nei_oot_compat.h"
#include "mods/extended_inventory.h"
#include <cassert>
#include <cstdio>
#include <cstring>
static NeiSaveData nei{};
extern "C" NeiSaveData* Nei_Save(){return &nei;}
static ExtendedInventoryState sExtInvState{};
extern "C" uint8_t RitoMask_IsOwned(){return nei.ritoMaskOwned;}
extern "C" uint8_t RitoMask_UsesCornerWheel(){return 1;}
unsigned char OotAssets_Available(){return 1;}
// PRODUCTION
int main(){
    // The fourteen OoT entries intentionally use MM's placeholder; identity is
    // their saved bit/cursor and icon, not an invented MM inventory byte.
    const uint8_t expected[23]={
        ITEM_NONE,ITEM_NONE,ITEM_NONE,ITEM_NONE,ITEM_NONE,ITEM_NONE,
        ITEM_NONE,ITEM_NONE,ITEM_NONE,ITEM_NONE,ITEM_NONE,
        ITEM_MOONS_TEAR,ITEM_DEED_LAND,ITEM_DEED_SWAMP,ITEM_DEED_MOUNTAIN,
        ITEM_DEED_OCEAN,ITEM_ROOM_KEY,ITEM_LETTER_TO_KAFEI,ITEM_LETTER_MAMA,
        ITEM_PENDANT_OF_MEMORIES,ITEM_NONE,ITEM_NONE,ITEM_NONE
    };
    assert(TradeAdult_Count()==23);
    assert(TradeAdult_IndexOfItem(ITEM_NONE)==-1);
    for(int index=0;index<23;++index) {
        nei={};nei.tradeAdultOwned=1u<<index;
        assert(TradeAdult_ItemId(index)==expected[index]);
        assert(TradeAdult_OwnedCount()==1 && TradeAdult_OwnedAt(0)==index);
        assert(TradeAdult_CursorIndex()==index);
        assert(TradeAdult_CellItem()==(expected[index]==ITEM_NONE?ITEM_TRADE_PLACEHOLDER:expected[index]));
        assert(OotMask_OwnedCount()==0 && OotMask_CellItem()==ITEM_NONE);
        TradeAdult_CursorStep(1);assert(TradeAdult_CursorIndex()==index);
        assert(nei.tradeAdultOwned==(1u<<index));
    }
    // The page exposes exactly one trade cell and a separate virtual mask cell.
    sExtInvState.currentPage=0;int tradeHomes=0;
    for(int visual=0;visual<24;++visual) {
        const int slot=ExtInv_GetInventorySlot(visual);
        assert(slot!=SLOT_TRADE_KEY_MAMA && slot!=SLOT_TRADE_COUPLE);
        if(slot==SLOT_TRADE_DEED) ++tradeHomes;
    }
    assert(tradeHomes==1 && ExtInv_GetInventorySlot(22)==SLOT_TRADE_DEED);
    assert(ExtInv_GetInventorySlot(23)==VSLOT_OOT_MASKS);
    nei={};nei.ootMasksOwned=0xFF;nei.ritoMaskOwned=1;
    assert(OotMask_OwnedCount()==9);
    for(int i=0;i<9;++i) assert(OotMask_OwnedAt(i)==i);
    assert(nei.tradeAdultOwned==0);
    puts("PASS MM: all23 saved trade indices, native/placeholder IDs, nine masks, one visible trade home; hidden native homes retained");
}
