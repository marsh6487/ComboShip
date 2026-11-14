#include "mm_fixture.h"
#include <cstdio>
#include <cstring>
#include <initializer_list>

static unsigned checks, failed;
static void Check(bool value, const char* message, int detail = 0) {
    ++checks;
    if (!value) {
        ++failed;
        if (failed <= 36) std::fprintf(stderr, "FAIL MM %s (%d)\n", message, detail);
    }
}
static void Reset(PlayState& play) {
    std::memset(&gSaveContext, 0, sizeof(gSaveContext));
    std::memset(&saveContextCopy, 0, sizeof(saveContextCopy));
    std::memset(&gExtEquipState, 0, sizeof(gExtEquipState));
    std::memset(&play, 0, sizeof(play));
    std::memset(gSaveContext.save.saveInfo.inventory.items, ITEM_NONE, 48);
    std::memset(gSaveContext.save.saveInfo.equips.buttonItems, ITEM_NONE, 16);
    std::memset(gSaveContext.save.saveInfo.equips.cButtonSlots, SLOT_NONE, 16);
    std::memset(gSaveContext.save.shipSaveInfo.dpadEquips.dpadItems, ITEM_NONE, 16);
    std::memset(gSaveContext.save.shipSaveInfo.dpadEquips.dpadSlots, SLOT_NONE, 16);
    gSaveContext.save.playerForm = PLAYER_FORM_HUMAN;
    gPlayState = &play;
    fixtureRando = fixtureKeepExpress = fixtureVeto = fixtureAdvance = false;
    fixtureApproveGive = true;
    fixtureRandoOptions[0] = fixtureRandoOptions[1] = false;
    gFcCombo_SuppressRecord = 0;
    fixtureOffers = fixtureCsActions = fixtureCallbacks = 0;
    fixtureLastOffer = GI_NONE;
}
static void Own(u8 item, int index) {
    Nei_Save()->tradeAdultOwned |= 1u << index;
    gSaveContext.save.saveInfo.inventory.items[gItemSlots[item]] = item;
}
static void DeleteScript(Actor& actor, PlayState& play, u8 item) {
    MsgScriptCmdDeleteItem command = { 0, 0, item };
    u8* cursor = reinterpret_cast<u8*>(&command);
    s32 end = 0;
    MsgEvent_DeleteItem(&actor, &play, &cursor, nullptr, &end);
}
static void AcquisitionAndDeletion(PlayState& play) {
    Reset(play);
    Item_Give(&play, ITEM_LETTER_TO_KAFEI);
    Check(TradeAdult_IsOwnedIndex(17), "native grant joins trade wheel before pause");
    Inventory_DeleteItem(ITEM_LETTER_TO_KAFEI, SLOT_TRADE_COUPLE);
    Check(!TradeAdult_IsOwnedIndex(17), "native handoff retires just-acquired letter");
    Check(Nei_Save()->tradeAdultRevision[17] == 1, "native letter consumed revision");
    Item_Give(&play, ITEM_LETTER_TO_KAFEI);
    Check(TradeAdult_IsOwnedIndex(17) && Nei_Save()->tradeAdultRevision[17] == 2,
          "real native reacquisition restores letter");
    Inventory_DeleteItem(ITEM_LETTER_TO_KAFEI, SLOT_TRADE_COUPLE);
    gFcCombo_SuppressRecord = 1;
    Item_Give(&play, ITEM_LETTER_TO_KAFEI);
    Check(!TradeAdult_IsOwnedIndex(17) && Nei_Save()->tradeAdultRevision[17] == 3,
          "registry materialization is not reacquisition");
    gFcCombo_SuppressRecord = 0;
    fixtureApproveGive = false;
    Item_Give(&play, ITEM_LETTER_TO_KAFEI);
    Check(Nei_Save()->tradeAdultRevision[17] == 3, "vetoed grant remains retired");

    Reset(play);
    Own(ITEM_PENDANT_OF_MEMORIES, 19);
    Own(ITEM_LETTER_TO_KAFEI, 17); // Different item occupies Pendant's native home.
    gSaveContext.save.saveInfo.inventory.items[SLOT_TRADE_DEED] = ITEM_PENDANT_OF_MEMORIES;
    Nei_Save()->pendantEffectOff = 1;
    for (int btn = EQUIP_SLOT_C_LEFT; btn <= EQUIP_SLOT_C_RIGHT; ++btn) {
        BUTTON_ITEM_EQUIP(0, btn) = ITEM_PENDANT_OF_MEMORIES;
        C_SLOT_EQUIP(0, btn) = SLOT_TRADE_DEED;
    }
    for (int btn = EQUIP_SLOT_D_RIGHT; btn <= EQUIP_SLOT_D_UP; ++btn) {
        DPAD_BUTTON_ITEM_EQUIP(0, btn) = ITEM_PENDANT_OF_MEMORIES;
        DPAD_SLOT_EQUIP(0, btn) = SLOT_TRADE_COUPLE;
    }
    Inventory_DeleteItem(ITEM_PENDANT_OF_MEMORIES, SLOT_TRADE_COUPLE);
    Check(!TradeAdult_IsOwnedIndex(19) && TradeAdult_IsOwnedIndex(17), "delete retires only submitted Pendant");
    Check(gSaveContext.save.saveInfo.inventory.items[SLOT_TRADE_DEED] != ITEM_PENDANT_OF_MEMORIES,
          "visible duplicate of consumed Pendant cleared");
    Check(gSaveContext.save.saveInfo.inventory.items[SLOT_TRADE_COUPLE] == ITEM_LETTER_TO_KAFEI,
          "unsubmitted native-home letter preserved");
    Check(Nei_Save()->pendantOwned && Nei_Save()->pendantEffectOff, "permanent Pendant perk and toggle preserved");
    for (int btn = EQUIP_SLOT_C_LEFT; btn <= EQUIP_SLOT_C_RIGHT; ++btn)
        Check(BUTTON_ITEM_EQUIP(0, btn) == ITEM_NONE && C_SLOT_EQUIP(0, btn) == SLOT_NONE,
              "consumed C binding removed", btn);
    for (int btn = EQUIP_SLOT_D_RIGHT; btn <= EQUIP_SLOT_D_UP; ++btn)
        Check(DPAD_BUTTON_ITEM_EQUIP(0, btn) == ITEM_NONE && DPAD_SLOT_EQUIP(0, btn) == SLOT_NONE,
              "consumed D binding removed", btn);
    const auto revision = Nei_Save()->tradeAdultRevision[19];
    Inventory_DeleteItem(ITEM_PENDANT_OF_MEMORIES, SLOT_TRADE_COUPLE);
    Check(Nei_Save()->tradeAdultRevision[19] == revision &&
          gSaveContext.save.saveInfo.inventory.items[SLOT_TRADE_COUPLE] == ITEM_LETTER_TO_KAFEI,
          "repeated delete is idempotent and leaves next item");

    Reset(play);
    Own(ITEM_DEED_LAND, 12); Own(ITEM_DEED_SWAMP, 13);
    Inventory_ReplaceItem(&play, ITEM_DEED_SWAMP, ITEM_DEED_LAND);
    Check(TradeAdult_IsOwnedIndex(12) && TradeAdult_IsOwnedIndex(13), "wheel replacement is not consumption");

    for (s16 invalidItem : { s16(ITEM_LETTER_TO_KAFEI + 0x100), s16(ITEM_LETTER_TO_KAFEI - 0x100) }) {
        Reset(play); Own(ITEM_LETTER_TO_KAFEI, 17);
        gSaveContext.save.saveInfo.inventory.items[SLOT_BOMB] = ITEM_BOMB;
        BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_LEFT) = ITEM_LETTER_TO_KAFEI;
        C_SLOT_EQUIP(0, EQUIP_SLOT_C_LEFT) = SLOT_TRADE_COUPLE;
        Inventory_DeleteItem(invalidItem, SLOT_BOMB);
        Check(TradeAdult_IsOwnedIndex(17) && Nei_Save()->tradeAdultRevision[17] == 0,
              "out-of-range delete cannot consume low-byte trade alias", invalidItem);
        Check(gSaveContext.save.saveInfo.inventory.items[SLOT_TRADE_COUPLE] == ITEM_LETTER_TO_KAFEI &&
              BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_LEFT) == ITEM_LETTER_TO_KAFEI,
              "out-of-range delete leaves trade inventory and binding intact", invalidItem);
        Check(gSaveContext.save.saveInfo.inventory.items[SLOT_BOMB] == ITEM_NONE,
              "out-of-range delete keeps ordinary slot cleanup", invalidItem);
    }
}
static void OldBootsMigration(PlayState& play) {
    const u32 oldPendant = 1u << (EXT_EQUIP_OWNED_SHIFT + EQUIP_TYPE_BOOTS * 3 + 1);
    const u32 oldScale = 1u << (EXT_EQUIP_OWNED_SHIFT + EQUIP_TYPE_BOOTS * 3 + 2);
    const u32 otherEquipment = (1u << EXT_EQUIP_OWNED_SHIFT) |
                               (1u << (EXT_EQUIP_OWNED_SHIFT + EQUIP_TYPE_BOOTS * 3));
    for (u32 revision : { 0u, 1u }) {
        for (bool equippedOnly : { false, true }) {
            Reset(play);
            Nei_Save()->tradeAdultRevision[19] = revision;
            Nei_Save()->pendantEffectOff = 1;
            Nei_Save()->extEquipOwnedBits = otherEquipment | oldScale | (equippedOnly ? 0 : oldPendant);
            gExtEquipState.currentExtBoots = Nei_Save()->extEquipBoots = equippedOnly ? 2 : 1;
            Probe_OldBootsMigration();
            Check(Nei_Save()->tradeAdultRevision[19] == revision &&
                  !!TradeAdult_IsOwnedIndex(19) == (revision == 0),
                  "old Pendant migration observes history without reacquiring consumed copy", revision);
            Check((Nei_Save()->tradeAdultOwned & (1u << 19)) && Nei_Save()->pendantOwned &&
                  Nei_Save()->pendantEffectOff == 1,
                  "old Pendant migration preserves permanent perk and effect toggle", equippedOnly);
            Check(Nei_Save()->extEquipOwnedBits == otherEquipment && Nei_Save()->extBootsLayoutVersion == 1 &&
                  Nei_Save()->extEquipBoots == (equippedOnly ? 0 : 1) &&
                  gExtEquipState.currentExtBoots == Nei_Save()->extEquipBoots,
                  "boots migration retains existing version and equipment cleanup", equippedOnly);
            Probe_OldBootsMigration();
            Check(Nei_Save()->tradeAdultRevision[19] == revision && Nei_Save()->extEquipOwnedBits == otherEquipment,
                  "completed boots migration is idempotent", revision);
        }
    }
    Reset(play);
    Nei_Save()->extBootsLayoutVersion = 1;
    Nei_Save()->extEquipOwnedBits = oldPendant;
    gExtEquipState.currentExtBoots = Nei_Save()->extEquipBoots = 2;
    Probe_OldBootsMigration();
    Check(!Nei_Save()->tradeAdultOwned && !Nei_Save()->pendantOwned &&
          Nei_Save()->extEquipOwnedBits == oldPendant && Nei_Save()->extEquipBoots == 2,
          "current Climb Boots are not legacy Pendant history");
}
static void ScriptVetoes(PlayState& play) {
    for (bool rando : { false, true }) {
        Reset(play); fixtureRando = rando;
        Actor anju{};
        Own(ITEM_PENDANT_OF_MEMORIES, 19);
        fixtureVeto = true;
        DeleteScript(anju, play, ITEM_PENDANT_OF_MEMORIES);
        Check(TradeAdult_IsOwnedIndex(19), "final script veto preserves item", rando);
        fixtureVeto = false;
        DeleteScript(anju, play, ITEM_PENDANT_OF_MEMORIES);
        Check(!TradeAdult_IsOwnedIndex(19), "accepted script deletion works in both modes", rando);

        Reset(play); fixtureRando = rando; fixtureKeepExpress = true; play.sceneId = SCENE_POSTHOUSE;
        Actor postman{}; postman.id = ACTOR_EN_PM;
        Own(ITEM_LETTER_MAMA, 18);
        DeleteScript(postman, play, ITEM_LETTER_MAMA);
        Check(TradeAdult_IsOwnedIndex(18), "Keep Express first Postman trade preserved", rando);
        SET_WEEKEVENTREG(WEEKEVENTREG_57_04);
        DeleteScript(postman, play, ITEM_LETTER_MAMA);
        Check(!TradeAdult_IsOwnedIndex(18), "Keep Express second delivery consumes", rando);
        Check(CHECK_WEEKEVENTREG(WEEKEVENTREG_57_04), "recipient progression flag preserved", rando);

        Reset(play); fixtureRando = rando;
        Own(ITEM_DEED_SWAMP, 13);
        Check(GameInteractor_Should(VB_TOILET_HAND_TAKE_ITEM, true, &anju),
              "toilet hand accepted take is not blanket-vetoed", rando);
        if (GameInteractor_Should(VB_TOILET_HAND_TAKE_ITEM, true, &anju))
            EnBjt_TakeItem(PLAYER_IA_DEED_SWAMP);
        Check(!TradeAdult_IsOwnedIndex(13), "toilet paper removed", rando);
        Own(ITEM_ROOM_KEY, 16);
        EnBjt_TakeItem(PLAYER_IA_ROOM_KEY);
        Check(TradeAdult_IsOwnedIndex(16), "Room Key shown as wrong item stays owned", rando);
    }
    Reset(play); fixtureRando = true;
    Check(!GameInteractor_Should(VB_MSG_SCRIPT_DEL_ITEM, false, nullptr, ITEM_LETTER_TO_KAFEI),
          "rando hook never overrides earlier false decision");
    Check(!GameInteractor_Should(VB_GIVE_PENDANT_OF_MEMORIES_FROM_KAFEI, true),
          "unrelated rando reward veto retained");
}
static void Callback(EnAkindonuts*, PlayState*) { ++fixtureCallbacks; }
static void ScrubHandoffs(PlayState& play) {
    const u8 deeds[] = { ITEM_DEED_LAND, ITEM_DEED_SWAMP, ITEM_DEED_MOUNTAIN, ITEM_DEED_OCEAN };
    const GetItemId rewards[] = { GI_DEED_SWAMP, GI_DEED_MOUNTAIN, GI_DEED_OCEAN, GI_RUPEE_HUGE };
    for (bool rando : { false, true }) {
        Reset(play); fixtureRando = rando; Own(ITEM_MOONS_TEAR, 11);
        EnSellnuts scrub{}; scrub.actor.id = ACTOR_EN_SELLNUTS; scrub.actionFunc = func_80ADBAB8;
        play.msgCtx.fixtureState = TEXT_STATE_EVENT;
        func_80ADBAB8(&scrub, &play);
        Check(TradeAdult_IsOwnedIndex(11) && scrub.actionFunc == func_80ADBAB8,
              "Moon tear wait does not consume", rando);
        fixtureAdvance = true; play.msgCtx.fixtureState = TEXT_STATE_CHOICE;
        func_80ADBAB8(&scrub, &play);
        Check(TradeAdult_IsOwnedIndex(11), "uncommitted Moon tear dialogue does not consume", rando);
        play.msgCtx.fixtureState = TEXT_STATE_EVENT;
        func_80ADBAB8(&scrub, &play);
        Check(!TradeAdult_IsOwnedIndex(11) && scrub.actionFunc == func_80ADBBEC,
              "accepted Moon tear handoff consumes before reward intercept", rando);
        if (rando) {
            Probe_RandoActorUpdate(&scrub.actor);
            Item_Give(&play, ITEM_MASK_BUNNY); // Queue's unrelated shuffled reward.
            Check(scrub.actionFunc == func_80ADB0D8 && fixtureOffers == 0,
                  "rando Moon tear reward bypass retains completed handoff");
        } else {
            func_80ADBBEC(&scrub, &play);
            Check(fixtureLastOffer == GI_DEED_LAND, "native Moon tear reward unchanged");
        }
        Check(!TradeAdult_IsOwnedIndex(11), "reward does not revive Moon tear", rando);

        for (int i = 0; i < 4; ++i) {
            Reset(play); fixtureRando = rando; Own(deeds[i], 12 + i);
            EnAkindonuts actor{}; actor.actor.id = ACTOR_EN_AKINDONUTS; actor.actor.params = i;
            actor.unk_2DC = Callback; actor.actionFunc = func_80BEF20C;
            play.msgCtx.fixtureState = TEXT_STATE_EVENT; fixtureAdvance = true;
            func_80BEF20C(&actor, &play); // More dialogue, final accepted flag absent.
            Check(TradeAdult_IsOwnedIndex(12 + i) && fixtureCallbacks == 1,
                  "deed precommit dialogue retains item", i);
            actor.unk_32C = 1; fixtureAdvance = false;
            func_80BEF20C(&actor, &play);
            Check(TradeAdult_IsOwnedIndex(12 + i), "deed wait retains item", i);
            fixtureAdvance = true;
            func_80BEF20C(&actor, &play);
            Check(!TradeAdult_IsOwnedIndex(12 + i) && actor.actionFunc == func_80BEF360,
                  "accepted deed consumes before reward intercept", i);
            if (rando) {
                Probe_RandoActorUpdate(&actor.actor);
                Item_Give(&play, ITEM_MASK_BUNNY);
                Check(actor.actionFunc == func_80BEEFA8 && fixtureOffers == 0,
                      "rando deed reward bypass retains completed handoff", i);
            } else {
                func_80BEF360(&actor, &play);
                Check(fixtureLastOffer == rewards[i], "native deed reward unchanged", i);
            }
            Check(!TradeAdult_IsOwnedIndex(12 + i), "shuffled/native reward cannot restore old deed", i);
            const auto revision = Nei_Save()->tradeAdultRevision[12 + i];
            if (i == 3) {
                Own(ITEM_ROOM_KEY, 16);
                gSaveContext.save.saveInfo.inventory.items[SLOT_TRADE_DEED] = ITEM_ROOM_KEY;
                func_80BECFBC(&actor); func_80BECFBC(&actor);
                Check(Nei_Save()->tradeAdultRevision[15] == revision &&
                      gSaveContext.save.saveInfo.inventory.items[SLOT_TRADE_DEED] == ITEM_ROOM_KEY,
                      "repeated Ocean reward lookup leaves new selection intact");
            }
            Reset(play); fixtureRando = rando; Own(deeds[i], 12 + i);
            actor = {}; actor.actor.params = i; actor.unk_32C = 0x40; actor.unk_2DC = Callback;
            func_80BEF360(&actor, &play);
            Check(TradeAdult_IsOwnedIndex(12 + i) && fixtureLastOffer == GI_MAGIC_BEANS,
                  "scrub purchase never consumes deed", i);
        }
    }
}
static void CycleBoundaries(PlayState& play) {
    Reset(play);
    Own(ITEM_LETTER_TO_KAFEI, 17); Own(ITEM_ROOM_KEY, 16); Own(ITEM_DEED_LAND, 12);
    Nei_Save()->tradeAdultOwned |= 1u << 2; // OoT Cojiro, no native MM ID.
    Nei_Save()->tradeAdultRevision[2] = 1;
    Nei_Save()->tradeAdultRevision[17] = 1;
    Nei_Save()->tradeAdultRevision[12] = 1;
    saveContextCopy = gSaveContext;
    Probe_NativeTradeCycleReset(&play);
    Check(gSaveContext.save.saveInfo.inventory.items[SLOT_TRADE_DEED] == ITEM_NONE &&
          gSaveContext.save.saveInfo.inventory.items[SLOT_TRADE_KEY_MAMA] == ITEM_NONE &&
          gSaveContext.save.saveInfo.inventory.items[SLOT_TRADE_COUPLE] == ITEM_NONE,
          "native dummy-ID cycle calls still clear three native homes");
    Check(Nei_Save()->tradeAdultRevision[2] == 1 && Nei_Save()->tradeAdultRevision[12] == 1 &&
          Nei_Save()->tradeAdultRevision[17] == 1 && Nei_Save()->tradeAdultRevision[16] == 0,
          "native/Vanilla cycle neither revives consumed nor consumes held history");
    const u32 history = Nei_Save()->tradeAdultOwned;
    fixtureRando = true;
    Probe_RandoTradeCycleRestore();
    Check(Nei_Save()->tradeAdultRevision[12] == 2 && Nei_Save()->tradeAdultRevision[17] == 2,
          "rando next cycle explicitly restores consumed earned MM items");
    Check(Nei_Save()->tradeAdultRevision[2] == 1 && Nei_Save()->tradeAdultRevision[16] == 0 &&
          Nei_Save()->tradeAdultOwned == history && !TradeAdult_IsOwnedIndex(19),
          "rando reset preserves OoT retirement and does not grant absent items");
}
int main() {
    PlayState play{};
    AcquisitionAndDeletion(play);
    OldBootsMigration(play);
    ScriptVetoes(play);
    ScrubHandoffs(play);
    CycleBoundaries(play);
    std::printf("MM trade hooks: %u checks, %u failures\n", checks, failed);
    return failed ? 1 : 0;
}
