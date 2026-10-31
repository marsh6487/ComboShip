#pragma once
#include <cstddef>
#include <cstdint>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using s16 = int16_t;
using s32 = int32_t;

constexpr u8 ITEM_NONE = 0xFF, SLOT_NONE = 0xFF, ITEM_EXT_BUTTON = 0xFE;
constexpr u8 ITEM_STICK = 0, SLOT_STICK = 0, ITEM_BOW = 3, SLOT_BOW = 3;
constexpr u8 ITEM_ARROW_FIRE = 4, ITEM_ARROW_ICE = 12, ITEM_ARROW_LIGHT = 18;
constexpr u8 ITEM_BOW_ARROW_FIRE = 0x38, ITEM_BOW_ARROW_ICE = 0x39, ITEM_BOW_ARROW_LIGHT = 0x3A;
constexpr u8 ITEM_BOW_FIRE = 0x4A;
constexpr int LINK_AGE_CHILD = 1;
constexpr u16 PAUSE_ITEM_NONE = 999, ITEM_SOLD_OUT = 0x2C, ITEM_LENS = 15, ITEM_BOMB = 2;
constexpr u16 BTN_B = 0x4000, BTN_CLEFT = 2, BTN_CDOWN = 4, BTN_CRIGHT = 1, BTN_DUP = 0x800, BTN_DDOWN = 0x400,
              BTN_DLEFT = 0x200, BTN_DRIGHT = 0x100;
constexpr u8 BTN_DISABLED = 0xFF;
constexpr u16 ITEM_MM_LETTER_KAFEI = 0x7F, ITEM_MM_SPECIAL_DELIVERY = 0x7E;
constexpr u8 SLOT_ARROW_LIGHT = 18;
constexpr int EQUIP_SLOT_C_LEFT = 1, EQUIP_SLOT_C_DOWN = 2, EQUIP_SLOT_C_RIGHT = 3;
constexpr int EQUIP_SLOT_D_RIGHT = 0, EQUIP_SLOT_D_LEFT = 1, EQUIP_SLOT_D_DOWN = 2, EQUIP_SLOT_D_UP = 3;
constexpr int PAUSE_EQUIP_C_LEFT = 0, PAUSE_EQUIP_C_DOWN = 1, PAUSE_EQUIP_C_RIGHT = 2;
constexpr int PAUSE_EQUIP_D_RIGHT = 3, PAUSE_EQUIP_D_LEFT = 4, PAUSE_EQUIP_D_DOWN = 5, PAUSE_EQUIP_D_UP = 6;

struct PauseContext {
    u16 equipTargetItem = 0;
    u8 equipTargetSlot = SLOT_NONE;
    int equipTargetCBtn = 0;
};
struct PlayState { PauseContext pauseCtx; };
struct ExtButtons { u16 items[8]{}; };
struct SaveContext {
    u8 buttonStatus[9]{};
    struct {
        struct {
            u8 status[4]{};
        } dpad;
    } shipSaveContext;
    struct {
        u8 buttonItems[8]{};
        u8 cButtonSlots[7]{};
    } equips;
    struct { ExtButtons extButtons, childExtButtons, adultExtButtons; } ship;
    struct {
        struct {
            struct { u8 buttonItems[5][4]{}; u8 cButtonSlots[5][3]{}; } equips;
        } saveInfo;
        struct {
            struct { u16 items[5][4]{}; } extButtons;
            struct { u8 items[5][4]{}; u8 slots[5][4]{}; u16 extItems[5][4]{}; } dpadEquips;
            struct { u16 ownedItems[24]{}; } nei;
        } shipSaveInfo;
    } save;
};
extern "C" SaveContext gSaveContext;

#ifdef MIRROR_FIXTURE_MM
#define EXT_BUTTON_ITEM(form, btn) (gSaveContext.save.shipSaveInfo.extButtons.items[form][btn])
#else
#define EXT_BUTTON_ITEM(btn) (gSaveContext.ship.extButtons.items[btn])
#endif
#define BUTTON_ITEM_EQUIP(form, btn) (gSaveContext.save.saveInfo.equips.buttonItems[form][btn])
#define C_SLOT_EQUIP(form, btn) (gSaveContext.save.saveInfo.equips.cButtonSlots[form][(btn) - 1])
#define DPAD_BUTTON_ITEM_EQUIP(form, btn) (gSaveContext.save.shipSaveInfo.dpadEquips.items[form][btn])
#define DPAD_SLOT_EQUIP(form, btn) (gSaveContext.save.shipSaveInfo.dpadEquips.slots[form][btn])

void Interface_LoadItemIcon1(PlayState*, u16);
void Interface_LoadItemIcon2(PlayState*, u16);
void Interface_LoadItemIconImpl(PlayState*, u8);
void Interface_Dpad_LoadItemIconImpl(PlayState*, s32);
int CVarGetInteger(const char*, int);
#define CVAR_ENHANCEMENT(key) key
#define ARRAY_COUNT(value) (sizeof(value) / sizeof((value)[0]))
