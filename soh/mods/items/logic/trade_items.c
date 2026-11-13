/**
 * trade_items.c - MM adult trade-quest items for the SLOT_TRADE_ADULT 2D-grid wheel (Skijer's NEI).
 *
 * The adult trade slot becomes a grid selector over every adult trade item the player owns: OoT's
 * (Pocket Egg..Claim Check) plus the 9 Majora's Mask ones. The bitmask records earned history;
 * tradeAdultRevision separately records whether the current copy is available or consumed.
 *
 * The Pendant of Memories also grants a permanent combat perk. Its separate pendantOwned flag
 * survives handing away the trade copy; the old Ext Boots 2 ownership bit belongs to Climb Boots.
 *
 * Included by custom_items.c (unity build -> z_player.c). Helpers are non-static so the kaleido grid
 * and the grant menu can call them via extern prototypes.
 */

#include "mods/nei_save.h"
#include "mods/extended_equipment.h" // ExtEquip_GiveItem/HasItem, ITEM_EXT_BOOTS_2

#define TRADE_ADULT_COUNT 23
#define TRADE_ADULT_PENDANT 19 // Pendant of Memories (== ITEM_EXT_BOOTS_2)

// NEI trade index -> inventory item id. Order MUST match the tradeAdultOwned bit layout (nei_save.h).
// APPEND ONLY — the index is the save bit, so reordering invalidates existing saves.
static const u8 sTradeAdultItems[TRADE_ADULT_COUNT] = {
    ITEM_POCKET_EGG, ITEM_POCKET_CUCCO, ITEM_COJIRO, ITEM_ODD_MUSHROOM,               // 0-3  (OoT adult)
    ITEM_ODD_POTION, ITEM_SAW, ITEM_SWORD_BROKEN, ITEM_PRESCRIPTION,                  // 4-7  (OoT adult)
    ITEM_FROG, ITEM_EYEDROPS, ITEM_CLAIM_CHECK,                                       // 8-10 (OoT adult)
    ITEM_MM_MOONS_TEAR,                                                               // 11
    ITEM_MM_DEED_LAND, ITEM_MM_DEED_SWAMP, ITEM_MM_DEED_MOUNTAIN, ITEM_MM_DEED_OCEAN, // 12-15
    ITEM_MM_ROOM_KEY, ITEM_MM_LETTER_KAFEI, ITEM_MM_SPECIAL_DELIVERY,                 // 16-18
    ITEM_EXT_BOOTS_2,                                                                 // 19 Pendant of Memories
    // OoT child trade chain — the non-mask half of SLOT_TRADE_CHILD. Moved here so the unified wheel
    // holds EVERY non-mask trade item and SLOT_TRADE_CHILD is masks-only. Appended (not sorted into
    // the OoT block) to keep the save bit layout stable for in-flight saves. Skijer's NEI
    ITEM_WEIRD_EGG, ITEM_CHICKEN, ITEM_LETTER_ZELDA, // 20-22 (OoT child)
};

s32 TradeAdult_Count(void) {
    return TRADE_ADULT_COUNT;
}

u8 TradeAdult_ItemId(s32 index) {
    if (index < 0 || index >= TRADE_ADULT_COUNT) {
        return ITEM_NONE;
    }
    return sTradeAdultItems[index];
}

s32 TradeAdult_IndexOfItem(u8 item) {
    // ITEM_NONE never identifies a trade item (keeps the two builds' behaviour identical — the MM
    // build aliases the OoT trade ids to ITEM_NONE, where an unguarded scan matched entry 0).
    if (item == ITEM_NONE) {
        return -1;
    }
    for (s32 i = 0; i < TRADE_ADULT_COUNT; i++) {
        if (sTradeAdultItems[i] == item) {
            return i;
        }
    }
    return -1;
}

u8 TradeAdult_IsConsumedIndex(s32 index) {
    if (index < 0 || index >= TRADE_ADULT_COUNT) {
        return 0;
    }
    return (Nei_Save()->tradeAdultRevision[index] & 1u) != 0;
}

