/* The production gate/getters/command branches run unchanged. These structures
 * provide their accessed fields, not the engine ABI. Actor spawning and action
 * starts are observed at the engine boundary; no gameplay is simulated. */
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef int32_t s32;
#include "production_constants.inc"

typedef struct { float x, y, z; } Vec3f;
typedef struct { struct { Vec3f pos; } focus; } Actor;
typedef struct { Actor actor; s16 timer; } EnBom;
typedef struct {
    Actor actor;
    u8 currentMask, transformation;
    s16 blastMaskTimer;
} Player;
typedef struct {
    ActionLabel* doActionSegment;
    s16 bButtonPlayerDoActionActive, bButtonPlayerDoAction;
} InterfaceContext;
typedef struct { InterfaceContext interfaceCtx; Player* player; int actorCtx; } PlayState;
#define GET_PLAYER(play) ((play)->player)
static struct {
    struct {
        u8 playerForm;
        struct {
            struct { u8 isMagicAcquired; } playerData;
            struct { u8 buttonItems[4][4]; } equips;
        } saveInfo;
    } save;
    u8 bButtonStatus, buttonStatus[6];
} gSaveContext;
static char actionLabels[DO_ACTION_MAX][1];
static char* doActionTbl[DO_ACTION_MAX];
static EnBom bomb;
static unsigned spawned, marched, danced, checks, failed;
static bool spawnFails, riding, ultrahand;
static int hookItemOverride = -1;
enum { VB_GET_ITEM_ON_BUTTON = 1, VB_SET_BLAST_MASK_COOLDOWN_TIMER,
       ACTOR_EN_BOM, BOMB_EXPLOSIVE_TYPE_BOMB, BOMB_TYPE_BODY };
static u8 Pacci_UltrahandModeActive(void) { return ultrahand; }
static u8 MasterCycle_IsRiding(void) { return riding; }
static int GameInteractor_Should(int event, int original, ...) {
    va_list args;
    va_start(args, original);
    if (event == VB_GET_ITEM_ON_BUTTON) {
        int slot = va_arg(args, int);
        ItemId* item = va_arg(args, ItemId*);
        if (slot != EQUIP_SLOT_B || *item != (ItemId)original)
            abort();
        if (hookItemOverride >= 0)
            *item = (ItemId)hookItemOverride;
    } else if (event != VB_SET_BLAST_MASK_COOLDOWN_TIMER) {
        abort();
    }
    va_end(args);
    return original;
}
static Actor* Actor_Spawn(int* context, PlayState* play, int id, float x, float y, float z,
                         int rx, int ry, int rz, int params) {
    if (context != &play->actorCtx || id != ACTOR_EN_BOM || rx != BOMB_EXPLOSIVE_TYPE_BOMB ||
        ry || rz || params != BOMB_TYPE_BODY || x != 1.0f || y != 2.0f || z != 3.0f)
        abort();
    ++spawned;
    return spawnFails ? NULL : &bomb.actor;
}
static void func_80839978(PlayState* play, Player* player) { (void)play; (void)player; ++marched; }
static void func_80839A10(PlayState* play, Player* player) { (void)play; (void)player; ++danced; }
/* C-button lookup is not used by these B/A fixtures. */
static ItemId Stub_CBtnItem(EquipSlot slot) { (void)slot; abort(); }
#define C_BTN_ITEM(btn) Stub_CBtnItem(btn)
static u16 ExtButton_GetItem(int form, EquipSlot slot) { (void)form; (void)slot; abort(); }
static u8 ExtEquip_IsButtonItem(u16 item) { (void)item; abort(); }
static u16 ExtEquip_GetLegacyButtonItem(u16 item) { (void)item; abort(); }
#include "production_functions.inc"

