#include "mm_fixture.h"

SaveContext gSaveContext;
ExtendedEquipmentState gExtEquipState;
PlayState* gPlayState;
bool fixtureRando, fixtureKeepExpress, fixtureVeto, fixtureAdvance, fixtureApproveGive;
bool fixtureRandoOptions[2];
int gFcCombo_SuppressRecord;
unsigned fixtureOffers, fixtureCsActions, fixtureCallbacks;
GetItemId fixtureLastOffer;
NeiSaveData* Nei_Save(void) { return &gSaveContext.save.shipSaveInfo.nei; }
unsigned char OotAssets_Available(void) { return 0; }
void ExtEquip_GivePendant(void) { Nei_Save()->pendantOwned = 1; }
// Equipment removal is an existing external boundary of the extracted migration.
void ExtEquip_RemoveItem(s16 equipType, u8 index) {
    if (equipType != EQUIP_TYPE_BOOTS || (index != 2 && index != 3)) abort();
    Nei_Save()->extEquipOwnedBits &= ~(1u << (EXT_EQUIP_OWNED_SHIFT + equipType * 3 + index - 1));
    if (gExtEquipState.currentExtBoots == index) {
        gExtEquipState.currentExtBoots = 0;
        Nei_Save()->extEquipBoots = 0;
    }
}
int GameInteractor_ShouldItemGive(u8 item) { (void)item; return fixtureApproveGive; }
void GameInteractor_ExecuteOnItemGive(u8 item) { (void)item; }
void ItemGrantAudit_Begin(const char* from, int item, int slot, int mode) {
    (void)from; (void)item; (void)slot; (void)mode;
}
void ItemGrantAudit_End(void) {}
void Interface_LoadItemIconImpl(PlayState* play, u8 btn) { (void)play; (void)btn; }
void Interface_Dpad_LoadItemIconImpl(PlayState* play, u8 btn) { (void)play; (void)btn; }
void Flags_SetWeekEventReg(u16 flag) {
    gSaveContext.save.saveInfo.weekEventReg[flag >> 8] |= flag & 0xFF;
}
bool Flags_GetRandoInf(RandoInf flag) { (void)flag; return false; }
void Flags_SetRandoInf(RandoInf flag) { (void)flag; }
void Rupees_ChangeBy(s16 count) { (void)count; }
u8 Message_GetState(MessageContext* message) { return message->fixtureState; }
int Message_ShouldAdvance(PlayState* play) { (void)play; return fixtureAdvance; }
s16 Animation_GetLastFrame(void* animation) { (void)animation; return 100; }
void SubS_ChangeAnimationByInfoS(SkelAnime* skel, AnimationInfo* info, s16 index) {
    (void)skel; (void)info; (void)index;
}
void Player_SetCsActionWithHaltedActors(PlayState* play, Actor* actor, s32 action) {
    (void)play; (void)actor; if (action != PLAYER_CSACTION_19) abort(); ++fixtureCsActions;
}
int Actor_HasParent(Actor* actor, PlayState* play) { (void)play; return actor->parent != NULL; }
void Actor_OfferGetItem(Actor* actor, PlayState* play, GetItemId item, f32 x, f32 y) {
    (void)actor; (void)play; if (x != 300.0f || y != 300.0f) abort();
    ++fixtureOffers; fixtureLastOffer = item;
}
void Message_StartTextbox(PlayState* play, u16 text, Actor* actor) { (void)play; (void)text; (void)actor; }
void func_80ADB0D8(EnSellnuts* actor, PlayState* play) { (void)actor; (void)play; }
void func_80ADBCE4(EnSellnuts* actor, PlayState* play) { (void)actor; (void)play; }
void func_80BEF450(EnAkindonuts* actor, PlayState* play) { (void)actor; (void)play; }
void func_80BEEFA8(EnAkindonuts* actor, PlayState* play) { (void)actor; (void)play; }
s32 func_80BED034(EnAkindonuts* actor) { (void)actor; return GI_MAGIC_BEANS; }

#include "mm_trade.inc"
#include "mm_native_functions.inc"
