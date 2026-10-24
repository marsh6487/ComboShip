// Included in BlackMarket.cpp's private namespace after the transaction policy.
// Native EnOssan owns every shop action; these are scene/presentation and
// transaction boundaries, never a second input or camera state machine.
EnOssan* sNativeShop = nullptr;
PlayState* sNativeShopPlay = nullptr;
s16 sBrowseCamera = CAM_ID_NONE, sBrowseParent = CAM_ID_NONE, sBrowseCameraUid = -1;
Camera* sBrowseCameraPtr = nullptr;
std::array<Vec3s, TREASURE_CHEST_SHOP_GAL_LIMB_MAX> sKeeperJoints{}, sKeeperMorphs{};

void InitKeeper(Actor* actor);
void KeeperUpdate(Actor* actor, PlayState* play);
void KeeperDraw(Actor* actor, PlayState* play);

bool NativeShopCameraCapacity(PlayState* play) {
    // The last slot belongs to CutsceneManager's return snapshot.
    for (s16 id = CAM_ID_SUB_FIRST; id < NUM_CAMS - 1; ++id)
        if (!play->cameraPtrs[id])
            return true;
    return false;
}
bool NativeShopOwnCamera(PlayState* play) {
    return play && sBrowseCamera >= CAM_ID_SUB_FIRST && sBrowseCamera < NUM_CAMS - 1 && sBrowseCameraPtr &&
           play->cameraPtrs[sBrowseCamera] == sBrowseCameraPtr && sBrowseCameraPtr->uid == sBrowseCameraUid;
}
bool NativeShopDialog(const EnOssan* shop) {
    return shop && shop->actionFunc != EnOssan_Idle && shop->actionFunc != EnOssan_InitShop &&
           shop->actionFunc != EnOssan_BuyItemWithFanfare && shop->actionFunc != EnOssan_SetupItemPurchased;
}
bool NativeShopMessage(PlayState* play) {
    const auto id = play->msgCtx.currentTextId;
    return id >= kBrowseText && id <= kKeeperHelpText;
}
void ForgetNativeShopCamera() {
    sBrowseCamera = sBrowseParent = CAM_ID_NONE;
    sBrowseCameraUid = -1;
    sBrowseCameraPtr = nullptr;
}
void EndBrowsing(PlayState* play, bool destroying = false) {
    // Abnormal lifecycle/ownership exit only. Normal B, bank return and receipt
    // continuation execute EnOssan's own actions.
    if (!destroying && play && sNativeShop && sNativeShopPlay == play) {
        const bool own = NativeShopOwnCamera(play) && CutsceneManager_IsCurrentActor(&sNativeShop->actor);
        if (CutsceneManager_IsCurrentActor(&sNativeShop->actor))
            CutsceneManager_Stop(CutsceneManager_GetCurrentCsId());
        if ((own && Play_GetActiveCamId(play) == sBrowseCamera) || Play_GetActiveCamId(play) == CAM_ID_MAIN) {
            if (NativeShopMessage(play))
                Message_CloseTextbox(play);
        }
        if (auto* player = GET_PLAYER(play); player && sTalking)
            player->stateFlags2 &= ~PLAYER_STATE2_20000000;
        sNativeShop->actionFunc = EnOssan_Idle;
        sNativeShop->cutsceneState = 0;
        sNativeShop->drawCursor = 0;
        sNativeShop->stickLeftPrompt.isEnabled = sNativeShop->stickRightPrompt.isEnabled = false;
        sNativeShop->shopItemSelectedTween = 0;
        for (int i = 0; i < 8; ++i)
            if (auto* item = sNativeShop->items[i]) {
                item->actor.world.pos = kShelves[i];
                item->isSelected = false;
            }
    }
    ForgetNativeShopCamera();
    sTalking = false;
    if (destroying) {
        sNativeShop = nullptr;
        sNativeShopPlay = nullptr;
    }
}
bool NativeShopContext(PlayState* play) {
    return play && play == sNativeShopPlay && play == gPlayState && Active() &&
           play->transitionTrigger == TRANS_TRIGGER_OFF && play->transitionMode == TRANS_MODE_OFF &&
           play->gameOverCtx.state == GAMEOVER_INACTIVE;
}
bool NativeShopCameraAvailable(PlayState* play) {
    const auto cs = CutsceneManager_GetCurrentCsId();
    if (cs == CS_ID_NONE || cs == CS_ID_GLOBAL_TALK)
        return Play_GetActiveCamId(play) == CAM_ID_MAIN;
    return cs >= 0 && cs < BlackMarketMmShopScene::kCutsceneCount &&
           CutsceneManager_IsCurrentActor(&sNativeShop->actor) && NativeShopOwnCamera(play) &&
           Play_GetActiveCamId(play) == sBrowseCamera;
}
void MonitorBrowsing() {
    if (!sNativeShop || sNativeShop->actionFunc == EnOssan_InitShop || sNativeShop->actionFunc == EnOssan_Idle)
        return;
    auto* play = gPlayState;
    if (!play || play != sNativeShopPlay || gGameState != &play->state) {
        EndBrowsing(nullptr, true);
        return;
    }
    const auto messageState = Message_GetState(&play->msgCtx);
    const bool interruptedDialogue =
        sNativeShop->actionFunc == EnOssan_TalkToShopkeeper &&
        (!NativeShopMessage(play) || messageState == TEXT_STATE_NONE || messageState == TEXT_STATE_CLOSING);
    // Receipt actors legitimately own their own get-item camera and messages.
    if (!NativeShopContext(play) || interruptedDialogue || (sReceipt.pending && !ReceiptContext()) ||
        (!sReceipt.pending && !NativeShopCameraAvailable(play))) {
        EndBrowsing(play);
        CancelReceipt();
    }
}
int NativeStockSlot(const EnGirlA* item) {
    return item->actor.params - BlackMarketMmShopScene::kItemParamBase;
}
void NativeStockRefresh(PlayState*, EnGirlA* item) {
    const int slot = NativeStockSlot(item);
    item->isOutOfStock = slot < 0 || slot >= 8 || !Offer(slot) || Status(slot) == PurchaseResult::SoldOut;
}
void NativeStockNoPurchase(PlayState*, EnGirlA*) {
    // The acknowledged CustomItem receipt is the only grant/payment authority.
}
s32 NativeStockCanBuy(PlayState*, EnGirlA* item) {
    sSelected = NativeStockSlot(item);
    switch (Status(sSelected)) {
        case PurchaseResult::Ready:
            return CANBUY_RESULT_SUCCESS_1;
        case PurchaseResult::NeedRupees:
            sStatus = "You need more rupees.";
            return CANBUY_RESULT_NEED_RUPEES;
        case PurchaseResult::SoldOut:
            sStatus = "That one has already been sold.";
            break;
        case PurchaseResult::Owned:
            sStatus = "You already own that power.";
            break;
        case PurchaseResult::Locked: {
            const int clears = sSession.market["stock"][sSelected]["clears"];
            sStatus = "The seal needs " + std::to_string(clears) + " unique dungeon clear" + (clears == 1 ? "." : "s.");
            break;
        }
        case PurchaseResult::Unavailable:
            sStatus = Offer(sSelected) && Offer(sSelected)->renewable ? "Bring an empty bottle for this refill."
                                                                      : "You need its prerequisite first.";
            if (Offer(sSelected) && Offer(sSelected)->renewable)
                return CANBUY_RESULT_NEED_EMPTY_BOTTLE;
            break;
    }
    return CANBUY_RESULT_CANNOT_GET_NOW_2;
}
void InitNativeStock(Actor* actor, PlayState* play) {
    auto* item = reinterpret_cast<EnGirlA*>(actor);
    const int slot = NativeStockSlot(item);
    actor->destroy = nullptr;
    if (!sNativeShop || play != sNativeShopPlay || slot < 0 || slot >= 8) {
        Actor_Kill(actor);
        return;
    }
    item->actionFunc = EnGirlA_DoNothing;
    item->mainActionFunc = EnGirlA_Update2;
    item->isInitialized = true;
    item->actor.textId = kBrowseText;
    item->choiceTextId = kConfirmText;
    item->canBuyFunc = NativeStockCanBuy;
    item->buyFunc = item->buyFanfareFunc = item->boughtFunc = NativeStockNoPurchase;
    item->restockFunc = NativeStockRefresh;
    actor->update = EnGirlA_Update;
    actor->draw = nullptr; // Black Market's GI renderer draws this actual actor.
    actor->flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED;
    actor->world.pos = actor->home.pos = kShelves[slot];
    Actor_SetScale(actor, .25f);
    Actor_SetFocus(actor, 5);
    NativeStockRefresh(play, item);
}
void NativeKeeperBlink(EnOssan* shop) {
    if (shop->blinkTimer > 0)
        --shop->blinkTimer;
    if (!shop->blinkTimer) {
        shop->eyeTexIndex = (shop->eyeTexIndex + 1) % 4;
        shop->blinkTimer = shop->eyeTexIndex ? 2 : 80 + (sNativeShopPlay->gameplayFrames % 53);
    }
}
s32 NativeKeeperInit(EnOssan* shop, PlayState* play) {
    InitKeeper(&shop->actor);
    if (!shop->actor.update)
        return false;
    for (int i = 0; i < 8; ++i) {
        const auto pos = kShelves[i];
        shop->items[i] =
            reinterpret_cast<EnGirlA*>(Actor_Spawn(&play->actorCtx, play, ACTOR_EN_GIRLA, pos.x, pos.y, pos.z, 0, 0, 0,
                                                   BlackMarketMmShopScene::kItemParamBase + i));
        if (!shop->items[i]) {
            for (int j = 0; j < i; ++j) {
                Actor_Kill(&shop->items[j]->actor);
                shop->items[j] = nullptr;
            }
            return false;
        }
    }
    return true;
}
s32 NativeKeeperCanTalk(EnOssan* shop, PlayState* play) {
    return NativeShopContext(play) && Safe(play) && !sReceipt.pending && !sTalking &&
           Play_GetActiveCamId(play) == CAM_ID_MAIN && NativeShopCameraCapacity(play) &&
           shop->actor.xzDistToPlayer < 100.0f;
}
s32 NativeKeeperBeginTalk(EnOssan* shop, PlayState* play) {
    auto* player = GET_PLAYER(play);
    const auto current = CutsceneManager_GetCurrentCsId();
    if (!NativeKeeperCanTalk(shop, play) || !player || (player->stateFlags2 & PLAYER_STATE2_20000000) ||
        (current != CS_ID_NONE && current != CS_ID_GLOBAL_TALK))
        return false;
    ForgetNativeShopCamera();
    sBrowseParent = CAM_ID_MAIN;
    sTalking = true;
    return true;
}
u16 NativeKeeperText(EnOssan* shop, u16 text) {
    sSelected = shop->cursorIndex;
    if (text == 0x640)
        return kKeeperMenuText;
    if (text == 0x642)
        return kContinueText;
    if (text == 0x6AB || text == 0x6C8)
        return kKeeperTalkText;
    if ((text >= 0x6BC && text <= 0x6C0) || (text >= 0x6D9 && text <= 0x6DD))
        return kResultText;
    return kWelcomeText;
}
s32 NativeKeeperCanStart(EnOssan*, PlayState* play) {
    return NativeShopContext(play) && NativeShopCameraAvailable(play) && NativeShopCameraCapacity(play);
}
void NativeKeeperStarted(EnOssan* shop, PlayState* play) {
    sBrowseCamera = CutsceneManager_GetCurrentSubCamId(shop->csId);
    sBrowseCameraPtr = Play_GetCamera(play, sBrowseCamera);
    sBrowseCameraUid = sBrowseCameraPtr->uid;
    sBrowseParent = CAM_ID_MAIN;
}
void NativeItemPositions(EnOssan*, s32 slot, Vec3f* shelf, Vec3f* selected) {
    if (slot < 0 || slot >= 8)
        return;
    *shelf = kShelves[slot];
    const auto& target = BlackMarketMmShopScene::kSelectedTargets[slot >> 2];
    *selected = { target[0], target[1], target[2] };
}
s32 NativeOfferReceipt(EnOssan* shop, PlayState*) {
    sSelected = shop->cursorIndex;
    return Purchase();
}
s32 NativeReceiptAccepted(EnOssan*, PlayState*) {
    return !sReceipt.pending || !ReceiptContext() ? -1 : sReceipt.delivered ? 1 : 0;
}
s32 NativeReceiptComplete(EnOssan*, PlayState* play) {
    auto* player = GET_PLAYER(play);
    if (!sReceipt.pending || !sReceipt.delivered || !ReceiptContext() || !player || play->msgCtx.msgMode != 0 ||
        (player->stateFlags1 & PLAYER_STATE1_400) || Player_InBlockingCsMode(play, player) || !Safe(play) ||
        Play_GetActiveCamId(play) != CAM_ID_MAIN)
        return false;
    CancelReceipt();
    return true;
}
s32 NativeKeeperDialogue(EnOssan*, PlayState* play) {
    // EnOssan retains action/cutscene ownership. Only the native Talk action
    // delegates these optional message transitions; shelves and purchases do not.
    if (play->state.input[0].press.button & BTN_B)
        return true;
    const auto state = Message_GetState(&play->msgCtx);
    if (!(play->state.input[0].press.button & BTN_A) || !Message_ShouldAdvance(play))
        return false;
    const auto text = play->msgCtx.currentTextId;
    if (text == kKeeperHelpChoiceText && state == TEXT_STATE_CHOICE) {
        if (play->msgCtx.choiceIndex != 0)
            return true;
        Message_ContinueTextbox(play, kKeeperHelpText);
    } else if (state == TEXT_STATE_EVENT || state == TEXT_STATE_DONE) {
        if (text != kKeeperTalkText)
            return true;
        Message_ContinueTextbox(play, kKeeperHelpChoiceText);
    }
    return false;
}
const ComboBlackMarketShopAdapter kNativeShopAdapter = {
    NativeKeeperInit,      NativeKeeperCanTalk,   NativeKeeperBeginTalk, NativeKeeperText,
    NativeKeeperCanStart,  NativeKeeperStarted,   NativeItemPositions,   NativeOfferReceipt,
    NativeReceiptAccepted, NativeReceiptComplete, NativeKeeperDialogue
};