static void Check(bool condition, const char* test, int item, int detail) {
    ++checks;
    if (!condition) {
        ++failed;
        if (failed <= 12)
            fprintf(stderr, "FAIL %s (B=0x%02X, detail=%d)\n", test, item, detail);
    }
}
static void Reset(PlayState* play, Player* player, ActionLabel* labels, int form, int item, int mask) {
    memset(&gSaveContext, 0, sizeof(gSaveContext));
    memset(play, 0, sizeof(*play));
    memset(player, 0, sizeof(*player));
    memset(labels, 0, sizeof(ActionLabel) * DO_ACTION_SEG_MAX);
    player->actor.focus.pos = (Vec3f){ 1.0f, 2.0f, 3.0f };
    player->transformation = form;
    player->currentMask = mask;
    play->player = player;
    play->interfaceCtx.doActionSegment = labels;
    play->interfaceCtx.bButtonPlayerDoAction = DO_ACTION_CHECK;
    play->interfaceCtx.bButtonPlayerDoActionActive = true;
    gSaveContext.save.playerForm = form;
    gSaveContext.save.saveInfo.playerData.isMagicAcquired = true;
    BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = item;
    gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_ENABLED;
    gSaveContext.bButtonStatus = BTN_ENABLED;
    spawned = marched = danced = 0;
    spawnFails = riding = ultrahand = false;
    hookItemOverride = -1;
    bomb.timer = 99;
}

