#define main wolf_core_fixture_main
#include "core_runtime_test.cpp"
#undef main
#include "mods/ext_buttons/ext_buttons.h"
#include "mods/extended_inventory.h"
#include "mods/nei_save.h"
#include "mods/forms/custom_forms.h"

static s32 otherForm = CUSTOM_FORM_NONE;
static u8 pendant, beetle, kite, trident, mario, customItemBlock, pakModel, o2rModel;
static unsigned nativeActions, nativeDamageTicks, iceBursts, thawCompletions;
static bool nativeAnimationDone;
extern "C" {
Input* sPlayerControlInput;
f32 sControlStickMagnitude;
s16 sControlStickAngle;
NeiSaveData* Nei_Save(void) {
    return &gSaveContext.save.shipSaveInfo.nei;
}
s32 CustomForms_ActiveForm(void) {
    return otherForm;
}
u8 ExtEquip_PendantActive(void) {
    return pendant;
}
u8 Beetle_IsFlying(void) {
    return beetle;
}
u8 KiteSurf_IsActive(void) {
    return kite;
}
u8 Trident_OwnsPlayerAction(void) {
    return trident;
}
u8 Sm64Mario_IsActive(void) {
    return mario;
}
s32 CustomItems_BlocksMovement(Player*) {
    return customItemBlock;
}
u8 PakLoader_HasActiveModel(void) {
    return pakModel;
}
u8 O2rLoader_HasActiveModel(void) {
    return o2rModel;
}
void Player_Action_20(Player*, PlayState*) {
    ++nativeActions;
}
void Player_Action_21(Player*, PlayState*) {
    ++nativeActions;
}
void Player_Action_83(Player*, PlayState*) {
    ++nativeActions;
}
void NativeIdle(Player*, PlayState*) {
    ++nativeActions;
}
s32 Player_InflictDamage(PlayState*, s32 damage) {
    ++nativeDamageTicks;
    gSaveContext.save.saveInfo.playerData.health += damage;
    return 1;
}
void func_80836988(Player* player, PlayState*) {
    ++thawCompletions;
    player->actionFunc = NativeIdle;
}
void func_808339B4(Player* player, s32 timer) {
    player->invincibilityTimer = timer;
}
void func_80834104(PlayState*, Player*) {
    assert(false);
}
s32 PlayerAnimation_Update(PlayState*, SkelAnime*) {
    return nativeAnimationDone;
}
void EffectSsIcePiece_SpawnBurst(PlayState*, Vec3f*, f32) {
    ++iceBursts;
}
void Player_Action_82(Player*, PlayState*);
}
#ifdef MM_WOLF_HOST
#include "mods/forms/wolf_link_host.cpp"
#else
static void WolfLinkHost_PreUpdate(PlayState*, Player*) {
}
static void WolfLinkHost_FilterInput(Player*, Input*) {
}
static void WolfLinkHost_BeforeAction(PlayState*, Player*, Input*) {
}
static u8 WolfLinkHost_Draw(PlayState*, Player*) {
    return 0;
}
static void WolfLinkHost_Destroy(PlayState*, Player*) {
}
#endif

