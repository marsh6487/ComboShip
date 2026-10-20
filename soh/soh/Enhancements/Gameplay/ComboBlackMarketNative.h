#pragma once
#ifdef COMBO_BUILD
struct PlayState;
struct Actor;
enum {
    COMBO_BLACK_MARKET_OOT_DOOR_UNHANDLED,
    COMBO_BLACK_MARKET_OOT_DOOR_ENTERED,
    COMBO_BLACK_MARKET_OOT_DOOR_BLOCKED,
};
#ifdef __cplusplus
extern "C" {
#endif
void ComboBlackMarketOot_NormalizeEntry(void);
void ComboBlackMarketOot_ReportClear(int boss, int slot);
int ComboBlackMarketOot_TryDoorEntrance(struct PlayState* play, struct Actor* door);
#ifdef __cplusplus
}
#endif
#endif