int main(void) {
    for (int i = 0; i < DO_ACTION_MAX; ++i) doActionTbl[i] = actionLabels[i];
    PlayState play;
    Player player;
    ActionLabel labels[DO_ACTION_SEG_MAX];
    const struct { int item; bool allowed; } items[] = {
        { ITEM_SWORD_KOKIRI, true }, { ITEM_SWORD_RAZOR, true }, { ITEM_SWORD_GILDED, true },
        { ITEM_NONE, true }, { ITEM_DEKU_NUT, true },
        { ITEM_SWORD_MASTER, true }, { ITEM_SWORD_BGS, true }, { ITEM_SWORD_GREAT_FAIRY, true },
        { ExtEquip_GetItemId(EQUIP_TYPE_SWORD, 1), true },
        { ExtEquip_GetItemId(EQUIP_TYPE_SWORD, 2), true },
        { ExtEquip_GetItemId(EQUIP_TYPE_SWORD, 3), true },
        { ITEM_BOW, false }, { ITEM_BOMB, false }, { ITEM_BOMBCHU, false },
        { ITEM_OCARINA_OF_TIME, false }, { ITEM_NET, false }, { ITEM_ROD_FIRE, false },
        { ITEM_CANE_OF_SOMARIA, false }, { ITEM_SWORD_KNIFE, false },
        { ITEM_SWORD_DEITY, false }, { ITEM_EXT_SHIELD_1, false }, { ITEM_EXT_BUTTON, false },
        { ITEM_FD, false }, { ITEM_FE, false },
    };
    const struct { int mask, action, command; } masks[] = {
        { PLAYER_MASK_BLAST, DO_ACTION_EXPLODE, ITEM_F0 },
        { PLAYER_MASK_BREMEN, DO_ACTION_MARCH, ITEM_F1 },
        { PLAYER_MASK_KAMARO, DO_ACTION_DANCE, ITEM_F2 },
    };
    for (unsigned i = 0; i < sizeof(items) / sizeof(items[0]); ++i) {
        for (unsigned m = 0; m < sizeof(masks) / sizeof(masks[0]); ++m) {
            for (int disabled = 0; disabled < 3; ++disabled) {
                Reset(&play, &player, labels, PLAYER_FORM_HUMAN, items[i].item, masks[m].mask);
                if (disabled == 1) gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_DISABLED;
                if (disabled == 2) gSaveContext.bButtonStatus = BTN_DISABLED;
                u8 equipmentBefore[4][4];
                memcpy(equipmentBefore, gSaveContext.save.saveInfo.equips.buttonItems, sizeof(equipmentBefore));
                Interface_SetBButtonPlayerDoAction(&play, masks[m].action);
                ItemId command = Player_GetItemOnButton(&play, &player, EQUIP_SLOT_B);
                RunNativeMaskCommand(&play, &player, command);
                bool dispatch = items[i].allowed && !disabled;
                Check(play.interfaceCtx.bButtonPlayerDoAction == (items[i].allowed ? masks[m].action : 0),
                      "mask prompt", items[i].item, masks[m].action);
                Check(play.interfaceCtx.bButtonPlayerDoActionActive == items[i].allowed,
                      "prompt active", items[i].item, masks[m].action);
                Check(labels[DO_ACTION_SEG_B].subTex == (items[i].allowed ? doActionTbl[masks[m].action] : NULL),
                      "prompt label", items[i].item, masks[m].action);
                Check(disabled ? command == ITEM_NONE :
                      (items[i].allowed ? (int)command == masks[m].command : (int)command == items[i].item),
                      "B item to mask command", items[i].item, disabled);
                Check(spawned == (unsigned)(dispatch && m == 0) && marched == (unsigned)(dispatch && m == 1) &&
                      danced == (unsigned)(dispatch && m == 2), "native mask dispatch", items[i].item, disabled);
                Check(memcmp(equipmentBefore, gSaveContext.save.saveInfo.equips.buttonItems,
                             sizeof(equipmentBefore)) == 0,
                      "equipped identities unchanged", items[i].item, masks[m].action);
                if (dispatch && m == 0)
                    Check(bomb.timer == 0 && player.blastMaskTimer == 310, "blast timer", items[i].item, 0);
            }
        }
    }
    /* Added sword support is limited to human mask actions. Native transformed
     * buttons retain their existing action and Deku's no-magic sentinel. */
    const int extraSwords[] = { ITEM_SWORD_MASTER, ITEM_SWORD_BGS, ITEM_SWORD_GREAT_FAIRY,
                               ITEM_EXT_SWORD_1, ITEM_EXT_SWORD_2, ITEM_EXT_SWORD_3 };
    for (unsigned i = 0; i < sizeof(extraSwords) / sizeof(extraSwords[0]); ++i) {
        for (int form = PLAYER_FORM_FIERCE_DEITY; form < PLAYER_FORM_HUMAN; ++form) {
            Reset(&play, &player, labels, form, extraSwords[i], PLAYER_MASK_NONE);
            Interface_SetBButtonPlayerDoAction(&play, DO_ACTION_EXPLODE);
            Check(!play.interfaceCtx.bButtonPlayerDoActionActive && play.interfaceCtx.bButtonPlayerDoAction == 0,
                  "nonhuman extra sword", extraSwords[i], form);
            Reset(&play, &player, labels, PLAYER_FORM_HUMAN, extraSwords[i], PLAYER_MASK_BLAST);
            player.transformation = form;
            Interface_SetBButtonPlayerDoAction(&play, DO_ACTION_EXPLODE);
            Check(!play.interfaceCtx.bButtonPlayerDoActionActive && play.interfaceCtx.bButtonPlayerDoAction == 0,
                  "actor form owns added action", extraSwords[i], form);
        }
        Reset(&play, &player, labels, PLAYER_FORM_HUMAN, extraSwords[i], PLAYER_MASK_NONE);
        Interface_SetBButtonPlayerDoAction(&play, DO_ACTION_PUNCH);
        Check(!play.interfaceCtx.bButtonPlayerDoActionActive, "unrelated action unchanged", extraSwords[i], 0);
    }
    const struct { int form, item, action; } nativeForms[] = {
        { PLAYER_FORM_GORON, ITEM_SWORD_RAZOR, DO_ACTION_PUNCH },
        { PLAYER_FORM_ZORA, ITEM_SWORD_KOKIRI, DO_ACTION_PUNCH },
        { PLAYER_FORM_DEKU, ITEM_DEKU_NUT, DO_ACTION_SHOOT },
    };
    for (unsigned i = 0; i < sizeof(nativeForms) / sizeof(nativeForms[0]); ++i) {
        Reset(&play, &player, labels, nativeForms[i].form, nativeForms[i].item, PLAYER_MASK_NONE);
        Interface_SetBButtonPlayerDoAction(&play, nativeForms[i].action);
        Check(play.interfaceCtx.bButtonPlayerDoAction == nativeForms[i].action &&
              play.interfaceCtx.bButtonPlayerDoActionActive, "native form action", nativeForms[i].item, nativeForms[i].form);
    }
    Reset(&play, &player, labels, PLAYER_FORM_DEKU, ITEM_DEKU_NUT, PLAYER_MASK_NONE);
    gSaveContext.save.saveInfo.playerData.isMagicAcquired = false;
    Interface_SetBButtonPlayerDoAction(&play, DO_ACTION_SHOOT);
    Check(play.interfaceCtx.bButtonPlayerDoAction == 0xFD, "Deku no-magic sentinel", ITEM_DEKU_NUT, 0);
    Reset(&play, &player, labels, PLAYER_FORM_FIERCE_DEITY, ITEM_SWORD_DEITY, PLAYER_MASK_FIERCE_DEITY);
    Interface_SetBButtonPlayerDoAction(&play, DO_ACTION_EXPLODE);
    Check(!play.interfaceCtx.bButtonPlayerDoActionActive, "Deity sword unchanged", ITEM_SWORD_DEITY, 0);

    Reset(&play, &player, labels, PLAYER_FORM_HUMAN, ITEM_SWORD_MASTER, PLAYER_MASK_BLAST);
    Interface_SetBButtonPlayerDoAction(&play, DO_ACTION_EXPLODE);
    player.blastMaskTimer = 1;
    RunNativeMaskCommand(&play, &player, Player_GetItemOnButton(&play, &player, EQUIP_SLOT_B));
    Check(spawned == 0 && player.blastMaskTimer == 1, "cooldown blocks repeat", ITEM_SWORD_MASTER, 0);
    player.blastMaskTimer = 0;
    spawnFails = true;
    RunNativeMaskCommand(&play, &player, Player_GetItemOnButton(&play, &player, EQUIP_SLOT_B));
    Check(spawned == 1 && player.blastMaskTimer == 0, "failed spawn keeps cooldown clear", ITEM_SWORD_MASTER, 0);
    spawnFails = false;
    spawned = 0;
    RunNativeMaskCommand(&play, &player, Player_GetItemOnButton(&play, &player, EQUIP_SLOT_A));
    Check(spawned == 0, "A is not a mask command", ITEM_SWORD_MASTER, 0);
    riding = true;
    Check(Player_GetItemOnButton(&play, &player, EQUIP_SLOT_B) == ITEM_NONE, "bike retains B", ITEM_SWORD_MASTER, 0);
    riding = false;
    ultrahand = true;
    Check(Player_GetItemOnButton(&play, &player, EQUIP_SLOT_B) == ITEM_NONE, "Ultrahand retains B", ITEM_SWORD_MASTER, 0);
    ultrahand = false;
    hookItemOverride = ITEM_NONE;
    RunNativeMaskCommand(&play, &player, Player_GetItemOnButton(&play, &player, EQUIP_SLOT_B));
    Check(spawned == 0, "item hook can deny mask action", ITEM_SWORD_MASTER, 0);
    hookItemOverride = -1;
    player.currentMask = PLAYER_MASK_NONE;
    RunNativeMaskCommand(&play, &player, Player_GetItemOnButton(&play, &player, EQUIP_SLOT_B));
    Check(spawned == 0, "mask ownership required", ITEM_SWORD_MASTER, 0);
    printf("%u checks, %u failures; production mask gate/getters/commands, external engine boundaries substituted.\n",
           checks, failed);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