int main(int argc, char** argv) {
    assert(argc == 2);
    assetDirectory = argv[1];
    std::memset(&gSaveContext, 0, sizeof(gSaveContext));
    std::memset(gSaveContext.save.saveInfo.equips.buttonItems, ITEM_NONE,
                sizeof(gSaveContext.save.saveInfo.equips.buttonItems));
    gSaveContext.save.saveInfo.playerData.health = 48;
    Nei_Save()->ownedItems[SLOT_SHADOW_CRYSTAL - 24] = EXT_ITEM_SHADOW_CRYSTAL;
    PlayState play{};
    Player player{};
    Camera camera{};
    GraphicsContext gfx{};
    Gfx commands[128]{};
    play.state.gfxCtx = &gfx;
    play.cameraPtrs[0] = &camera;
    gPlayState = &play;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    player.transformation = PLAYER_FORM_HUMAN;
    player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    player.cylinder.dim.radius = 12;
    player.cylinder.dim.height = 50;
    player.actor.shape.shadowDraw = ActorShadow_DrawFeet;
    R_UPDATE_RATE = 3;
    ExtButton_SetItem(0, EQUIP_SLOT_C_LEFT, EXT_ITEM_SHADOW_CRYSTAL);
    assert(load(makeAsset()));
    sAssetsLoaded = 0;
    auto frame = [&](u16 buttons = 0, u16 held = 0, s8 stick = 0) {
        play.state.input[0] = {};
        play.state.input[0].press.button = buttons;
        play.state.input[0].cur.button = held;
        play.state.input[0].rel.stick_y = stick;
        play.state.input[0].cur.stick_y = stick;
        WolfLinkHost_PreUpdate(&play, &player);
        Input filtered = play.state.input[0];
        WolfLinkHost_FilterInput(&player, &filtered);
        sPlayerControlInput = &filtered;
        WolfLinkHost_BeforeAction(&play, &player, &filtered);
        if (!(player.stateFlags3 & PLAYER_STATE3_4) && player.actionFunc)
            player.actionFunc(&player, &play);
        ++play.gameplayFrames;
        return filtered;
    };
    frame(BTN_CLEFT);
    if (!WolfLinkForm_IsSelected() || !WolfLinkForm_IsReady()) {
        std::fputs("FAIL full-width C-button Shadow Crystal did not activate the production MM Wolf runtime\n", stderr);
        return 1;
    }
    auto filtered = frame(BTN_B, BTN_R);
    assert(!(filtered.press.button & BTN_B) && !(filtered.cur.button & BTN_R));
    assert(sWolf.procOwnsPlayer && sWolf.proc == PROC_WOLF_WAIT_ATTACK);
    // Native collision-shape fitting cannot overwrite Wolf's low body.
#ifdef MM_WOLF_HOST
    player.cylinder.dim.height = 80;
    WolfLinkHost_ApplyCollisionShape(&player);
    assert(player.cylinder.dim.height == 30);
#endif
    // Knockback/electricity pointers remain native; fixture boundary bodies count dispatches.
    for (auto action : { Player_Action_20, Player_Action_21, Player_Action_83 }) {
        player.actionFunc = action;
        player.stateFlags1 = 0;
        player.invincibilityTimer = 0;
        const auto before = nativeActions;
        const auto health = gSaveContext.save.saveInfo.playerData.health;
        frame(BTN_A | BTN_B, 0, 60);
        assert(nativeActions == before + 1 && player.actionFunc == action);
        assert(!sWolf.procOwnsPlayer && !sWolf.atActive && !(player.stateFlags3 & PLAYER_STATE3_4));
        assert(gSaveContext.save.saveInfo.playerData.health == health);
    }
    // Actual native freeze action + actual mash helper are extracted unchanged from z_player.c.
    // They must tick periodic native damage once, thaw on A/B mashing, and finish their animation.
    player.actionFunc = Player_Action_82;
    player.av1.actionVar1 = 6;
    player.av2.actionVar2 = 0;
    const auto damageBefore = nativeDamageTicks;
    const auto healthBefore = gSaveContext.save.saveInfo.playerData.health;
    play.gameplayFrames = 40;
    for (int i = 0; i < 4; ++i)
        frame();
    assert(nativeDamageTicks == damageBefore + 1 && gSaveContext.save.saveInfo.playerData.health == healthBefore - 1);
    unsigned freezeFrames = 0;
    while (player.av1.actionVar1 >= 0 && freezeFrames++ < 20) {
        filtered = frame(BTN_A | BTN_B);
        assert((filtered.press.button & (BTN_A | BTN_B)) == (BTN_A | BTN_B));
        assert(!sWolf.procOwnsPlayer && !sWolf.atActive && !(player.stateFlags3 & PLAYER_STATE3_4));
    }
    assert(player.av1.actionVar1 == -1 && iceBursts == 1);
    const auto thawHealth = gSaveContext.save.saveInfo.playerData.health;
    for (int i = 0; i < 3; ++i) {
        frame(BTN_B);
        assert(player.actionFunc == Player_Action_82 && !sWolf.procOwnsPlayer);
    }
    assert(gSaveContext.save.saveInfo.playerData.health == thawHealth && thawCompletions == 0);
    nativeAnimationDone = true;
    frame(BTN_B);
    assert(player.actionFunc == NativeIdle && thawCompletions == 1 && player.invincibilityTimer == 20 &&
           !sWolf.procOwnsPlayer);
    nativeAnimationDone = false;
    player.actionFunc = nullptr;
    Actor npc{};
    player.talkActor = &npc;
    player.stateFlags2 |= PLAYER_STATE2_CAN_ACCEPT_TALK_OFFER;
    filtered = frame(BTN_A, 0, 60);
    assert(filtered.press.button & BTN_A);
    assert(!sWolf.procOwnsPlayer);
    player.talkActor = nullptr;
    player.stateFlags2 = 0;
    gfx.polyOpa.p = commands;
    gfx.polyOpa.d = commands + 128;
    assert(WolfLinkHost_Draw(&play, &player));
    frame(BTN_CLEFT);
    assert(!WolfLinkForm_IsReady() && !(player.stateFlags3 & PLAYER_STATE3_4));
    // Full-width D-pad store is independent from the three shared C slots.
    ExtButton_ClearItem(0, EQUIP_SLOT_C_LEFT);
    ExtButton_SetDpadItem(0, EQUIP_SLOT_D_UP, EXT_ITEM_SHADOW_CRYSTAL);
    frame(BTN_DUP);
    assert(!WolfLinkForm_IsReady());
    integerCvars["gEnhancements.Dpad.DpadEquips"] = 1;
    frame(BTN_DUP);
    assert(WolfLinkForm_IsReady());
    frame(BTN_DUP);
    assert(!WolfLinkForm_IsReady());
    // Native mask press reaches the existing item path and releases Wolf before that path runs.
    ExtButton_SetItem(0, EQUIP_SLOT_C_LEFT, EXT_ITEM_SHADOW_CRYSTAL);
    frame(BTN_CLEFT);
    assert(WolfLinkForm_IsReady());
    BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_RIGHT) = ITEM_MASK_DEKU;
    filtered = frame(BTN_CRIGHT);
    assert(!WolfLinkForm_IsReady() && (filtered.press.button & BTN_CRIGHT));
    // Active tool/other-form arbitration must never silently steal their input/action.
    for (u8* blocker : { &pendant, &beetle, &kite, &trident, &mario, &customItemBlock, &pakModel, &o2rModel }) {
        *blocker = 1;
        frame(BTN_CLEFT);
        assert(!WolfLinkForm_IsReady());
        *blocker = 0;
    }
    otherForm = CUSTOM_FORM_GARO;
    frame(BTN_CLEFT);
    assert(!WolfLinkForm_IsReady());
    otherForm = CUSTOM_FORM_NONE;
    player.stateFlags3 = PLAYER_STATE3_FLYING_WITH_HOOKSHOT;
    frame(BTN_CLEFT);
    assert(!WolfLinkForm_IsReady());
    player.stateFlags3 = 0;
    frame(BTN_CLEFT);
    assert(WolfLinkForm_IsReady());
    player.transformation = PLAYER_FORM_GORON;
    frame();
    assert(!WolfLinkForm_IsReady());
    player.transformation = PLAYER_FORM_HUMAN;
    frame(BTN_CLEFT);
    assert(WolfLinkForm_IsReady());
    play.transitionTrigger = TRANS_TRIGGER_START;
    frame();
    assert(!WolfLinkForm_IsReady());
    play.transitionTrigger = TRANS_TRIGGER_OFF;
    frame(BTN_CLEFT);
    assert(WolfLinkForm_IsReady());
    player.stateFlags1 = PLAYER_STATE1_DEAD;
    frame();
    assert(!WolfLinkForm_IsReady());
    player.stateFlags1 = 0;
    frame(BTN_CLEFT);
    assert(WolfLinkForm_IsReady());
    ++play.sceneId;
    frame();
    assert(!WolfLinkForm_IsReady());
    frame(BTN_CLEFT);
    assert(WolfLinkForm_IsReady());
    WolfLinkHost_Destroy(&play, &player);
    assert(!WolfLinkForm_IsReady() && !sWolf.atCylInit && !(player.stateFlags3 & PLAYER_STATE3_4));
    // A malformed replacement on a fresh load retains the normal player's draw, input and shape.
    sAssetsLoaded = 0;
    auto bad = makeAsset();
    write32(bad, field(bad, 6), 0x7fc00001);
    assert(!load(bad));
    filtered = frame(BTN_CLEFT | BTN_B);
    assert(!WolfLinkForm_IsReady() && !WolfLinkHost_Draw(&play, &player) && (filtered.press.button & BTN_B));
    assert(player.cylinder.dim.radius == 12 && player.cylinder.dim.height == 50 &&
           player.actor.shape.shadowDraw == ActorShadow_DrawFeet);
    std::remove((assetDirectory + "/wolf_link.bin").c_str());
    sAssetsLoaded = 0;
    filtered = frame(BTN_CLEFT | BTN_B);
    assert(!WolfLinkForm_IsReady() && (filtered.press.button & BTN_B));
    std::puts("PASS production MM Wolf host: full-width C/D-pad toggles, native damage/freeze/thaw action dispatch, "
              "input/tool arbitration, rendering and teardown");
}