u8 TradeAdult_IsOwnedIndex(s32 index) {
    if (index < 0 || index >= TRADE_ADULT_COUNT) {
        return 0;
    }
    // Legacy revisions are zero. Earned history alone must not revive a retired copy.
    return (Nei_Save()->tradeAdultOwned & (1u << index)) != 0 && !TradeAdult_IsConsumedIndex(index);
}

u8 TradeAdult_IsOwnedItem(u8 item) {
    s32 idx = TradeAdult_IndexOfItem(item);
    return (idx >= 0) ? TradeAdult_IsOwnedIndex(idx) : 0;
}

void TradeAdult_GiveIndex(s32 index) {
    if (index < 0 || index >= TRADE_ADULT_COUNT) {
        return;
    }
    NeiSaveData* save = Nei_Save();
    save->tradeAdultOwned |= (1u << index);
    // Only a real acquisition advances a consumed copy. Duplicate grants while available are
    // idempotent. UINT32_MAX is terminal consumed: saturate instead of wrapping below old peers.
    if (TradeAdult_IsConsumedIndex(index) && save->tradeAdultRevision[index] != UINT32_MAX) {
        ++save->tradeAdultRevision[index];
    }
    if (index == TRADE_ADULT_PENDANT) {
        save->pendantOwned = 1;
    }
}

void TradeAdult_GiveItem(u8 item) {
    s32 idx = TradeAdult_IndexOfItem(item);
    if (idx >= 0) {
        TradeAdult_GiveIndex(idx);
    }
}

// Owned-item count (drives the 2D-grid layout: rows/cols sized to how many the player holds).
s32 TradeAdult_OwnedCount(void) {
    s32 n = 0;
    for (s32 i = 0; i < TRADE_ADULT_COUNT; i++) {
        if (TradeAdult_IsOwnedIndex(i)) {
            n++;
        }
    }
    return n;
}

// The ordinal-th owned trade item -> its global trade index (or -1). The 2D-grid wheel lays out only
// the items the player holds, so it walks owned ordinals.
s32 TradeAdult_OwnedAt(s32 ordinal) {
    s32 n = 0;
    for (s32 i = 0; i < TRADE_ADULT_COUNT; i++) {
        if (TradeAdult_IsOwnedIndex(i)) {
            if (n == ordinal) {
                return i;
            }
            n++;
        }
    }
    return -1;
}

// An item's position within the owned list (or -1 if not owned). Used to start the grid cursor on the
// item currently shown in the slot.
s32 TradeAdult_OrdinalOf(u8 item) {
    s32 target = TradeAdult_IndexOfItem(item);
    if (target < 0 || !TradeAdult_IsOwnedIndex(target)) {
        return -1;
    }
    s32 n = 0;
    for (s32 i = 0; i < target; i++) {
        if (TradeAdult_IsOwnedIndex(i)) {
            n++;
        }
    }
    return n;
}

// Prev/next owned trade item relative to the one in the slot — feeds the shared adult-trade cycle wheel
// (KaleidoScope_HandleItemCycleExtras), so the wheel cycles every owned trade item (vanilla + MM).
u8 TradeAdult_NextItem(u8 cur) {
    s32 owned = TradeAdult_OwnedCount();
    if (owned <= 0) {
        return ITEM_NONE;
    }
    s32 ord = TradeAdult_OrdinalOf(cur);
    s32 nextOrd = (ord < 0) ? 0 : (ord + 1) % owned;
    s32 gi = TradeAdult_OwnedAt(nextOrd);
    return (gi >= 0) ? TradeAdult_ItemId(gi) : ITEM_NONE;
}

u8 TradeAdult_PrevItem(u8 cur) {
    s32 owned = TradeAdult_OwnedCount();
    if (owned <= 0) {
        return ITEM_NONE;
    }
    s32 ord = TradeAdult_OrdinalOf(cur);
    s32 prevOrd = (ord < 0) ? 0 : (ord + owned - 1) % owned;
    s32 gi = TradeAdult_OwnedAt(prevOrd);
    return (gi >= 0) ? TradeAdult_ItemId(gi) : ITEM_NONE;
}

