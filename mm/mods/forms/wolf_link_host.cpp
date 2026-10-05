#include "mods/forms/wolf_link_host.h"
#include "mods/nei_oot_compat.h"
#include "mods/forms/custom_forms.h"
#include "mods/ext_buttons/ext_buttons.h"
#include "mods/extended_inventory.h"
#include "mods/extended_equipment.h"
#include "mods/items/custom_items.h"
#include "mods/items/logic/item_beetle.h"
#include "mods/nei_save.h"
#include "mods/o2r_loader/o2r_loader.h"
#include "mods/pak_loader/pak_loader.h"
#include "mods/transformation_masks/wolf_link_form.h"
#include "expansions/sm64/sm64_mario.h"
#include <libultraship/bridge/consolevariablebridge.h>

extern "C" {
// MM installs these actions for knockback, electricity, freezing and thawing.
void Player_Action_20(Player*, PlayState*);
void Player_Action_21(Player*, PlayState*);
void Player_Action_82(Player*, PlayState*);
void Player_Action_83(Player*, PlayState*);
}

namespace {
struct HostState {
    PlayState* play = nullptr;
    Player* player = nullptr;
    Input raw{};
    Input effective{};
    u32 frame = 0;
    s16 scene = 0;
    u16 filter = 0;
    u16 toggle = 0;
};
HostState sHost;

constexpr u16 kItemButtons = BTN_CLEFT | BTN_CDOWN | BTN_CRIGHT | BTN_DRIGHT | BTN_DLEFT | BTN_DDOWN | BTN_DUP;
constexpr u16 kMashButtons = BTN_A | BTN_B | BTN_R;
constexpr u32 kInteractionStates = PLAYER_STATE1_TALKING | PLAYER_STATE1_CARRYING_ACTOR | PLAYER_STATE1_400 |
                                   PLAYER_STATE1_4 | PLAYER_STATE1_2000 | PLAYER_STATE1_4000 |
                                   PLAYER_STATE1_FIRST_PERSON | PLAYER_STATE1_CLIMBING_LADDER |
                                   PLAYER_STATE1_IN_CUTSCENE | PLAYER_STATE1_8000000 | PLAYER_STATE1_800000;

bool NativeDamage(const Player* player) {
    return (player->stateFlags1 & PLAYER_STATE1_4000000) || player->actionFunc == Player_Action_20 ||
           player->actionFunc == Player_Action_21 || player->actionFunc == Player_Action_82 ||
           player->actionFunc == Player_Action_83;
}

bool FormItem(u16 item) {
    return (item >= ITEM_MASK_DEKU && item <= ITEM_MASK_GIANT) || item == ITEM_MARIO_MASK || item == ITEM_POKEBALL ||
           item == ITEM_OOT_MASK_PLACEHOLDER || item == ITEM_OOT_MASK_PREV || item == ITEM_OOT_MASK_NEXT;
}

u16 ItemButtons(u16 item, bool forms) {
    constexpr u16 cButtons[] = { BTN_CLEFT, BTN_CDOWN, BTN_CRIGHT };
    constexpr u16 dButtons[] = { BTN_DRIGHT, BTN_DLEFT, BTN_DDOWN, BTN_DUP };
    u16 buttons = 0;
    for (s32 i = 0; i < 3; ++i) {
        const u16 equipped = ExtButton_GetItem(0, EQUIP_SLOT_C_LEFT + i);
        if (forms ? FormItem(equipped) : equipped == item)
            buttons |= cButtons[i];
    }
    if (CVarGetInteger("gEnhancements.Dpad.DpadEquips", 0)) {
        for (s32 i = 0; i < 4; ++i) {
            const u16 equipped = ExtButton_GetDpadItem(0, i);
            if (forms ? FormItem(equipped) : equipped == item)
                buttons |= dButtons[i];
        }
    }
    return buttons;
}

bool OtherOwner(Player* player) {
    return CustomForms_ActiveForm() != CUSTOM_FORM_NONE || Sm64Mario_IsActive() || CVarGetInteger("gSm64Mario", 0) ||
           PakLoader_HasActiveModel() || O2rLoader_HasActiveModel() || ExtEquip_PendantActive() || Beetle_IsFlying() ||
           KiteSurf_IsActive() || Trident_OwnsPlayerAction() || CustomItems_BlocksMovement(player);
}

bool MustRelease(PlayState* play, Player* player) {
    return player != GET_PLAYER(play) || player->transformation != PLAYER_FORM_HUMAN ||
           player->currentMask != PLAYER_MASK_NONE || OtherOwner(player) ||
           !CVarGetInteger("gMods.WolfLink.Enabled", 1) || play->transitionTrigger != TRANS_TRIGGER_OFF ||
           play->transitionMode != TRANS_MODE_OFF || play->actorCtx.isOverrideInputOn ||
           (player->stateFlags1 & (PLAYER_STATE1_DEAD | PLAYER_STATE1_200 | PLAYER_STATE1_20)) ||
           gSaveContext.save.saveInfo.playerData.health <= 0 ||
           (player->stateFlags3 & PLAYER_STATE3_FLYING_WITH_HOOKSHOT);
}

bool ContextAction(const Player* player) {
    return (player->stateFlags1 & kInteractionStates) ||
           ((player->stateFlags2 & PLAYER_STATE2_CAN_ACCEPT_TALK_OFFER) && player->talkActor) ||
           player->interactRangeActor || player->doorType != PLAYER_DOORTYPE_NONE ||
           player->csAction != PLAYER_CSACTION_NONE || player->textboxBtnCooldownTimer != 0;
}

bool CanActivate(PlayState* play, Player* player) {
    return !MustRelease(play, player) && !NativeDamage(player) && !ContextAction(player) &&
           player->heldActor == nullptr && player->rightHandActor == nullptr && player->itemAction <= PLAYER_IA_NONE &&
           player->meleeWeaponState == PLAYER_MELEE_WEAPON_STATE_0 &&
           Nei_Save()->ownedItems[SLOT_SHADOW_CRYSTAL - 24] == EXT_ITEM_SHADOW_CRYSTAL;
}

bool Active(Player* player) {
    return sHost.player == player && WolfLinkForm_IsSelected() && WolfLinkForm_IsReady();
}

void Stop(PlayState* play, Player* player) {
    WolfLinkForm_Cleanup(player, play);
    sHost.filter = 0;
}

void Filter(Input* input, u16 buttons) {
    input->cur.button &= ~buttons;
    input->press.button &= ~buttons;
    input->rel.button &= ~buttons;
}
} // namespace

