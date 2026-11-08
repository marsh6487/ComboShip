#include "../../combo/gui/UiVisibility.h"
#include "global.h"
#include "soh/Enhancements/cosmetics/cosmeticsTypes.h"
#include "tests/test_require.h"
#include <string.h>
static int armed, hidden = 1, debugEnabled, moonJump, conflictOverride, minimal;
static unsigned toggles;
static GameInfo gameInfo;
GameInfo *gGameInfo = &gameInfo;
PlayState *gPlayState;
SaveContext gSaveContext;
void Play_Main(GameState *state) {}
static void playerUpdate(Actor *actor, PlayState *play) {}
int UiVisibility_IsEnabled(void) { return armed; }
int UiVisibility_IsHidden(void) { return armed && hidden; }
void UiVisibility_Toggle(void) {
  REQUIRE(armed);
  hidden = !hidden;
  ++toggles;
}
int32_t CVarGetInteger(const char *name, int32_t fallback) {
  if (!strcmp(name, "gDeveloperTools.DebugEnabled"))
    return debugEnabled;
  if (!strcmp(name, "gCheats.MoonJumpOnL"))
    return moonJump;
  if (!strcmp(name, "gDeveloperTools.EnableMapToggle"))
    return conflictOverride;
  if (!strcmp(name, "gEnhancements.MinimalUI"))
    return minimal;
  return fallback;
}
int Play_InCsMode(PlayState *play) {
  return play->csCtx.state != CS_STATE_IDLE;
}
/* PRODUCTION_INPUT */
/* PRODUCTION_FULL_UI */
/* PRODUCTION_MAP_L */
/* PRODUCTION_MAP_VISIBILITY */
int main(void) {
  static PlayState play;
  static Player player;
  gPlayState = &play;
  play.state.main = Play_Main;
  play.state.running = 1;
  play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
  player.actor.update = playerUpdate;
  gSaveContext.health = 0x30;
  gSaveContext.gameMode = GAMEMODE_NORMAL;
  play.state.input[0].press.button = BTN_L;
  for (unsigned state = 0; state < 3; ++state) {
    /* L belongs to page navigation/dialogue outside normal player control. */
    play.pauseCtx.state = state == 1 ? 6 : 0;
    play.msgCtx.msgMode = state == 2 ? MSGMODE_TEXT_DISPLAYING : MSGMODE_NONE;
    R_MINIMAP_DISABLED = 1;
    armed = 0;
    unsigned before = toggles;
    GameState_UpdateUiVisibility(&play.state);
    REQUIRE(toggles == before && R_MINIMAP_DISABLED == 1);
    armed = 1;
    hidden = 1;
    GameState_UpdateUiVisibility(&play.state);
    unsigned accepted = state == 0 ? 1 : 0;
    REQUIRE(toggles == before + accepted);
    REQUIRE(UiVisibility_IsHidden() == (state != 0));
    REQUIRE(R_MINIMAP_DISABLED == 1);
    REQUIRE(play.state.input[0].press.button == BTN_L);
    GameState_UpdateUiVisibility(&play.state);
    REQUIRE(UiVisibility_IsHidden() && toggles == before + 2 * accepted);
    play.state.input[0].press.button = 0;
    play.state.input[0].cur.button = BTN_L;
    GameState_UpdateUiVisibility(&play.state);
    REQUIRE(toggles == before + 2 * accepted);
    play.state.input[0].press.button = BTN_L;
  }
  GameState fileSelect = {0};
  fileSelect.input[0].press.button = BTN_L;
  unsigned before = toggles;
  GameState_UpdateUiVisibility(&fileSelect);
  REQUIRE(toggles == before);
  play.pauseCtx.state = 0;
  play.msgCtx.msgMode = MSGMODE_NONE;
  for (unsigned conflicts = 1; conflicts <= 3; ++conflicts) {
    debugEnabled = (conflicts & 1) != 0;
    moonJump = (conflicts & 2) != 0;
    conflictOverride = 0;
    before = toggles;
    GameState_UpdateUiVisibility(&play.state);
    REQUIRE(toggles == before);
    conflictOverride = 1;
    GameState_UpdateUiVisibility(&play.state);
    REQUIRE(toggles == before + 1);
  }
  debugEnabled = moonJump = conflictOverride = 0;
  play.csCtx.state = CS_STATE_IDLE;
  play.pauseCtx.state = 0;
  minimal = 1;
  for (unsigned enabled = 0; enabled < 2; ++enabled) {
    armed = enabled;
    for (unsigned mapDisabled = 0; mapDisabled < 2; ++mapDisabled) {
      R_MINIMAP_DISABLED = mapDisabled;
      REQUIRE(FullUi(&play) == (enabled || !mapDisabled));
      REQUIRE(MapVisible0() == (enabled || !mapDisabled));
      REQUIRE(MapVisible1() == (enabled || !mapDisabled));
      REQUIRE(LegacyMapL0(&play, true) == !enabled);
      REQUIRE(LegacyMapL1(&play, true) == !enabled);
      REQUIRE(!LegacyMapL0(&play, false) && !LegacyMapL1(&play, false));
      REQUIRE(R_MINIMAP_DISABLED == mapDisabled);
    }
  }
  armed = 0;
  minimal = 0;
  R_MINIMAP_DISABLED = 1;
  REQUIRE(FullUi(&play));
  minimal = 1;
  play.pauseCtx.state = 6;
  REQUIRE(FullUi(&play));
  play.csCtx.state = CS_STATE_UNSKIPPABLE_EXEC;
  REQUIRE(!LegacyMapL0(&play, true) && !LegacyMapL1(&play, true));
  puts("PASS OoT L input: player control only, held/conflicting buttons, full-UI "
       "priority and no legacy double-toggle");
}