// Observe a native/legacy item without treating a stale slot as a new acquisition. Folding only
// adds earned history; an odd revision remains consumed until TradeAdult_GiveIndex is called.
void TradeAdult_FoldCurrent(u8 item) {
    s32 idx = TradeAdult_IndexOfItem(item);
    if (idx >= 0) {
        Nei_Save()->tradeAdultOwned |= (1u << idx);
    }
}

void TradeAdult_ReconcileConsumed(void) {
    NeiSaveData* save = Nei_Save();
    if ((save->tradeAdultOwned & (1u << TRADE_ADULT_PENDANT)) && TradeAdult_IsConsumedIndex(TRADE_ADULT_PENDANT)) {
        save->pendantOwned = 1;
    }
    const s32 slots[] = { SLOT_TRADE_ADULT, SLOT_TRADE_CHILD };
    for (s32 i = 0; i < (s32)(sizeof(slots) / sizeof(slots[0])); ++i) {
        u8* item = &gSaveContext.inventory.items[slots[i]];
        if (TradeAdult_IsConsumedIndex(TradeAdult_IndexOfItem(*item))) {
            *item = ITEM_NONE;
        }
    }
    // The saved child/adult banks can restore an item after an age change. Retire exact IDs in
    // all three banks and all seven C/Dpad entries; a shared slot alone is not evidence to clear.
    ItemEquips* banks[] = { &gSaveContext.equips, &gSaveContext.childEquips, &gSaveContext.adultEquips };
    for (s32 bank = 0; bank < (s32)(sizeof(banks) / sizeof(banks[0])); ++bank) {
        ItemEquips* equips = banks[bank];
        for (s32 button = 1; button < (s32)(sizeof(equips->buttonItems) / sizeof(equips->buttonItems[0])); ++button) {
            if (TradeAdult_IsConsumedIndex(TradeAdult_IndexOfItem(equips->buttonItems[button]))) {
                equips->buttonItems[button] = ITEM_NONE;
                equips->cButtonSlots[button - 1] = SLOT_NONE;
            }
        }
    }
}

void TradeAdult_ConsumeIndex(s32 index) {
    if (index < 0 || index >= TRADE_ADULT_COUNT) {
        return;
    }
    NeiSaveData* save = Nei_Save();
    if (index == TRADE_ADULT_PENDANT && (save->tradeAdultOwned & (1u << index))) {
        save->pendantOwned = 1;
    }
    // Every even value has an odd successor, including UINT32_MAX - 1. Repeated callbacks for
    // this consumed copy are idempotent until a real acquisition advances it again.
    if (!TradeAdult_IsConsumedIndex(index)) {
        ++save->tradeAdultRevision[index];
    }
    TradeAdult_ReconcileConsumed();
}

void TradeAdult_ConsumeItem(u8 item) {
    s32 index = TradeAdult_IndexOfItem(item);
    if (index >= 0) {
        // A native legacy hand-in may precede the first pause-menu observation of the item.
        TradeAdult_FoldCurrent(item);
        TradeAdult_ConsumeIndex(index);
    }
}

// The editor can remove history locally, but must also retire the current copy so an older peer's
// history merge cannot restore availability. The independently earned Pendant perk remains owned.
void TradeAdult_SetOwnedIndex(s32 index, u8 on) {
    if (index < 0 || index >= TRADE_ADULT_COUNT) {
        return;
    }
    if (on) {
        TradeAdult_GiveIndex(index);
        return;
    }
    TradeAdult_ConsumeIndex(index);
    Nei_Save()->tradeAdultOwned &= ~(1u << index);
}

// True for the 8 MM trade-quest items that have no real use in OoT: indices 11..18 (Moon's Tear ..
// Special Delivery). The OoT items (0-10) keep their vanilla behavior; the Pendant (19 = Ext Boots 2)
// keeps its combat moveset. Using one of these just presents it + pops the gag message.
u8 TradeAdult_IsMmTradeUseItem(s32 item) {
    if (item < 0) {
        return 0;
    }
    s32 idx = TradeAdult_IndexOfItem((u8)item);
    return (idx >= 11 && idx <= 18);
}

