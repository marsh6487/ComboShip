#include "ComboStartupReceipt.h"
#ifdef COMBO_BUILD
#include "ComboExport.h"
#include "ComboGameMode.h"
#include "soh/OTRGlobals.h"
#include "soh/ShipInit.hpp"
#include "soh/SaveManager.h"
#include "soh/Notification/Notification.h"
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/Enhancements/enhancementTypes.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/randomizer/hook_handlers.h"
#include "soh/Enhancements/randomizer/static_data.h"
#include "ComboDungeonPickup.h"
#include <cstring>
#include <string>
extern "C" {
#include "global.h"
extern PlayState* gPlayState;
void Save_SaveFile(void);
}
namespace {
// BEGIN TESTED STARTUP CORE
constexpr uint16_t kStartupText = 0xF9E2;
CwStartupReceiptHost sHost = nullptr;
CwStartupReceipt sReceipt{};
GetItemEntry sEntry{};
int sSlot = -1;
int32_t sToken = 0;
uint32_t sSequence = 0;
bool sOffered = false, sVisible = false, sAcked = false;

bool Active() {
    return sHost && Combo_IsCombinedGame() && IS_RANDO && gSaveContext.fileNum <= 2;
}
bool PrepareContextChange() {
    return !sHost || !Combo_IsCombinedGame() ||
           sHost(CW_STARTUP_PREPARE_CONTEXT_CHANGE, 0, gSaveContext.fileNum, nullptr) == 1;
}
int CaptureSave(int32_t slot) {
    if (!Active() || slot < 0 || slot > 2 || slot != gSaveContext.fileNum || !SaveManager::Instance)
        return 0;
    try {
        SaveManager::Instance->ThreadPoolWait();
        if (sHost(CW_STARTUP_PREPARE_PEER_SNAPSHOT, 0, slot, nullptr) != 1)
            return 0;
        Save_SaveFile();
        SaveManager::Instance->ThreadPoolWait();
        return 1;
    } catch (...) {
        // The launcher validates that its staged OoT snapshot actually arrived;
        // a queued writer's failure cannot be inferred from a void native API.
        return 0;
    }
}
bool IsReceipt(const GetItemEntry* entry) {
    return entry && (entry->comboMarketReceipt & CW_STARTUP_RECEIPT_TOKEN_BIT) != 0;
}
bool SameEntry(const GetItemEntry& a, const GetItemEntry& b) {
    return a.modIndex == b.modIndex && a.tableId == b.tableId && a.getItemId == b.getItemId && a.itemId == b.itemId;
}
void Cancel(bool destroying = false) {
    if (!destroying && gPlayState) {
        auto* player = GET_PLAYER(gPlayState);
        if (player && sToken && player->getItemEntry.comboMarketReceipt == sToken) {
            player->getItemEntry = GET_ITEM_NONE;
            player->getItemId = GI_NONE;
            if (player->interactRangeActor == &player->actor)
                player->interactRangeActor = nullptr;
        }
    }
    sReceipt = {};
    sEntry = {};
    sSlot = -1;
    sToken = 0;
    sOffered = sVisible = sAcked = false;
}
void BeginFile() {
    Cancel();
    if (Active())
        sHost(CW_STARTUP_BEGIN_FILE, 0, gSaveContext.fileNum, nullptr);
}
void CopyName(char (&out)[256], const std::string& name) {
    std::strncpy(out, name.c_str(), sizeof(out) - 1);
}
bool Capture(const GetItemEntry* entry, int32_t check) {
    if (!Active() || !entry || check != RC_SONG_FROM_IMPA ||
        (entry->modIndex == MOD_NONE && entry->itemId == ITEM_NONE))
        return false;
    CwStartupReceipt r{};
    r.version = CW_STARTUP_RECEIPT_VERSION;
    r.state = CW_STARTUP_READY;
    r.modIndex = entry->modIndex;
    r.getItemId = entry->getItemId;
    r.itemId = entry->itemId;
    r.tableId = entry->tableId;
    r.objectId = entry->objectId;
    r.field = entry->field;
    r.gi = entry->gi;
    r.gid = entry->gid;
    r.drawItemId = entry->drawItemId;
    r.drawModIndex = entry->drawModIndex;
    r.category = entry->getItemCategory;
    if (entry->modIndex == MOD_RANDOMIZER && entry->getItemId == RG_COMBO_FOREIGN) {
        const auto* fi = OOT_LookupForeignByCheck(RC_SONG_FROM_IMPA);
        if (!fi || fi->itemGame != ComboRando::GAME_MM)
            return false;
        r.itemGame = 1;
        r.state = CW_STARTUP_WAIT_MM;
        r.category = OOT_GetForeignCategory(RC_SONG_FROM_IMPA);
        r.trap = fi->trap;
        r.dungeonInfo =
            fi->category == "map" || fi->category == "compass" || ComboDungeonPickup::IsMapOrCompass(fi->itemName);
        CopyName(r.itemName, fi->itemName);
        CopyName(r.displayName, fi->displayName.empty() ? fi->itemName : fi->displayName);
    } else {
        // Match the pregrant entry against concrete catalog rows. GetGIEntry()
        // would re-resolve a progressive chain and can select the next tier.
        for (int id = RG_NONE + 1; id < RG_MAX; ++id) {
            const auto& item = Rando::StaticData::RetrieveItem(static_cast<RandomizerGet>(id));
            const auto exact = item.GetGIEntryUnresolved();
            if (exact && SameEntry(*exact, *entry)) {
                CopyName(r.itemName, item.GetName().english);
                CopyName(r.displayName, item.GetName().english);
                break;
            }
        }
        if (!r.itemName[0])
            return false;
        r.dungeonInfo = ComboDungeonPickup::IsMapOrCompass(r.itemName);
        r.trap = entry->modIndex == MOD_RANDOMIZER && entry->getItemId == RG_ICE_TRAP;
    }
    r.importance = r.category == ITEM_CATEGORY_JUNK ? CW_STARTUP_JUNK
                   : r.category == ITEM_CATEGORY_MAJOR || r.category == ITEM_CATEGORY_BOSS_KEY ||
                           r.category == ITEM_CATEGORY_SMALL_KEY
                       ? CW_STARTUP_MAJOR
                       : CW_STARTUP_MINOR;
    const bool deferred = r.state == CW_STARTUP_WAIT_MM;
    return sHost(CW_STARTUP_CAPTURE, 0, gSaveContext.fileNum, &r) == 1 && deferred;
}
bool SkipAnimation(const CwStartupReceipt& r, int policy) {
    if (r.itemGame == 0 && r.trap)
        return false;
    return policy == SGIA_ALL || (policy == SGIA_JUNK && !r.dungeonInfo &&
                                  (r.category == ITEM_CATEGORY_JUNK || r.category == ITEM_CATEGORY_SKULLTULA_TOKEN ||
                                   r.category == ITEM_CATEGORY_HEALTH || r.category == ITEM_CATEGORY_LESSER));
}
bool Safe(PlayState* play) {
    if (!play || play != gPlayState || !GameInteractor::IsSaveLoaded() || gSaveContext.gameMode != GAMEMODE_NORMAL ||
        play->transitionTrigger != TRANS_TRIGGER_OFF || play->transitionMode != TRANS_MODE_OFF ||
        play->pauseCtx.state || play->pauseCtx.debugState || play->gameOverCtx.state != GAMEOVER_INACTIVE ||
        play->csCtx.state != CS_STATE_IDLE || gSaveContext.cutsceneTrigger ||
        Message_GetState(&play->msgCtx) != TEXT_STATE_NONE)
        return false;
    auto* player = GET_PLAYER(play);
    return player && !Player_InCsMode(play) && player->getItemId == GI_NONE &&
           !(player->stateFlags1 &
             (PLAYER_STATE1_DEAD | PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_TALKING | PLAYER_STATE1_IN_CUTSCENE |
              PLAYER_STATE1_CARRYING_ACTOR | PLAYER_STATE1_HANGING_OFF_LEDGE | PLAYER_STATE1_CLIMBING_LEDGE |
              PLAYER_STATE1_FREEFALL | PLAYER_STATE1_JUMPING | PLAYER_STATE1_IN_WATER | PLAYER_STATE1_CLIMBING_LADDER));
}
bool RestoreFrozenDraw(const CwStartupReceipt* receipt, GetItemEntry* entry) {
    if (!receipt || !entry)
        return false;
    if (receipt->drawModIndex == MOD_RANDOMIZER) {
        if (receipt->drawItemId <= RG_NONE || receipt->drawItemId >= RG_MAX)
            return false;
        const auto draw =
            Rando::StaticData::RetrieveItem(static_cast<RandomizerGet>(receipt->drawItemId)).GetGIEntryUnresolved();
        if (!draw)
            return false;
        entry->drawFunc = draw->drawFunc;
        return true;
    }
    if (receipt->drawModIndex == MOD_NONE) {
        for (int id = RG_NONE + 1; id < RG_MAX; ++id) {
            const auto draw = Rando::StaticData::RetrieveItem(static_cast<RandomizerGet>(id)).GetGIEntryUnresolved();
            if (draw && draw->drawModIndex == MOD_NONE && draw->drawItemId == receipt->drawItemId) {
                entry->drawFunc = draw->drawFunc;
                return true;
            }
        }
    }
    return false;
}
bool BuildEntry(const CwStartupReceipt& r, GetItemEntry& entry) {
    if (r.version != CW_STARTUP_RECEIPT_VERSION || r.state != CW_STARTUP_READY || !r.generation ||
        r.category < ITEM_CATEGORY_JUNK || r.category > ITEM_CATEGORY_MAJOR ||
        !std::memchr(r.itemName, 0, sizeof(r.itemName)) || !r.itemName[0] ||
        !std::memchr(r.displayName, 0, sizeof(r.displayName)) || !r.displayName[0])
        return false;
    // Foreign receipts use only the generic model wrapper; their owner identity
    // comes from the frozen name below, never the originating check/latch.
    if (r.itemGame == 1) {
        const auto foreign = Rando::StaticData::RetrieveItem(RG_COMBO_FOREIGN).GetGIEntryUnresolved();
        if (!foreign)
            return false;
        entry = *foreign;
    } else if (r.itemGame == 0) {
        bool found = false;
        for (int id = RG_NONE + 1; id < RG_MAX; ++id) {
            const auto exact = Rando::StaticData::RetrieveItem(static_cast<RandomizerGet>(id)).GetGIEntryUnresolved();
            if (exact && exact->modIndex == r.modIndex && exact->tableId == r.tableId &&
                exact->getItemId == r.getItemId && exact->itemId == r.itemId) {
                entry = *exact;
                found = true;
                break;
            }
        }
        if (!found)
            return false;
        entry.objectId = r.objectId;
        entry.field = r.field;
        entry.gi = r.gi;
        entry.gid = r.gid;
        entry.drawItemId = r.drawItemId;
        entry.drawModIndex = r.drawModIndex;
        if (!RestoreFrozenDraw(&r, &entry))
            return false;
    } else {
        return false;
    }
    entry.getItemCategory = static_cast<GetItemCategory>(r.category);
    entry.textId = kStartupText;
    entry.collectable = true;
    entry.getItemFrom = ITEM_FROM_NPC;
    entry.comboForeignCheck = 0;
    return true;
}
void RetryAck() {
    if (sVisible && !sAcked && Active() && sSlot == gSaveContext.fileNum)
        sAcked = sHost(CW_STARTUP_ACKNOWLEDGE, 0, sSlot, &sReceipt) == 1;
}
void Visible() {
    if (sVisible)
        return;
    sVisible = true;
    Notification::Emit(
        { .message = "You received", .suffix = std::string(sReceipt.displayName) + " — " + CW_STARTUP_SOURCE });
    RetryAck();
}
void TextStarted(PlayState* play, const GetItemEntry* entry) {
    const auto* player = play && play == gPlayState ? GET_PLAYER(play) : nullptr;
    if (!Active() || !sOffered || !entry || !player || sSlot != gSaveContext.fileNum ||
        entry->comboMarketReceipt != sToken || player->getItemEntry.comboMarketReceipt != sToken ||
        !(player->stateFlags1 & PLAYER_STATE1_GETTING_ITEM) || !SameEntry(*entry, sEntry))
        return;
    Visible();
}
void Frame() {
    if (!Active() || !GameInteractor::IsSaveLoaded() || !gPlayState) {
        Cancel();
        return;
    }
    if (sSlot != -1 && sSlot != gSaveContext.fileNum)
        Cancel();
    if (sVisible) {
        RetryAck();
        if (sAcked && Safe(gPlayState))
            Cancel();
        return;
    }
    if (sOffered) {
        const auto* player = GET_PLAYER(gPlayState);
        if (gPlayState->transitionTrigger != TRANS_TRIGGER_OFF || gPlayState->gameOverCtx.state != GAMEOVER_INACTIVE ||
            !player ||
            (player->getItemEntry.comboMarketReceipt != sToken && !(player->stateFlags1 & PLAYER_STATE1_GETTING_ITEM)))
            Cancel();
        return;
    }
    if (!Safe(gPlayState))
        return;
    CwStartupReceipt r{};
    if (sHost(CW_STARTUP_PEEK, 0, gSaveContext.fileNum, &r) != 1 || !BuildEntry(r, sEntry))
        return;
    sReceipt = r;
    sSlot = gSaveContext.fileNum;
    sSequence = sSequence >= 0x0FFFFFFE ? 1 : sSequence + 1;
    sToken = CW_STARTUP_RECEIPT_TOKEN_BIT | sSequence;
    sEntry.comboMarketReceipt = sToken;
    if (SkipAnimation(r, CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("TimeSavers.SkipGetItemAnimation"), SGIA_JUNK)))
        Visible();
    else
        sOffered = GiveItemEntryWithoutActor(gPlayState, sEntry) != 0;
}
// END TESTED STARTUP CORE
void Text(u16* textId, bool* fromTable) {
    if (*textId != kStartupText || !sToken || !Active() || sSlot != gSaveContext.fileNum)
        return;
    CustomMessage message(std::string("You received %g") + sReceipt.displayName + "%w!&From " + CW_STARTUP_SOURCE +
                          ".");
    message.AutoFormat();
    message.LoadIntoFont();
    *fromTable = false;
}
void RegisterStartupReceipt() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>(Frame);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>([](int) { Cancel(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([] { Cancel(true); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnOpenText>(Text);
}
RegisterShipInitFunc initStartupReceipt(RegisterStartupReceipt);
} // namespace
extern "C" COMBO_EXPORT void SOH_SetStartupReceiptHost(CwStartupReceiptHost host) {
    Cancel();
    sHost = host;
}
extern "C" COMBO_EXPORT int32_t SOH_CaptureStartupReceiptSave(int32_t slot) {
    return CaptureSave(slot);
}
extern "C" bool ComboStartupOot_RestoreFrozenDraw(const CwStartupReceipt* receipt, GetItemEntry* entry) {
    return RestoreFrozenDraw(receipt, entry);
}
extern "C" bool ComboStartupOot_PrepareContextChange() {
    return PrepareContextChange();
}
extern "C" void ComboStartupOot_BeginFile() {
    BeginFile();
}
extern "C" bool ComboStartupOot_Capture(const GetItemEntry* entry, int32_t check) {
    return Capture(entry, check);
}
extern "C" bool ComboStartupOot_IsReceipt(const GetItemEntry* entry) {
    return IsReceipt(entry);
}
extern "C" void ComboStartupOot_TextStarted(PlayState* play, const GetItemEntry* entry) {
    TextStarted(play, entry);
}
extern "C" const char* ComboStartupOot_FrozenForeignName(const GetItemEntry* entry) {
    return IsReceipt(entry) && sToken && entry->comboMarketReceipt == sToken && sReceipt.itemGame == 1 && Active() &&
                   sSlot == gSaveContext.fileNum
               ? sReceipt.itemName
               : nullptr;
}
#endif
