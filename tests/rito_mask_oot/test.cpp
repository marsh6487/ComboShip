#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "soh/include/z64item.h"
#include "soh/mods/nei_save.h"
using u8 = uint8_t;
using s32 = int32_t;
#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
static NeiSaveData nei{};
extern "C" NeiSaveData* Nei_Save(void) { return &nei; }
static int vanilla, toggle = 1, randomizer;
#define IS_RANDO randomizer
static bool Combo_IsVanilla() { return vanilla; }
static int CVarGetInteger(const char*, int) { return toggle; }
static constexpr int RAND_INF_CHILD_TRADES_HAS_WEIRD_EGG = 100;
static bool randomizerFlags[128];
static bool Flags_GetRandomizerInf(int flag) { assert(flag>=100 && flag<=110); return randomizerFlags[flag]; }
struct CustomFormDef { const char* modelName; const char* cvar; uint8_t cvarDefault; };
struct ItemEquips { uint8_t buttonItems[8]; uint8_t cButtonSlots[7]; };
static struct { struct { uint8_t items[24]; } inventory; ItemEquips equips, childEquips, adultEquips; } gSaveContext;
struct NeiItem { uint8_t slot; };
static const NeiItem* Nei_FindByItem(uint16_t) { return nullptr; }
static uint8_t gItemSlots[52], gPage3MaskItems[24];
#define NEI_NO_SLOT ((uint8_t)0xFF)
enum { MM_PLAYER_FORM_HUMAN, MM_PLAYER_FORM_RITO, MM_PLAYER_FORM_OTHER };
enum { MMFORM_STATE_INACTIVE, MMFORM_STATE_ACTIVE, MMFORM_STATE_TRANSFORMING };
constexpr uint32_t PLAYER_STATE1_INPUT_DISABLED=1u<<29, PLAYER_STATE3_PAUSE_ACTION_FUNC=1u<<2;
struct Player { uint32_t stateFlags1{},stateFlags3{}; int mass=37,shadow=25,tunic=3; };
struct PlayState { Player* player; };
static struct { int currentForm,targetForm,state; int skeletonLoaded,initialized; void* savedAgeProperties; } gFormState{};
static int sPendingReactivateForm,sPendingReactivate,sPendingSoftReload,sFleetPendingForm=-1;
static PlayState* gPlayState;
#define GET_PLAYER(play) ((play)->player)
static int formRemovals,propertyRestores,equipRestores,extendedRestores;
static bool extendedBackup;
static const char* forcedModel;
static const char* O2rLoader_GetForcedName(){return forcedModel;}
static void O2rLoader_ClearForcedModel(){forcedModel=nullptr;}
static void MmForm_RestoreOotState(Player* p){assert(gFormState.savedAgeProperties);++propertyRestores;p->mass=50;p->shadow=60;p->tunic=1;gFormState.savedAgeProperties=nullptr;}
static void MmForm_RestoreEquips(PlayState*){++equipRestores;}
static void ExtEquip_RestoreFromTransform(){if(extendedBackup){++extendedRestores;extendedBackup=false;}}
static void MmForm_Reset(){assert(!extendedBackup);++formRemovals;gFormState.state=MMFORM_STATE_INACTIVE;gFormState.currentForm=MM_PLAYER_FORM_HUMAN;}

