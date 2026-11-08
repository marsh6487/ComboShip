#include "global.h"
#include "rando/RpgStats.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

SaveContext gSaveContext;
static int inCutscene;
bool Play_InCsMode(PlayState* play) { return inCutscene; }

/* NATIVE_RECOVERY */

static void Reset(Player* player, PlayState* play) {
    memset(&gSaveContext, 0, sizeof(gSaveContext));
    memset(player, 0, sizeof(*player));
    memset(play, 0, sizeof(*play));
    inCutscene = 0;
    gSaveContext.save.saveInfo.playerData.health = 16;
    gSaveContext.save.saveInfo.playerData.healthCapacity = 160;
    player->transformation = PLAYER_FORM_HUMAN;
    Player_UpdateCouplesMask(player, play); // removal resets the production timer
    player->currentMask = PLAYER_MASK_COUPLE;
}

static void Tick(Player* player, PlayState* play, int frames) {
    while (frames-- > 0) {
        Player_UpdateCouplesMask(player, play);
    }
}

int main(void) {
    Player player;
    PlayState play;
    Reset(&player, &play);
    Tick(&player, &play, 640);
    assert(gSaveContext.save.saveInfo.playerData.health == 160);
    assert(gSaveContext.save.saveInfo.playerData.magic == 0);
    Tick(&player, &play, 50);
    assert(gSaveContext.save.saveInfo.playerData.health == 160);

    // Native and fractional capacities use the same active, initialized meter.
    const int capacities[] = { 0, 1, 12, 24, 36, 48, 60, 72, 84, 96, 100 };
    for (unsigned i = 0; i < sizeof(capacities) / sizeof(capacities[0]); ++i) {
        Reset(&player, &play);
        gSaveContext.save.isNight = 1;
        gSaveContext.magicCapacity = capacities[i];
        gSaveContext.save.saveInfo.playerData.isMagicAcquired = capacities[i] != 0;
        gSaveContext.save.saveInfo.playerData.magicLevel = capacities[i] ? 1 : 0;
        Tick(&player, &play, 800);
        assert(gSaveContext.save.saveInfo.playerData.magic == capacities[i]);
        assert(gSaveContext.magicCapacity == capacities[i]);
        assert(gSaveContext.save.saveInfo.playerData.health == 16);
        assert(player.currentMask == PLAYER_MASK_COUPLE);
        assert(gSaveContext.save.saveInfo.playerData.magicLevel == (capacities[i] ? 1 : 0));
    }

    ComboRpgState stats = {0};
    stats.knownMask = stats.enabledMask = 1 << COMBO_RPG_MAGIC;
    stats.required[COMBO_RPG_MAGIC] = 8;
    for (int total = 96; total <= 100; total += 4) {
        stats.magicTotal = total;
        for (int level = 1; level <= 8; ++level) {
            stats.level[COMBO_RPG_MAGIC] = level;
            Reset(&player, &play);
            gSaveContext.save.isNight = 1;
            gSaveContext.magicCapacity = ComboRpgState_MagicCapacity(&stats, 48);
            gSaveContext.save.saveInfo.playerData.isMagicAcquired = true;
            gSaveContext.save.saveInfo.playerData.magicLevel = 1;
            Tick(&player, &play, 800);
            assert(gSaveContext.save.saveInfo.playerData.magic == gSaveContext.magicCapacity);
        }
    }

    Reset(&player, &play);
    gSaveContext.save.isNight = 1;
    gSaveContext.magicCapacity = 48;
    gSaveContext.save.saveInfo.playerData.isMagicAcquired = true;
    gSaveContext.save.saveInfo.playerData.magicLevel = 1;
    Tick(&player, &play, 1);
    assert(gSaveContext.save.saveInfo.playerData.magic == 1);
    Tick(&player, &play, 6);
    assert(gSaveContext.save.saveInfo.playerData.magic == 1);
    Tick(&player, &play, 1);
    assert(gSaveContext.save.saveInfo.playerData.magic == 2);
    gSaveContext.save.isNight = 0;
    Tick(&player, &play, 8);
    assert(gSaveContext.save.saveInfo.playerData.health > 16);
    assert(gSaveContext.save.saveInfo.playerData.magic == 2);

    for (int blocked = 0; blocked < 9; ++blocked) {
        Reset(&player, &play);
        switch (blocked) {
            case 0: player.currentMask = PLAYER_MASK_NONE; break;
            case 1: player.transformation = PLAYER_FORM_DEKU; break;
            case 2: player.stateFlags1 = PLAYER_STATE1_DEAD; break;
            case 3: gSaveContext.save.saveInfo.playerData.health = 0; break;
            case 4: inCutscene = 1; break;
            case 5: play.pauseCtx.state = PAUSE_STATE_MAIN; break;
            case 6: play.gameOverCtx.state = GAMEOVER_DEATH_START; break;
            case 7: play.transitionTrigger = TRANS_TRIGGER_START; break;
            case 8: play.transitionMode = TRANS_MODE_SETUP; break;
        }
        int health = gSaveContext.save.saveInfo.playerData.health;
        Tick(&player, &play, 800);
        assert(gSaveContext.save.saveInfo.playerData.health == health);
        assert(gSaveContext.save.saveInfo.playerData.magic == 0);
    }

    for (int state = MAGIC_STATE_STEP_CAPACITY; state <= MAGIC_STATE_FILL; ++state) {
        Reset(&player, &play);
        gSaveContext.save.isNight = 1;
        gSaveContext.magicCapacity = 24;
        gSaveContext.save.saveInfo.playerData.isMagicAcquired = true;
        gSaveContext.magicState = state;
        Tick(&player, &play, 800);
        assert(gSaveContext.save.saveInfo.playerData.magic == 0);
    }
    Reset(&player, &play);
    gSaveContext.save.isNight = 1;
    gSaveContext.magicCapacity = 48;
    Tick(&player, &play, 800);
    assert(!gSaveContext.save.saveInfo.playerData.isMagicAcquired);
    assert(gSaveContext.save.saveInfo.playerData.magic == 0);
    puts("PASS Couple's Mask cadence, day/night, native/fractional caps, removal/death and blocked gameplay");
}
