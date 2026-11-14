static void Reset() {
    nei={};
    std::memset(&gSaveContext,ITEM_NONE,sizeof(gSaveContext));
    std::memset(randomizerFlags,0,sizeof(randomizerFlags));
    gCurrentItemCyclingSlot=-1; currentPage=0; slotWrites=0;randomizer=true;
}
static bool Combined() {
#ifdef COMBO_BUILD
    return true;
#else
    return false;
#endif
}
static void AssertNoMaskTrade() {
    for (int d : {-1,1}) {
        const auto item=OotMaskWheel_Neighbor(d,0);
        assert(item==ITEM_NONE || item==ITEM_RITO_MASK || (item>=ITEM_MASK_KEATON && item<=ITEM_MASK_TRUTH));
    }
}
int main() {
    PlayState play{};
    // A consumed copy can arrive while this peer still has old possession flags
    // and native selections. Pause/load normalization is observation, not pickup.
    for (int child = ITEM_CHICKEN; child <= ITEM_LETTER_ZELDA; ++child) {
        Reset();
        const int index = 20 + child - ITEM_WEIRD_EGG;
        nei.tradeAdultOwned = (1u << index) | (1u << 2);
        nei.tradeAdultRevision[index] = 1;
        randomizerFlags[RAND_INF_CHILD_TRADES_HAS_WEIRD_EGG + child - ITEM_WEIRD_EGG] = true;
        gSaveContext.inventory.items[SLOT_TRADE_CHILD] = child;
        gSaveContext.inventory.items[SLOT_TRADE_ADULT] = child;
        for (auto* equip : {&gSaveContext.equips, &gSaveContext.childEquips, &gSaveContext.adultEquips}) {
            for (int b = 1; b < 8; ++b) {
                equip->buttonItems[b] = child;
                equip->cButtonSlots[b - 1] = SLOT_TRADE_CHILD;
            }
        }
        for (int repeat = 0; repeat < 3; ++repeat) {
            RitoMask_MigrateLegacy();
            assert(nei.tradeAdultRevision[index] == 1 && !TradeAdult_IsOwnedItem(child));
            assert(gSaveContext.inventory.items[SLOT_TRADE_CHILD] != child);
            assert(gSaveContext.inventory.items[SLOT_TRADE_ADULT] != child);
            for (auto* equip : {&gSaveContext.equips, &gSaveContext.childEquips, &gSaveContext.adultEquips})
                for (int b = 1; b < 8; ++b) assert(equip->buttonItems[b] != child);
        }
        TradeAdult_GiveItem(child);
        RitoMask_MigrateLegacy();
        assert(nei.tradeAdultRevision[index] == 2 && TradeAdult_IsOwnedItem(child));
    }
    // Regression: one shared bit + legacy child selection must expose one trade cell.
    if (Combined()) {
        Reset(); nei.tradeAdultOwned=1u<<22;
        gSaveContext.inventory.items[SLOT_TRADE_CHILD]=ITEM_LETTER_ZELDA;
        Probe_ProductionTradeAndMaskPage0(&play);
        assert(gSaveContext.inventory.items[SLOT_TRADE_CHILD]==ITEM_NONE && "legacy Letter must leave the mask cell");
        assert(gSaveContext.inventory.items[SLOT_TRADE_ADULT]==ITEM_LETTER_ZELDA);
        assert(nei.tradeAdultOwned==(1u<<22)); AssertNoMaskTrade();
    }
    // All 23 existing bit indices retain their identity, one selection and stable ownership.
    const uint8_t expected[23]={
        ITEM_POCKET_EGG, ITEM_POCKET_CUCCO, ITEM_COJIRO, ITEM_ODD_MUSHROOM,
        ITEM_ODD_POTION, ITEM_SAW, ITEM_SWORD_BROKEN, ITEM_PRESCRIPTION,
        ITEM_FROG, ITEM_EYEDROPS, ITEM_CLAIM_CHECK,
        ITEM_MM_MOONS_TEAR, ITEM_MM_DEED_LAND, ITEM_MM_DEED_SWAMP,
        ITEM_MM_DEED_MOUNTAIN, ITEM_MM_DEED_OCEAN, ITEM_MM_ROOM_KEY,
        ITEM_MM_LETTER_KAFEI, ITEM_MM_SPECIAL_DELIVERY, ITEM_EXT_BOOTS_2,
        ITEM_WEIRD_EGG, ITEM_CHICKEN, ITEM_LETTER_ZELDA
    };
    assert(TradeAdult_Count()==23);
    for (int i=0;i<23;++i) {
        Reset();const auto item=expected[i];assert(TradeAdult_ItemId(i)==item && TradeAdult_IndexOfItem(item)==i);
        nei.tradeAdultOwned=1u<<i;
        Probe_ProductionTradeAndMaskPage0(&play);
        assert(gSaveContext.inventory.items[SLOT_TRADE_ADULT]==item);
        assert(gSaveContext.inventory.items[SLOT_TRADE_CHILD]==ITEM_NONE);
        assert(TradeAdult_OwnedCount()==1 && TradeAdult_OwnedAt(0)==i);
        auto before=nei;auto save=gSaveContext;
        for(int n=0;n<3;++n) Probe_ProductionTradeAndMaskPage0(&play);
        assert(std::memcmp(&before,&nei,sizeof(nei))==0 && std::memcmp(&save,&gSaveContext,sizeof(save))==0);
        if(Combined()) {
            AssertNoMaskTrade();
            gSaveContext.inventory.items[SLOT_TRADE_CHILD]=item;
            AssertNoMaskTrade(); // raw-current shortcut must reject every nonmask table row
        }
    }
    for(int child=ITEM_WEIRD_EGG;child<=ITEM_LETTER_ZELDA;++child) {
        for(int input=0;input<4;++input) {
            Reset();
            // raw child only, already duplicated, flag only, and raw+bit.
            if(input!=2) gSaveContext.inventory.items[SLOT_TRADE_CHILD]=child;
            if(input==1) gSaveContext.inventory.items[SLOT_TRADE_ADULT]=child;
            if(input==2) randomizerFlags[RAND_INF_CHILD_TRADES_HAS_WEIRD_EGG+child-ITEM_WEIRD_EGG]=true;
            if(input==3) nei.tradeAdultOwned=1u<<(20+child-ITEM_WEIRD_EGG);
            if(Combined()) {
                nei.comboTriforce=17;nei.comboObtained[73]=3;nei.comboObtainedFc[100]=4;
                const auto flags=randomizerFlags[RAND_INF_CHILD_TRADES_HAS_WEIRD_EGG+child-ITEM_WEIRD_EGG];
                RitoMask_MigrateLegacy();
                assert(nei.comboTriforce==17 && nei.comboObtained[73]==3 && nei.comboObtainedFc[100]==4);
                assert(randomizerFlags[RAND_INF_CHILD_TRADES_HAS_WEIRD_EGG+child-ITEM_WEIRD_EGG]==flags);
                assert(gSaveContext.inventory.items[SLOT_TRADE_CHILD]==ITEM_NONE);
                assert(gSaveContext.inventory.items[SLOT_TRADE_ADULT]==child);
                assert(nei.tradeAdultOwned==(1u<<(20+child-ITEM_WEIRD_EGG)));
                AssertNoMaskTrade();
            }
        }
        // Preserve the selected adult item and remap only nonmask C-slot bindings in every age set.
        Reset();gSaveContext.inventory.items[SLOT_TRADE_CHILD]=child;
        gSaveContext.inventory.items[SLOT_TRADE_ADULT]=ITEM_COJIRO;
        for(auto* equip : {&gSaveContext.equips,&gSaveContext.childEquips,&gSaveContext.adultEquips}) {
            equip->buttonItems[1]=child;equip->cButtonSlots[0]=SLOT_TRADE_CHILD;
            equip->buttonItems[2]=ITEM_MASK_KEATON;equip->cButtonSlots[1]=SLOT_TRADE_CHILD;
            equip->buttonItems[3]=ITEM_BOW;equip->cButtonSlots[2]=SLOT_BOW;
            for(int b=4;b<8;++b) {equip->buttonItems[b]=child;equip->cButtonSlots[b-1]=SLOT_TRADE_CHILD;}
        }
        RitoMask_MigrateLegacy();
        assert(gSaveContext.inventory.items[SLOT_TRADE_ADULT]==ITEM_COJIRO);
        for(auto* equip : {&gSaveContext.equips,&gSaveContext.childEquips,&gSaveContext.adultEquips}) {
            assert(equip->buttonItems[1]==child && equip->cButtonSlots[0]==(Combined()?SLOT_TRADE_ADULT:SLOT_TRADE_CHILD));
            assert(equip->buttonItems[2]==ITEM_MASK_KEATON && equip->cButtonSlots[1]==SLOT_TRADE_CHILD);
            assert(equip->buttonItems[3]==ITEM_BOW && equip->cButtonSlots[2]==SLOT_BOW);
            for(int b=4;b<8;++b) assert(equip->buttonItems[b]==child && equip->cButtonSlots[b-1]==(Combined()?SLOT_TRADE_ADULT:SLOT_TRADE_CHILD));
        }
        // Native nonrandomizer and solo normalization leave raw child items where native code expects them.
        Reset();randomizer=false;gSaveContext.inventory.items[SLOT_TRADE_CHILD]=child;
        RitoMask_MigrateLegacy();assert(gSaveContext.inventory.items[SLOT_TRADE_CHILD]==child);
        assert(gSaveContext.inventory.items[SLOT_TRADE_ADULT]==ITEM_NONE && nei.tradeAdultOwned==0);
        Reset();gSaveContext.inventory.items[SLOT_TRADE_CHILD]=child;
        RitoMask_MigrateLegacy();if(!Combined()) assert(gSaveContext.inventory.items[SLOT_TRADE_CHILD]==child);
    }
    if(Combined()) {
        Reset();gSaveContext.inventory.items[SLOT_TRADE_CHILD]=ITEM_LETTER_ZELDA;
        nei.ootMasksOwned=1u<<(ITEM_MASK_SKULL-ITEM_MASK_KEATON);
        RitoMask_MigrateLegacy();assert(gSaveContext.inventory.items[SLOT_TRADE_CHILD]==ITEM_MASK_SKULL);
        assert(nei.ootMasksOwned==(1u<<(ITEM_MASK_SKULL-ITEM_MASK_KEATON)));
        Reset();gSaveContext.inventory.items[SLOT_TRADE_CHILD]=ITEM_LETTER_ZELDA;currentPage=1;
        auto before=nei;auto save=gSaveContext;Probe_ProductionTradeAndMaskPage0(&play);
        assert(std::memcmp(&before,&nei,sizeof(nei))==0 && std::memcmp(&save,&gSaveContext,sizeof(save))==0);
    }
    // Eight real masks + Rito remain selectable, never become nonmask trade ownership.
    for(int selected=ITEM_MASK_KEATON;selected<=ITEM_MASK_TRUTH;++selected) {
        Reset();nei.ootMasksOwned=0xFF;nei.ritoMaskFlags=RITO_FLAG_MASK_OWNED;
        gSaveContext.inventory.items[SLOT_TRADE_CHILD]=selected;
        RitoMask_MigrateLegacy();assert(gSaveContext.inventory.items[SLOT_TRADE_CHILD]==selected);
        bool seen[9]{};
        for(int n=0;n<9;++n) {
            auto item=OotMaskWheel_Neighbor(1,0);
            assert(item==ITEM_RITO_MASK || (item>=ITEM_MASK_KEATON && item<=ITEM_MASK_TRUTH));
            int idx=item==ITEM_RITO_MASK?8:item-ITEM_MASK_KEATON;assert(!seen[idx]);seen[idx]=true;
            gSaveContext.inventory.items[SLOT_TRADE_CHILD]=item;
        }
        assert(nei.ootMasksOwned==0xFF && nei.ritoMaskFlags==RITO_FLAG_MASK_OWNED && nei.tradeAdultOwned==0);
    }
    // Native replacement scans actual inventory and updates the remapped live C item.
    if(Combined()) {
        Reset();gSaveContext.inventory.items[SLOT_TRADE_CHILD]=ITEM_WEIRD_EGG;
        gSaveContext.equips.buttonItems[1]=ITEM_WEIRD_EGG;gSaveContext.equips.cButtonSlots[0]=SLOT_TRADE_CHILD;
        RitoMask_MigrateLegacy();assert(Inventory_ReplaceItem(&play,ITEM_WEIRD_EGG,ITEM_CHICKEN));
        assert(gSaveContext.inventory.items[SLOT_TRADE_ADULT]==ITEM_CHICKEN);
        assert(gSaveContext.equips.buttonItems[1]==ITEM_CHICKEN && gSaveContext.equips.cButtonSlots[0]==SLOT_TRADE_ADULT);
    }
    std::puts(Combined()?"PASS Combo randomizer: 23 trades, legacy normalization, 9 masks, equips, native replacement":"PASS solo and native nonrandomizer controls");
}