static int sEquipsSaved,discardedBackups,rootFrees,unpins,windForgets;
static bool departedActors;
static void MmForm_RitoWindClear(){++windForgets;departedActors=false;}
static void MmForm_RitoResetFlight(){assert(!departedActors);}
static void MmForm_RitoBowReset(){}
static void MmForm_FreeRootMotion(){++rootFrees;}
static void MmForm_ClearCachedDLs(){}
static void MmForm_UnpinFormResources(){++unpins;}
static void ExtEquip_ClearTransformBackup(){extendedBackup=false;++discardedBackups;}
#include "production.inc"
static void reset() {
    nei = {}; vanilla=0;toggle=1;randomizer=0;formRemovals=0;
    propertyRestores=equipRestores=extendedRestores=0;extendedBackup=false;forcedModel=nullptr;
    sEquipsSaved=discardedBackups=rootFrees=unpins=windForgets=0;departedActors=false;
    gFormState={};gPlayState=nullptr;sPendingReactivateForm=MM_PLAYER_FORM_HUMAN;
    sPendingReactivate=sPendingSoftReload=0;sFleetPendingForm=-1;
    memset(&gSaveContext, ITEM_NONE, sizeof(gSaveContext));
    memset(randomizerFlags, 0, sizeof(randomizerFlags));
}
int main() {
    static_assert(ITEM_RITO_MASK == 0xD2);
    static_assert(SLOT_TRADE_CHILD == 23);
    assert(ExtInv_GetItemSlot(ITEM_RITO_MASK) == SLOT_TRADE_CHILD);
    for(int mode=0;mode<3;++mode) {
        reset();vanilla=mode==1;randomizer=mode==2;
        assert(!RitoItem_FormEnabled());
        RitoItem_SyncCell();assert(!RitoMask_IsOwned());
        gSaveContext.inventory.items[SLOT_FARORES_WIND]=ITEM_FARORES_WIND;
        RitoItem_SyncCell();assert(!RitoMask_IsOwned());
        assert(gSaveContext.inventory.items[SLOT_FARORES_WIND]==ITEM_FARORES_WIND);
    }
    reset();SetRitoMaskOwned(true);
    assert(RitoMask_IsOwned() && RitoItem_FormEnabled());
    assert(gSaveContext.inventory.items[SLOT_TRADE_CHILD] == ITEM_RITO_MASK);
    assert(gSaveContext.inventory.items[SLOT_FARORES_WIND] == ITEM_NONE);
    toggle=0;assert(!RitoItem_FormEnabled());vanilla=1;assert(RitoItem_FormEnabled());
    SetRitoMaskOwned(false);assert(!RitoMask_IsOwned() && formRemovals==0);
    assert(gSaveContext.inventory.items[SLOT_TRADE_CHILD] == ITEM_NONE);
    for (int ownsSpell=0;ownsSpell<2;++ownsSpell) {
        reset();
        nei.ritoMaskFlags=ownsSpell ? RITO_FLAG_FARORES_OWNED : 0;
        gSaveContext.inventory.items[SLOT_FARORES_WIND]=ITEM_RITO_MASK;
        gSaveContext.inventory.items[SLOT_TRADE_CHILD]=ITEM_MASK_GORON;
        ItemEquips* equips[]={&gSaveContext.equips,&gSaveContext.childEquips,&gSaveContext.adultEquips};
        for (auto* equip:equips) {
            equip->buttonItems[1]=equip->buttonItems[5]=ITEM_RITO_MASK;
            equip->cButtonSlots[0]=equip->cButtonSlots[4]=SLOT_FARORES_WIND;
            equip->buttonItems[2]=ITEM_BOW;equip->cButtonSlots[1]=SLOT_BOW;
        }
        RitoMask_MigrateLegacy();
        assert(RitoMask_IsOwned());
        assert(gSaveContext.inventory.items[SLOT_FARORES_WIND] == (ownsSpell ? ITEM_FARORES_WIND : ITEM_NONE));
        assert(gSaveContext.inventory.items[SLOT_TRADE_CHILD] == ITEM_MASK_GORON);
        for (auto* equip:equips) {
            assert(equip->buttonItems[1]==ITEM_RITO_MASK && equip->cButtonSlots[0]==SLOT_TRADE_CHILD);
            assert(equip->buttonItems[5]==ITEM_RITO_MASK && equip->cButtonSlots[4]==SLOT_TRADE_CHILD);
            assert(equip->buttonItems[2]==ITEM_BOW && equip->cButtonSlots[1]==SLOT_BOW);
        }
        const auto flags=nei.ritoMaskFlags;
        RitoMask_MigrateLegacy();assert(nei.ritoMaskFlags==flags);
        gSaveContext.inventory.items[SLOT_TRADE_CHILD]=ITEM_RITO_MASK;
        RitoMask_Remove();
        assert(!RitoMask_IsOwned());
        assert(gSaveContext.inventory.items[SLOT_TRADE_CHILD]==ITEM_MASK_GORON);
        assert(gSaveContext.inventory.items[SLOT_FARORES_WIND] == (ownsSpell ? ITEM_FARORES_WIND : ITEM_NONE));
        for(auto* equip:equips) {
            assert(equip->buttonItems[1]==ITEM_NONE && equip->buttonItems[5]==ITEM_NONE);
            assert(equip->buttonItems[2]==ITEM_BOW && equip->cButtonSlots[1]==SLOT_BOW);
        }
    }
    reset();RitoMask_Grant();
    assert(OotMaskWheel_Neighbor(1,0)==ITEM_RITO_MASK);
    assert(OotMaskWheel_Neighbor(-1,0)==ITEM_RITO_MASK);
    // All eight original masks plus Rito stay reachable, in both directions.
    nei.ootMasksOwned=0xFF;
    uint16_t visited=0;
    for(int i=0;i<9;++i) {
        uint8_t next=OotMaskWheel_Neighbor(1,0);
        visited |= next==ITEM_RITO_MASK ? 0x100 : 1u<<(next-ITEM_MASK_KEATON);
        gSaveContext.inventory.items[SLOT_TRADE_CHILD]=next;
        RitoItem_SyncCell();
    }
    assert(visited==0x1FF && nei.ootMasksOwned==0xFF);
    for(int i=0;i<9;++i) {
        const uint8_t previous=OotMaskWheel_Neighbor(-1,0);
        const uint8_t current=gSaveContext.inventory.items[SLOT_TRADE_CHILD];
        gSaveContext.inventory.items[SLOT_TRADE_CHILD]=previous;
        assert(OotMaskWheel_Neighbor(1,0)==current);
    }
    reset();randomizer=1;RitoMask_Grant();
    randomizerFlags[RAND_INF_CHILD_TRADES_HAS_WEIRD_EGG+ITEM_MASK_TRUTH-ITEM_WEIRD_EGG]=true;
    assert(OotMaskWheel_Neighbor(1,0)==ITEM_MASK_TRUTH);
    assert(OotMaskWheel_Neighbor(-1,0)==ITEM_MASK_TRUTH);
    // The new item cannot unlock the eight vanilla quest masks by itself.
    randomizer=0;randomizerFlags[RAND_INF_CHILD_TRADES_HAS_WEIRD_EGG+ITEM_MASK_TRUTH-ITEM_WEIRD_EGG]=false;
    assert(OotMaskWheel_Neighbor(1,0)==ITEM_RITO_MASK);
    assert(OotMaskWheel_Neighbor(1,1)==ITEM_MASK_KEATON);
    // The actual form-removal callback must cover active and pre-flash removal.
    for (int applied=0;applied<2;++applied) {
        reset();RitoMask_Grant();Player player;PlayState play{&player};gPlayState=&play;
        player.stateFlags1=PLAYER_STATE1_INPUT_DISABLED;player.stateFlags3=PLAYER_STATE3_PAUSE_ACTION_FUNC;
        gFormState.currentForm=applied ? MM_PLAYER_FORM_RITO : MM_PLAYER_FORM_HUMAN;
        gFormState.targetForm=MM_PLAYER_FORM_RITO;
        gFormState.state=applied ? MMFORM_STATE_ACTIVE : MMFORM_STATE_TRANSFORMING;
        gFormState.savedAgeProperties=applied ? reinterpret_cast<void*>(1) : nullptr;
        extendedBackup=applied;forcedModel="rito";
        sPendingReactivate=sPendingSoftReload=1;sPendingReactivateForm=MM_PLAYER_FORM_RITO;sFleetPendingForm=MM_PLAYER_FORM_RITO;
        RitoMask_Remove();
        assert(!RitoMask_IsOwned() && formRemovals==1 && propertyRestores==applied);
        assert(equipRestores==1 && extendedRestores==applied && !extendedBackup && !forcedModel);
        assert(!player.stateFlags1 && !player.stateFlags3);
        assert(gFormState.currentForm==MM_PLAYER_FORM_HUMAN && gFormState.targetForm==MM_PLAYER_FORM_HUMAN);
        assert(!sPendingSoftReload && !sPendingReactivate && sFleetPendingForm==-1);
        if(!applied)assert(player.mass==37 && player.shadow==25 && player.tunic==3);
        RitoMask_Grant();assert(gFormState.state==MMFORM_STATE_INACTIVE); // regrant never revives Rito
    }
    reset();RitoMask_Grant();gFormState.currentForm=MM_PLAYER_FORM_OTHER;
    gFormState.state=MMFORM_STATE_ACTIVE;gFormState.targetForm=MM_PLAYER_FORM_RITO; // stale prior target
    sPendingReactivateForm=MM_PLAYER_FORM_OTHER;sPendingSoftReload=1;forcedModel="gerudo";
    RitoMask_Remove();assert(formRemovals==0 && sPendingSoftReload==1 && forcedModel);
    assert(gFormState.currentForm==MM_PLAYER_FORM_OTHER);
    // Scene/file arrival must drop the old form, never restore old-play state
    // or gear into the newly initialized native player/save.
    for(int preFlash=0;preFlash<2;++preFlash) {
        reset();Player player;PlayState play{&player};gPlayState=&play;
        gFormState.currentForm=preFlash ? MM_PLAYER_FORM_HUMAN : MM_PLAYER_FORM_RITO;
        gFormState.state=preFlash ? MMFORM_STATE_TRANSFORMING : MMFORM_STATE_ACTIVE;
        gFormState.targetForm=MM_PLAYER_FORM_RITO;gFormState.savedAgeProperties=reinterpret_cast<void*>(1);
        sPendingReactivateForm=gFormState.currentForm;sPendingSoftReload=1;sPendingReactivate=1;
        sEquipsSaved=1;extendedBackup=true;departedActors=true;forcedModel="rito";
        gSaveContext.equips.buttonItems[1]=ITEM_BOW;gSaveContext.equips.cButtonSlots[0]=SLOT_BOW;
        MmForm_DiscardUnownedRitoReload();
        assert(gFormState.state==MMFORM_STATE_INACTIVE && gFormState.currentForm==MM_PLAYER_FORM_HUMAN);
        assert(gFormState.initialized && !sPendingSoftReload && !sPendingReactivate && !sEquipsSaved);
        assert(discardedBackups==1 && rootFrees==1 && unpins==1 && windForgets==1 && !departedActors);
        assert(propertyRestores==0 && equipRestores==0 && extendedRestores==0 && formRemovals==0);
        assert(player.mass==37 && player.shadow==25 && player.tunic==3);
        assert(gSaveContext.equips.buttonItems[1]==ITEM_BOW && gSaveContext.equips.cButtonSlots[0]==SLOT_BOW);
    }
    reset();RitoMask_Grant();gFormState.currentForm=MM_PLAYER_FORM_RITO;
    gFormState.state=MMFORM_STATE_ACTIVE;sPendingReactivateForm=MM_PLAYER_FORM_RITO;sPendingSoftReload=1;
    MmForm_DiscardUnownedRitoReload();assert(sPendingSoftReload && gFormState.currentForm==MM_PLAYER_FORM_RITO && !rootFrees);
    puts("PASS Rito OoT ownership, editor grant/remove, all nine corner masks, no free grants, legacy spell and age/C/D-button migration");
}
