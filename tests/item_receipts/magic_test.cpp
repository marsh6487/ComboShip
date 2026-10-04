#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
struct {
    struct { struct { struct { struct { uint8_t magicStatUpgrades; } randomizer; } data; } quest; } ship;
} gSaveContext;
constexpr int RSK_MAGIC_STAT_UPGRADE_ADJUSTABLE = 0, RSK_MAGIC_STAT_UPGRADE_TOTAL = 1,
              RSK_MAGIC_STAT_UPGRADE_REQUIRED = 2, ITEM_CUSTOM = 0;
uint8_t StatUpgradeRequired(int, int, int, int) { return 8; }
struct CustomMessage {
    std::string english;
    CustomMessage() = default;
    CustomMessage(const char* en, const char*, const char*) : english(en) {}
    void Replace(const char* key, const std::string& value) {
        auto pos = english.find(key);
        if (pos != std::string::npos) english.replace(pos, std::string(key).size(), value);
    }
    void AutoFormat(int) {}
};
/* MAGIC_BUILDER */
int main() {
    for (int owned : {0, 2, 7, 8, 254, 255}) {
        gSaveContext.ship.quest.data.randomizer.magicStatUpgrades = owned;
        CustomMessage message;
        BuildMagicStatUpgradeMessage(message);
        assert(message.english.find("%yMagic Meter%w") != std::string::npos);
        if (owned == 0) assert(message.english.find("%c7%w more") != std::string::npos);
        if (owned == 2) assert(message.english.find("%c5%w more") != std::string::npos);
        if (owned == 7) assert(message.english.find("Max magic reached") != std::string::npos);
        if (owned >= 8) assert(message.english.find("Already at max magic") != std::string::npos);
        assert(gSaveContext.ship.quest.data.randomizer.magicStatUpgrades == owned);
    }
    std::cout << "Yellow magic receipt keeps RPG remaining counts and handles saturated tiers\n";
}