extern void* MmAssets_LoadResource(const char* path);  // MM GI models (object_gi_*) from mm.o2r
extern void MmGi_SetupObjectSegments(PlayState* play); // shared with the rando GI draws (draw.cpp)

// Presented item -> its MM GI display list(s) from mm.o2r. NULL = nothing in that render bucket. (Room
// Key / letters are XLU; deeds are OPA.)
typedef struct {
    u8 item;
    const char* opaPath;
    const char* xluPath;
} TradePresentModel;

static const TradePresentModel sTradePresentModels[] = {
    { ITEM_MM_MOONS_TEAR, "__OTR__objects/object_gi_reserve00/gGiMoonsTearItemDL",
      "__OTR__objects/object_gi_reserve00/gGiMoonsTearGlowDL" },
    { ITEM_MM_DEED_LAND, "__OTR__objects/object_gi_reserve01/gGiTitleDeedLandColorDL", NULL },
    { ITEM_MM_DEED_SWAMP, "__OTR__objects/object_gi_reserve01/gGiTitleDeedSwampColorDL", NULL },
    { ITEM_MM_DEED_MOUNTAIN, "__OTR__objects/object_gi_reserve01/gGiTitleDeedMountainColorDL", NULL },
    { ITEM_MM_DEED_OCEAN, "__OTR__objects/object_gi_reserve01/gGiTitleDeedOceanColorDL", NULL },
    { ITEM_MM_ROOM_KEY, NULL, "__OTR__objects/object_gi_reserve_b_00/gGiRoomKeyDL" },
    { ITEM_MM_LETTER_KAFEI, "__OTR__objects/object_gi_reserve_c_00/gGiLetterToKafeiEnvelopeLetterDL",
      "__OTR__objects/object_gi_reserve_c_00/gGiLetterToKafeiInscriptionsDL" },
    { ITEM_MM_SPECIAL_DELIVERY, "__OTR__objects/object_gi_reserve_b_01/gGiLetterToMamaEnvelopeLetterDL",
      "__OTR__objects/object_gi_reserve_b_01/gGiLetterToMamaInscriptionsDL" },
};

static u8 sTradePresentState = 0; // 0 idle, 1 waiting for the textbox to open, 2 textbox open
static u8 sTradePresentItem = ITEM_NONE;

// Cached loaded DLs for the current item (MmAssets_LoadResource resolves the mm.o2r resource once).
static u8 sTradeLoadedItem = ITEM_NONE;
static Gfx* sTradeLoadedOpa = NULL;
static Gfx* sTradeLoadedXlu = NULL;

static void TradeAdult_LoadPresentModel(void) {
    if (sTradeLoadedItem == sTradePresentItem) {
        return;
    }
    sTradeLoadedItem = sTradePresentItem;
    sTradeLoadedOpa = NULL;
    sTradeLoadedXlu = NULL;
    for (s32 i = 0; i < (s32)(sizeof(sTradePresentModels) / sizeof(sTradePresentModels[0])); i++) {
        if (sTradePresentModels[i].item == sTradePresentItem) {
            if (sTradePresentModels[i].opaPath != NULL) {
                sTradeLoadedOpa = (Gfx*)MmAssets_LoadResource(sTradePresentModels[i].opaPath);
            }
            if (sTradePresentModels[i].xluPath != NULL) {
                sTradeLoadedXlu = (Gfx*)MmAssets_LoadResource(sTradePresentModels[i].xluPath);
            }
            break;
        }
    }
}

// Force the "hold up item" pose held at its last frame (Link's normal action would otherwise override
// the anim, so we re-assert it each frame while presenting).
static void TradeAdult_HoldPose(PlayState* play, Player* player) {
    if (player->skelAnime.animation != &gPlayerAnim_link_normal_take_out) {
        f32 last = Animation_GetLastFrame(&gPlayerAnim_link_normal_take_out);
        LinkAnimation_Change(play, &player->skelAnime, &gPlayerAnim_link_normal_take_out, 0.0f, last, last,
                             ANIMMODE_ONCE, -4.0f);
    }
}

