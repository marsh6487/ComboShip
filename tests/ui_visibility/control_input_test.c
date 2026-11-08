/* Native-header fixture. The runner inserts the real dispatch and cutscene
 * predicates below; only the shared UI latch and unused callbacks are doubles. */
#include "global.h"
#include "gui/UiVisibility.h"
#include "libultraship/bridge/consolevariablebridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PlayState* gPlayState;
GameState* gGameState;
SaveContext gSaveContext;

static PlayState play;
static PlayState otherPlay;
static Player player;
static int enabled, hidden, debugEnabled, moonJump, conflictOverride;
static unsigned toggles, checks;
static const char* caseName;

#define CHECK(expression)                                                                                      \
    do {                                                                                                       \
        if (!(expression)) {                                                                                   \
            fprintf(stderr, "%s:%d: %s: CHECK(%s) failed\n", __FILE__, __LINE__, caseName, #expression);          \
            exit(1);                                                                                           \
        }                                                                                                      \
    } while (0)

int UiVisibility_IsEnabled(void) {
    return enabled;
}

int UiVisibility_IsHidden(void) {
    return enabled && hidden;
}

void UiVisibility_Toggle(void) {
    if (enabled) {
        hidden ^= 1;
        ++toggles;
    }
}

int32_t CVarGetInteger(const char* name, int32_t fallback) {
    if (!strcmp(name, "gDeveloperTools.DebugEnabled")) {
        return debugEnabled;
    }
    if (!strcmp(name, "gCheats.MoonJumpOnL")) {
        return moonJump;
    }
    if (!strcmp(name, "gDeveloperTools.EnableMapToggle")) {
        return conflictOverride;
    }
    return fallback;
}

/* These callbacks supply real native function-pointer types. The dispatcher
 * must inspect identity/liveness without invoking a game or actor update. */
void Play_Main(GameState* state) {
    CHECK(0 && "The input gate must not execute Play_Main");
}

static void otherMain(GameState* state) {
    CHECK(0 && "The input gate must not execute another state");
}

static void playerUpdate(Actor* actor, PlayState* context) {
    CHECK(0 && "The input gate must not execute the player update");
}

/* PRODUCTION_CONTROL_FUNCTIONS */

static void setPlayer(Player* value) {
#ifdef UI_CONTROL_TEST_MM
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = (Actor*)value;
#else
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = (Actor*)value;
#endif
}

static void setHealth(s16 value) {
#ifdef UI_CONTROL_TEST_MM
    gSaveContext.save.saveInfo.playerData.health = value;
#else
    gSaveContext.health = value;
#endif
}

static void reset(const char* name, int initialHidden) {
    memset(&play, 0, sizeof(play));
    memset(&otherPlay, 0, sizeof(otherPlay));
    memset(&player, 0, sizeof(player));
    memset(&gSaveContext, 0, sizeof(gSaveContext));
    gPlayState = &play;
    gGameState = &play.state;
    play.state.main = Play_Main;
    play.state.running = 1;
    player.actor.update = playerUpdate;
    setPlayer(&player);
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    setHealth(0x30);
    play.state.input[0].cur.button = BTN_L | BTN_CLEFT;
    play.state.input[0].press.button = BTN_L;
    play.state.input[0].prev.button = BTN_CLEFT;
    play.state.input[0].rel.button = BTN_CRIGHT;
    play.state.input[0].cur.stick_x = 23;
    play.state.input[0].cur.stick_y = -17;
    play.state.input[1].press.button = BTN_A;
    enabled = 1;
    hidden = initialHidden;
    debugEnabled = moonJump = conflictOverride = 0;
    toggles = 0;
    caseName = name;
}

static void dispatch(GameState* state, int mayToggle) {
    Input before[ARRAY_COUNT(state->input)];
    int wasHidden = hidden;
    unsigned beforeToggles = toggles;
    memcpy(before, state->input, sizeof(before));
    GameState_UpdateUiVisibility(state);
    CHECK(toggles == beforeToggles + mayToggle);
    CHECK(hidden == (mayToggle ? !wasHidden : wasHidden));
    CHECK(memcmp(before, state->input, sizeof(before)) == 0);
    ++checks;
}

static void checkAllowedControl(int initialHidden) {
    /* Replacing the control gate with a file-number or map-availability check
     * would reject valid normal/debug gameplay. */
    const int slots[] = { 0, 1, 2, 0xFF };
    for (unsigned i = 0; i < ARRAY_COUNT(slots); ++i) {
        reset("normal gameplay and debug save slots", initialHidden);
        gSaveContext.fileNum = slots[i];
        dispatch(&play.state, 1);
    }

    reset("no minimap available", initialHidden);
    play.interfaceCtx.mapRoomNum = -1;
    play.interfaceCtx.minimapAlpha = 0;
    dispatch(&play.state, 1);

    reset("swimming remains player control", initialHidden);
#ifdef UI_CONTROL_TEST_MM
    player.stateFlags1 = PLAYER_STATE1_8000000;
#else
    player.stateFlags1 = PLAYER_STATE1_IN_WATER;
#endif
    dispatch(&play.state, 1);

    reset("shielding remains player control", initialHidden);
#ifdef UI_CONTROL_TEST_MM
    player.stateFlags1 = PLAYER_STATE1_400000;
#else
    player.stateFlags1 = PLAYER_STATE1_SHIELDING;
#endif
    dispatch(&play.state, 1);

    reset("sword use remains player control", initialHidden);
    player.meleeWeaponState = 1;
    dispatch(&play.state, 1);

    reset("unrelated simultaneous button remains available", initialHidden);
    play.state.input[0].press.button |= BTN_R;
    dispatch(&play.state, 1);
}

static void checkInputOwnership(int initialHidden) {
    reset("disabled enhancement preserves chosen latch", initialHidden);
    enabled = 0;
    dispatch(&play.state, 0);

    reset("held L is not a new press", initialHidden);
    play.state.input[0].press.button = 0;
    dispatch(&play.state, 0);

    reset("another controller cannot toggle", initialHidden);
    play.state.input[0].press.button = 0;
    play.state.input[1].press.button = BTN_L;
    dispatch(&play.state, 0);

    reset("opening pause owns simultaneous Start and L", initialHidden);
    play.state.input[0].press.button |= BTN_START;
    dispatch(&play.state, 0);

    reset("inventory page and close-frame L stay in the menu", initialHidden);
    play.pauseCtx.state = 6;
    dispatch(&play.state, 0);
    /* Model the next sampled frame after the menu closed: L remains held,
     * but the original press belonged to inventory. It must not be queued. */
    play.pauseCtx.state = 0;
    play.state.input[0].press.button = 0;
    dispatch(&play.state, 0);
    play.state.input[0].cur.button = 0;
    dispatch(&play.state, 0);
    play.state.input[0].cur.button = BTN_L;
    play.state.input[0].press.button = BTN_L;
    dispatch(&play.state, 1);
    play.state.input[0].press.button = 0;
    dispatch(&play.state, 0);

    reset("dialogue choice L stays in dialogue", initialHidden);
    play.msgCtx.msgMode = MSGMODE_TEXT_DISPLAYING;
    dispatch(&play.state, 0);
    play.msgCtx.msgMode = MSGMODE_NONE;
    play.state.input[0].press.button = 0;
    dispatch(&play.state, 0);
    play.state.input[0].press.button = BTN_L;
    dispatch(&play.state, 1);
}

static void checkStateLifetime(int initialHidden) {
    reset("no live play state", initialHidden);
    gPlayState = NULL;
    dispatch(&play.state, 0);

    reset("different live play state", initialHidden);
    gPlayState = &otherPlay;
    dispatch(&play.state, 0);

    reset("non-play main callback", initialHidden);
    play.state.main = otherMain;
    dispatch(&play.state, 0);

    reset("departing play state", initialHidden);
    play.state.running = 0;
    dispatch(&play.state, 0);

    reset("player not initialized", initialHidden);
    setPlayer(NULL);
    dispatch(&play.state, 0);

    reset("player actor stopped", initialHidden);
    player.actor.update = NULL;
    dispatch(&play.state, 0);

    /* A plain GameState has no pause/player fields. ASan catches any access to
     * PlayState fields before the identity and callback checks reject it. */
    reset("unrelated standalone game state", initialHidden);
    GameState* standalone = calloc(1, sizeof(*standalone));
    CHECK(standalone != NULL);
    standalone->main = Play_Main;
    standalone->running = 1;
    standalone->input[0].press.button = BTN_L;
    dispatch(standalone, 0);

    caseName = "non-play allocation with stale play pointer";
    standalone->main = otherMain;
    gPlayState = (PlayState*)standalone;
    gGameState = standalone;
    dispatch(standalone, 0);
    gPlayState = NULL;
    gGameState = NULL;
    free(standalone);

    const int nonPlayModes[] = { GAMEMODE_TITLE_SCREEN, GAMEMODE_FILE_SELECT, GAMEMODE_END_CREDITS };
    for (unsigned i = 0; i < ARRAY_COUNT(nonPlayModes); ++i) {
        reset("non-gameplay mode despite valid player", initialHidden);
        gSaveContext.gameMode = nonPlayModes[i];
        dispatch(&play.state, 0);
    }
}

static void checkSuspendedControl(int initialHidden) {
#ifdef UI_CONTROL_TEST_MM
    const unsigned lastPauseState = PAUSE_STATE_UNPAUSE_CLOSE;
    const unsigned lastDebugState = DEBUG_EDITOR_EVENTS;
#else
    const unsigned lastPauseState = 0x13; /* Native OoT unpause-close state. */
    const unsigned lastDebugState = 2;
#endif
    for (unsigned state = 1; state <= lastPauseState; ++state) {
        reset("all native pause phases reject L", initialHidden);
        play.pauseCtx.state = state;
        dispatch(&play.state, 0);
    }
    for (unsigned state = 1; state <= lastDebugState; ++state) {
        reset("debug pause editor rejects L", initialHidden);
#ifdef UI_CONTROL_TEST_MM
        play.pauseCtx.debugEditor = state;
#else
        play.pauseCtx.debugState = state;
#endif
        dispatch(&play.state, 0);
    }

    reset("frame advance suspends control", initialHidden);
    play.frameAdvCtx.enabled = 1;
    dispatch(&play.state, 0);

    reset("closing textbox still owns control", initialHidden);
    play.msgCtx.msgMode = MSGMODE_TEXT_CLOSING;
    dispatch(&play.state, 0);

    const int gameOverStates[] = { GAMEOVER_DEATH_START, GAMEOVER_REVIVE_START, GAMEOVER_REVIVE_FADE_OUT };
    for (unsigned i = 0; i < ARRAY_COUNT(gameOverStates); ++i) {
        reset("death and revival own control", initialHidden);
        play.gameOverCtx.state = gameOverStates[i];
        dispatch(&play.state, 0);
    }

    const int transitionTriggers[] = { TRANS_TRIGGER_START, TRANS_TRIGGER_END };
    for (unsigned i = 0; i < ARRAY_COUNT(transitionTriggers); ++i) {
        reset("pending entrance or exit transition", initialHidden);
        play.transitionTrigger = transitionTriggers[i];
        dispatch(&play.state, 0);
    }
    reset("active transition without pending trigger", initialHidden);
    play.transitionMode = TRANS_MODE_INSTANCE_RUNNING;
    dispatch(&play.state, 0);

    reset("actor freeze flash", initialHidden);
    play.actorCtx.freezeFlashTimer = 1;
    dispatch(&play.state, 0);

    reset("player freeze timer", initialHidden);
    player.actor.freezeTimer = 1;
    dispatch(&play.state, 0);

    reset("zero health before dead flag", initialHidden);
    setHealth(0);
    dispatch(&play.state, 0);
}

static void checkNativeCutscenePredicates(int initialHidden) {
    /* These are individual real engine states, not a stubbed inCutscene flag.
     * Dropping Play_InCsMode from the dispatch must fail these cases. */
#ifdef UI_CONTROL_TEST_MM
    const u32 blockedFlags[] = { PLAYER_STATE1_DEAD, PLAYER_STATE1_200, PLAYER_STATE1_20000000,
                                 PLAYER_STATE1_1, PLAYER_STATE1_20 };
#else
    const u32 blockedFlags[] = { PLAYER_STATE1_DEAD, PLAYER_STATE1_IN_CUTSCENE,
                                 PLAYER_STATE1_LOADING, PLAYER_STATE1_INPUT_DISABLED };
#endif
    for (unsigned i = 0; i < ARRAY_COUNT(blockedFlags); ++i) {
        reset("native player cutscene/input-disabled flag", initialHidden);
        player.stateFlags1 = blockedFlags[i];
        dispatch(&play.state, 0);
    }

    for (unsigned state = 1; state <= 4; ++state) {
        reset("native cutscene context state", initialHidden);
        play.csCtx.state = state;
        dispatch(&play.state, 0);
    }

    reset("native scripted player action", initialHidden);
    player.csAction = PLAYER_CSACTION_5;
    dispatch(&play.state, 0);

    reset("hookshot is carrying the player", initialHidden);
    player.stateFlags3 = PLAYER_STATE3_FLYING_WITH_HOOKSHOT;
    dispatch(&play.state, 0);

    reset("native special player control state", initialHidden);
#ifdef UI_CONTROL_TEST_MM
    player.unk_AA5 = PLAYER_UNKAA5_5;
#else
    player.unk_6AD = 4;
#endif
    dispatch(&play.state, 0);

#ifdef UI_CONTROL_TEST_MM
    reset("scripted input replaces the physical controller", initialHidden);
    play.actorCtx.isOverrideInputOn = true;
    play.actorCtx.overrideInput.press.button = BTN_L;
    dispatch(&play.state, 0);
#else
    for (unsigned spell = 0; spell < 6; ++spell) {
        reset("native magic spell controls the player", initialHidden);
        gSaveContext.magicState = MAGIC_STATE_CONSUME;
        player.itemAction = PLAYER_IA_MAGIC_SPELL_15 + spell;
        dispatch(&play.state, 0);
    }
    reset("idle magic does not block a selected spell", initialHidden);
    player.itemAction = PLAYER_IA_MAGIC_SPELL_15;
    dispatch(&play.state, 1);

    reset("magic meter activity without spell remains controllable", initialHidden);
    gSaveContext.magicState = MAGIC_STATE_CONSUME_LENS;
    player.itemAction = PLAYER_IA_NONE;
    dispatch(&play.state, 1);

    /* Developer overrides resolve the existing L conflict, but cannot grant
     * gameplay ownership while a menu, dialogue, or cutscene owns input. */
    for (unsigned conflicts = 1; conflicts <= 3; ++conflicts) {
        reset("native debug/moon-jump conflict", initialHidden);
        debugEnabled = (conflicts & 1) != 0;
        moonJump = (conflicts & 2) != 0;
        dispatch(&play.state, 0);
        conflictOverride = 1;
        dispatch(&play.state, 1);
        play.pauseCtx.state = 6;
        dispatch(&play.state, 0);
        play.pauseCtx.state = 0;
        play.msgCtx.msgMode = MSGMODE_TEXT_DISPLAYING;
        dispatch(&play.state, 0);
        play.msgCtx.msgMode = MSGMODE_NONE;
        player.stateFlags1 = PLAYER_STATE1_IN_CUTSCENE;
        dispatch(&play.state, 0);
    }
#endif
}

int main(void) {
    for (int initialHidden = 0; initialHidden <= 1; ++initialHidden) {
        checkAllowedControl(initialHidden);
        checkInputOwnership(initialHidden);
        checkStateLifetime(initialHidden);
        checkSuspendedControl(initialHidden);
        checkNativeCutscenePredicates(initialHidden);
    }
#ifdef UI_CONTROL_TEST_MM
    const char* host = "MM";
#else
    const char* host = "OoT";
#endif
#ifdef COMBO_BUILD
    const char* mode = "Combo";
#else
    const char* mode = "original";
#endif
    printf("PASS %s %s: %u native control dispatches; real cutscene predicates, both UI latch states, input preserved\n",
           host, mode, checks);
    return 0;
}
