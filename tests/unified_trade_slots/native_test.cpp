#include "z64.h"
#include "macros.h"
#include "mods/nei_save.h"
#include "mods/extended_inventory.h"
#include "FleetShipCombo/FleetComboIds.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
SaveContext gSaveContext{};
static NeiSaveData nei{};
extern "C" NeiSaveData* Nei_Save(){return &nei;}
#undef IS_RANDO
static bool randomizer=true;
#define IS_RANDO randomizer
static bool randomizerFlags[RAND_INF_MAX]{};
static bool Flags_GetRandomizerInf(RandomizerInf flag){return randomizerFlags[flag];}
static void Flags_UnsetRandomizerInf(RandomizerInf flag){randomizerFlags[flag]=false;}
static void Flags_SetRandomizerInf(RandomizerInf flag){randomizerFlags[flag]=true;}
static void Interface_LoadItemIcon1(PlayState*,int){}
#define osSyncPrintf(...) ((void)0)
static void Flags_UnsetInfTable(int){}
static void Flags_SetInfTable(int){}
static void ExtButton_SaveAgeItems(int){}
static void ExtButton_LoadAgeItems(int){}
static int Randomizer_GetSettingValue(int){return 0;}
#define VB_SET_BUTTON_ITEM_FROM_C_BUTTON_SLOT 1
static bool GameInteractor_Should(int,bool value,int){return value;}
static u16 gEquipMasks[]={0xF,0xF0,0xF00,0xF000};
static u16 gEquipNegMasks[]={0xFFF0,0xFF0F,0xF0FF,0x0FFF};
static u16 gEquipShifts[]={0,4,8,12};
static u32 gBitFlags[32];
const uint8_t gPage3MaskItems[24]{};
extern "C" const NeiItem* Nei_FindByItem(int32_t){return nullptr;}
static void HealBogusOwnedItems(){}
static void FoldExtEquipmentIntoRegistry(NeiSaveData*){}
// PRODUCTION
int main(){
    // Hatching is a genuine new bird after an earlier copy was turned in.
    // Native and rando paths must grant it independently of pause migration.
    for (bool rando : {false, true}) {
        for (bool pocket : {false, true}) {
            nei = {}; gSaveContext = {}; randomizer = rando;
            memset(randomizerFlags, 0, sizeof(randomizerFlags));
            memset(gSaveContext.inventory.items, ITEM_NONE, sizeof(gSaveContext.inventory.items));
            gSaveContext.linkAge = LINK_AGE_CHILD;
            const uint8_t egg = pocket ? ITEM_POCKET_EGG : ITEM_WEIRD_EGG;
            const uint8_t bird = pocket ? ITEM_POCKET_CUCCO : ITEM_CHICKEN;
            const int index = pocket ? 1 : 21;
            nei.tradeAdultOwned = 1u << index;
            nei.tradeAdultRevision[index] = 1;
            gSaveContext.inventory.items[pocket ? SLOT_TRADE_ADULT : SLOT_TRADE_CHILD] = egg;
            randomizerFlags[pocket ? RAND_INF_ADULT_TRADES_HAS_POCKET_EGG : RAND_INF_CHILD_TRADES_HAS_WEIRD_EGG] = true;
            assert(pocket ? Inventory_HatchPocketCucco(nullptr) : Inventory_HatchWeirdEgg(nullptr));
            assert(nei.tradeAdultRevision[index] == 2 && TradeAdult_IsOwnedItem(bird));
        }
    }
    randomizer = true;
    static_assert(sizeof(gSaveContext.equips.buttonItems)==8);
    static_assert(sizeof(gSaveContext.equips.cButtonSlots)==7);
    for(int age : {LINK_AGE_CHILD,LINK_AGE_ADULT}) {
        nei={};gSaveContext={};
        memset(gSaveContext.inventory.items,ITEM_NONE,sizeof(gSaveContext.inventory.items));
        gSaveContext.linkAge=age;
        gSaveContext.inventory.items[SLOT_TRADE_ADULT]=ITEM_COJIRO;
        nei.tradeAdultOwned=(1u<<2)|(1u<<22);
        for(auto* equip : {&gSaveContext.equips,&gSaveContext.childEquips,&gSaveContext.adultEquips}) {
            memset(equip->buttonItems,ITEM_NONE,sizeof(equip->buttonItems));
            memset(equip->cButtonSlots,SLOT_NONE,sizeof(equip->cButtonSlots));
            equip->buttonItems[0]=ITEM_SWORD_KOKIRI;equip->equipment=1;
            for(int i=1;i<8;++i) {equip->buttonItems[i]=ITEM_LETTER_ZELDA;equip->cButtonSlots[i-1]=SLOT_TRADE_ADULT;}
        }
        Rando_Inventory_SwapAgeEquipment();
        for(int i=1;i<8;++i) {
#ifdef COMBO_BUILD
            assert(gSaveContext.equips.buttonItems[i]==ITEM_LETTER_ZELDA && "age restore must preserve independently equipped owned child trade");
#else
            assert(gSaveContext.equips.buttonItems[i]==ITEM_COJIRO);
#endif
            assert(gSaveContext.equips.cButtonSlots[i-1]==SLOT_TRADE_ADULT);
        }
        assert(gSaveContext.inventory.items[SLOT_TRADE_ADULT]==ITEM_COJIRO);
    }
#ifdef COMBO_BUILD
    nei={};gSaveContext={};std::memset(randomizerFlags,0,sizeof(randomizerFlags));
    memset(gSaveContext.inventory.items,ITEM_NONE,sizeof(gSaveContext.inventory.items));
    gSaveContext.linkAge=LINK_AGE_CHILD;
    gSaveContext.inventory.items[SLOT_TRADE_ADULT]=ITEM_COJIRO;
    gSaveContext.inventory.items[SLOT_TRADE_CHILD]=ITEM_WEIRD_EGG;
    randomizerFlags[RAND_INF_CHILD_TRADES_HAS_WEIRD_EGG]=true;
    for(auto* equip : {&gSaveContext.equips,&gSaveContext.childEquips,&gSaveContext.adultEquips}) {
        equip->buttonItems[0]=ITEM_SWORD_KOKIRI;equip->equipment=1;
        for(int b=1;b<8;++b) {equip->buttonItems[b]=ITEM_WEIRD_EGG;equip->cButtonSlots[b-1]=SLOT_TRADE_CHILD;}
    }
    RitoMask_MigrateLegacy();
    assert(gSaveContext.inventory.items[SLOT_TRADE_CHILD]==ITEM_NONE);
    assert(Inventory_HatchWeirdEgg(nullptr));
    assert(gSaveContext.equips.buttonItems[1]==ITEM_CHICKEN && "native hatch must progress independently bound migrated Egg");
    assert(gSaveContext.equips.cButtonSlots[0]==SLOT_TRADE_ADULT);
    assert(gSaveContext.inventory.items[SLOT_TRADE_ADULT]==ITEM_COJIRO);
    assert(!randomizerFlags[RAND_INF_CHILD_TRADES_HAS_WEIRD_EGG] && randomizerFlags[RAND_INF_CHILD_TRADES_HAS_CHICKEN]);
    assert(TradeAdult_IsOwnedItem(ITEM_CHICKEN) && "hatch must register actual newly hatched Cucco before immediate age restore");
    for(auto* equip : {&gSaveContext.childEquips,&gSaveContext.adultEquips})
        for(int b=1;b<8;++b) assert(equip->buttonItems[b]==ITEM_CHICKEN && "hatch must progress migrated saved age bindings");
    for(int age : {LINK_AGE_CHILD,LINK_AGE_ADULT}) {
        gSaveContext.linkAge=age;Rando_Inventory_SwapAgeEquipment();
        for(int b=1;b<8;++b) assert(gSaveContext.equips.buttonItems[b]==ITEM_CHICKEN && "hatch then age switch must retain Cucco with other trade selected");
        assert(gSaveContext.inventory.items[SLOT_TRADE_ADULT]==ITEM_COJIRO);
    }
#endif
    // Item-to-slot routing follows the normalized trade cell only in intended mode.
    for(bool mode : {false,true}) {
        randomizer=mode;
        for(int item=ITEM_WEIRD_EGG;item<=ITEM_LETTER_ZELDA;++item) {
#ifdef COMBO_BUILD
            assert(ExtInv_GetItemSlot(item)==(mode?SLOT_TRADE_ADULT:SLOT_TRADE_CHILD));
#else
            assert(ExtInv_GetItemSlot(item)==SLOT_TRADE_CHILD);
#endif
        }
        assert(ExtInv_GetItemSlot(ITEM_MASK_KEATON)==SLOT_TRADE_CHILD);
        assert(ExtInv_GetItemSlot(ITEM_POCKET_EGG)==SLOT_TRADE_ADULT);
        assert(ExtInv_GetItemSlot(ITEM_RITO_MASK)==SLOT_TRADE_CHILD);
        // Registry sees native child in all modes; extra normalized evidence only in repaired mode.
        for(int input=0;input<3;++input) {
            nei={};gSaveContext={};
            memset(gSaveContext.inventory.items,ITEM_NONE,sizeof(gSaveContext.inventory.items));
            if(input==0) gSaveContext.inventory.items[SLOT_TRADE_CHILD]=ITEM_LETTER_ZELDA;
            if(input==1) gSaveContext.inventory.items[SLOT_TRADE_ADULT]=ITEM_LETTER_ZELDA;
            if(input==2) nei.tradeAdultOwned=1u<<22;
            FoldNativesIntoRegistry();
#ifdef COMBO_BUILD
            assert(nei.comboObtained[FC_OOT_TRADE_WEIRD_EGG + 2]==(input==0 || mode));
#else
            assert(nei.comboObtained[FC_OOT_TRADE_WEIRD_EGG + 2]==(input==0));
#endif
            assert(nei.comboObtainedFc[0]==0 && nei.ootMasksOwned==0);
        }
    }
    puts("PASS native ABI: both real age restore branches and seven equipped trade buttons");
}