// Present the item like an adult trade item — hold-up pose, present camera, world halt — plus the single
// gag message. We do NOT use the vanilla trade present (unk_6AD = 4): that routes to
// Player_Action_ExchangeItem and pops a SECOND textbox. Release + model draw run in the two funcs below.
void TradeAdult_PresentAndMessage(PlayState* play, Player* player, s32 item) {
    sTradePresentItem = (u8)item;
    // Play the MM "hold up cutscene item" anim (settles onto the presented pose).
    LinkAnimation_Change(play, &player->skelAnime, &gPlayerAnim_link_normal_take_out, 1.0f, 0.0f,
                         Animation_GetLastFrame(&gPlayerAnim_link_normal_take_out), ANIMMODE_ONCE, -6.0f);
    // Full vanilla-present freeze: halts every actor category except player/NPC (Actor_UpdateAll's
    // D_80116068 gates on these three flags).
    player->stateFlags1 |= PLAYER_STATE1_TALKING | PLAYER_STATE1_IN_ITEM_CS | PLAYER_STATE1_IN_CUTSCENE;
    // Present camera (swings around to face Link), same setting the vanilla trade present uses.
    Camera_RequestSetting(Play_GetCamera(play, 0), CAM_SET_TURN_AROUND);
    Message_StartTextbox(play, MM_TRADE_USE_TEXTID, NULL);
    sTradePresentState = 1;
}

// Per-frame (called from z_player next to Picto_Update): hold the pose + Link still until the textbox
// closes, then drop the freeze + restore the camera.
void TradeAdult_PresentUpdate(PlayState* play) {
    if (sTradePresentState == 0) {
        return;
    }
    Player* player = GET_PLAYER(play);
    player->linearVelocity = 0.0f;
    TradeAdult_HoldPose(play, player);
    if (sTradePresentState == 1) {
        if (play->msgCtx.msgMode != 0) { // the textbox actually opened
            sTradePresentState = 2;
        }
    } else if (play->msgCtx.msgMode == 0) { // textbox closed -> release
        extern s16 func_8005B1A4(Camera * camera);
        sTradePresentState = 0;
        player->stateFlags1 &= ~(PLAYER_STATE1_TALKING | PLAYER_STATE1_IN_ITEM_CS | PLAYER_STATE1_IN_CUTSCENE);
        func_8005B1A4(Play_GetCamera(play, 0)); // restore the camera from CAM_SET_TURN_AROUND
    }
}

// Draw the presented item's MM 3D model spinning in front of Link's left hand (mirrors the vanilla
// Player_DrawGetItemImpl placement: leftHandPos + 3.3 forward, +6 up, 0.2 scale, slow Y spin). Called
// from the player draw path after the limbs so leftHandPos is fresh.
void TradeAdult_PresentDraw(PlayState* play) {
    if (sTradePresentState == 0) {
        return;
    }
    TradeAdult_LoadPresentModel();
    if (sTradeLoadedOpa == NULL && sTradeLoadedXlu == NULL) {
        return;
    }
    Player* player = GET_PLAYER(play);
    Vec3f* hand = &player->leftHandPos;

    // We draw right after Link's limbs, so segment 0x09 still holds his mouth texture path here.
    MmGi_SetupObjectSegments(play);

    OPEN_DISPS(play->state.gfxCtx);
    Matrix_Translate(hand->x + (3.3f * Math_SinS(player->actor.shape.rot.y)), hand->y + 6.0f,
                     hand->z + (3.3f * Math_CosS(player->actor.shape.rot.y)), MTXMODE_NEW);
    Matrix_RotateZYX(0, play->gameplayFrames * 1000, 0, MTXMODE_APPLY); // slow Y spin, like the vanilla present
    Matrix_Scale(0.2f, 0.2f, 0.2f, MTXMODE_APPLY);
    if (sTradeLoadedOpa != NULL) {
        Gfx_SetupDL_25Opa(play->state.gfxCtx);
        gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_OPA_DISP++, sTradeLoadedOpa);
    }
    if (sTradeLoadedXlu != NULL) {
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_XLU_DISP++, sTradeLoadedXlu);
    }
    CLOSE_DISPS(play->state.gfxCtx);
}