extern "C" void WolfLinkHost_PreUpdate(PlayState* play, Player* player) {
    if (player != GET_PLAYER(play))
        return;
    if (sHost.player && (sHost.player != player || sHost.play != play)) {
        // A missed destroy must not dereference an actor/play allocation from the old scene.
        Stop(nullptr, nullptr);
    } else if (sHost.player && (sHost.scene != play->sceneId || play->gameplayFrames < sHost.frame)) {
        Stop(play, player);
    }
    sHost.play = play;
    sHost.player = player;
    sHost.scene = play->sceneId;
    sHost.frame = play->gameplayFrames;
    sHost.raw = play->state.input[0];
    sHost.effective = Input{};
    sHost.filter = 0;
    sHost.toggle = 0;

    if (MustRelease(play, player)) {
        if (Active(player))
            Stop(play, player);
        return;
    }

    if (sHost.raw.press.button & ItemButtons(0, true)) {
        if (Active(player))
            Stop(play, player);
        return;
    }
    if (Active(player) || CanActivate(play, player)) {
        sHost.toggle = ItemButtons(EXT_ITEM_SHADOW_CRYSTAL, false);
        sHost.filter = sHost.toggle;
    }
    if (Active(player)) {
        sHost.filter |= kItemButtons;
        if (!NativeDamage(player)) {
            sHost.filter |= BTN_B | BTN_R;
            if (!ContextAction(player))
                sHost.filter |= BTN_A;
        }
    }
    // Raw listeners (equipment/tools/Slate/Hourglass) run before native local-pad filtering.
    Filter(&play->state.input[0], sHost.filter);
}

extern "C" void WolfLinkHost_RestorePlayerInput(Player* player, Input* input) {
    if (sHost.player != player)
        return;
    // Reconstitute only reservations made before raw listeners. MM selects/suppresses
    // this physical pad afterward, and OnPassPlayerInputs still has final policy control.
    const u16 reserved = (sHost.filter & kMashButtons) | sHost.toggle;
    input->cur.button |= sHost.raw.cur.button & reserved;
    input->press.button |= sHost.raw.press.button & reserved;
    input->rel.button |= sHost.raw.rel.button & reserved;
}

extern "C" void WolfLinkHost_FilterInput(Player* player, Input* input) {
    if (sHost.player != player)
        return;
    sHost.effective = *input;
    const u16 toggle = sHost.toggle;
    if (input->press.button & toggle) {
        if (Active(player))
            Stop(sHost.play, player);
        else if (CanActivate(sHost.play, player) && WolfLinkForm_LoadSkeleton(sHost.play))
            WolfLinkForm_Select(1);
    }
    sHost.filter = toggle;
    if (Active(player)) {
        sHost.filter |= kItemButtons;
        if (!NativeDamage(player)) {
            sHost.filter |= BTN_B | BTN_R;
            if (!ContextAction(player))
                sHost.filter |= BTN_A;
        }
    }
    Filter(input, sHost.filter);
}

extern "C" void WolfLinkHost_BeforeAction(PlayState* play, Player* player, Input* input) {
    if (!Active(player))
        return;
    // Native damage/scene processing runs inside Player_UpdateCommon after PreUpdate.
    if (MustRelease(play, player)) {
        Stop(play, player);
        return;
    }
    const bool native = NativeDamage(player);
    Input actionInput = sHost.effective;
    if (native) {
        // Preserve freeze/thaw mash inputs even when MM installed the action this frame.
        input->cur.button |= sHost.effective.cur.button & kMashButtons;
        input->press.button |= sHost.effective.press.button & kMashButtons;
        input->rel.button |= sHost.effective.rel.button & kMashButtons;
    } else if (ContextAction(player)) {
        WolfLinkForm_ReleaseAction(player);
        Filter(&actionInput, kMashButtons);
    }
    WolfLinkForm_Update(player, play, &actionInput, native);
}

extern "C" void WolfLinkHost_ApplyCollisionShape(Player* player) {
    if (Active(player))
        WolfLinkForm_ApplyCollisionShape(player);
}

extern "C" u8 WolfLinkHost_Draw(PlayState* play, Player* player) {
    if (!Active(player))
        return 0;
    if (MustRelease(play, player)) {
        Stop(play, player);
        return 0;
    }
    WolfLinkForm_Draw(play, player);
    return 1;
}

extern "C" void WolfLinkHost_OnUseItem(PlayState* play, Player* player, s32 item) {
    if (Active(player) && FormItem(item))
        Stop(play, player);
}

extern "C" void WolfLinkHost_Destroy(PlayState* play, Player* player) {
    if (sHost.player == player) {
        Stop(play, player);
        sHost = HostState{};
    }
}
