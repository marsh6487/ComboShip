#include "combo/vanilla/ComboVanillaMode.h"
#include "soh/mods/nei_save.h"
#include <cassert>
#include <cstring>
#include <iostream>

extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
int gComboVanillaMode = 1;
}
using u8 = unsigned char;
using s32 = int;
using s8 = signed char;
constexpr int ITEM_NONE = 0xFF, ITEM_FARORES_WIND = 13, ITEM_RITO_MASK = 0xFB;
constexpr int SLOT_FARORES_WIND = 13;
constexpr int ITEM_NAYRUS_LOVE = 19, ITEM_ROCS_FEATHER = 0x9D, SLOT_NAYRUS_LOVE = 19;
constexpr int RAND_INF_OBTAINED_ROCS_FEATHER = 0, RAND_INF_OBTAINED_NAYRUS_LOVE = 1;
static bool randomizerFlags[2]{};
bool Flags_GetRandomizerInf(int flag) {
    return randomizerFlags[flag];
}
#define INV_CONTENT(item) gSaveContext.inventory.items[SLOT_NAYRUS_LOVE]
#define NEI_CANE_SKILL_MAX 6
#define NEI_CANE_TYPE_MAX 4
#define CANE_SKILL_MAX 6
#define CANE_SKILL_TYPE(skill) ((skill) / 3)
#define CANE_SKILL_SLOT(skill) ((skill) % 3)
#define SLOT_CANE_OF_SOMARIA 45
#define ITEM_CANE_OF_SOMARIA 0xAB
#define CANE_TYPE_SOMARIA 0
#define CANE_TYPE_TRIROD 1
#define CANE_TYPE_PACCI 2
#define CANE_SKILL_SOMARIA_STATUE 0
#define CANE_SKILL_SOMARIA_BLOCK 1
#define NA_SE_SY_CURSOR 0
struct Player {};
struct PlayState {};
static int gSfxDefaultPos = 0, gSfxDefaultFreqAndVolScale = 0, gSfxDefaultReverb = 0;
static unsigned cursorSounds = 0;
template <class... Args> void Audio_PlaySoundGeneral(Args...) {
    ++cursorSounds;
}
void Trirod_Cycle(Player*, PlayState*, s8) {
}
static bool mmAssetsAvailable = true;
bool MmAssets_IsAvailable() {
    return mmAssetsAvailable;
}
#define CVAR_EXT_EQUIP_ENABLED "gCheats.ExtEquip.Enabled"
static int optionValue = 0;
int CVarGetInteger(const char*, int) {
    return optionValue;
}
bool MagicMirror_IsOwned() {
    return false;
}
struct CustomFormDef {
    const char* modelName;
    const char* cvar;
    u8 cvarDefault;
};
static CustomFormDef form = { "rito", "gMods.RitoForm", 1 };
const CustomFormDef* CustomForms_ByName(const char*) {
    return &form;
}
static NeiSaveData nei{};
NeiSaveData* Nei_Save() {
    return &nei;
}
void Nei_SetOwnedItem(uint8_t slot, uint16_t value) {
    assert(slot >= 24 && slot < 72);
    nei.ownedItems[slot - 24] = value;
}
struct {
    struct {
        u8 items[24];
    } inventory;
} gSaveContext;
#include "oot_reward_modes.inc"

int main() {
    for (unsigned skill = 0; skill < 6; ++skill) {
        nei = {};
        optionValue = 3;
        assert(Cane_GiveSkill(skill));
        assert(nei.caneSkills == (1u << skill));
        const unsigned expectedType[6] = { 0, 0, 1, 2, 2, 3 };
        const unsigned expectedSkill[6] = { 0, 1, 7, 3, 4, 5 };
        assert(Nei_CaneGetType() == expectedType[skill]);
        assert(Nei_CaneActiveSkill() == expectedSkill[skill]);
        assert(!Cane_GiveSkill(skill) && nei.caneSkills == (1u << skill));
    }
    nei = {};
    nei.caneSkills = (1u << 3) | (1u << 4);
    Nei_CaneSetType(2);
    Nei_CaneSetSkillSlot(2, 1);
    assert(Nei_CaneActiveSkill() == 4);
    Nei_CaneSetSkillSlot(2, 0);
    assert(Nei_CaneActiveSkill() == 3);
    const auto skills = nei.caneSkills;
    Player player;
    PlayState play;
    Cane_CycleSummon(&player, &play, 1);
    assert(Nei_CaneActiveSkill() == 4 && nei.caneSkills == skills);
    Cane_CycleSummon(&player, &play, -1);
    assert(Nei_CaneActiveSkill() == 3 && nei.caneSkills == skills);
    nei.caneSkills = 1u << 4;
    const auto sounds = cursorSounds;
    Cane_CycleSummon(&player, &play, 1);
    assert(Nei_CaneActiveSkill() == 4 && cursorSounds == sounds);
    nei = {};
    optionValue = 0; // All legacy features off, as in an existing user config.
    assert(MmForm_IsEnabled());
    mmAssetsAvailable = false;
    assert(!MmForm_IsEnabled());
    mmAssetsAvailable = true;
    assert(Wand_RandoMode() == WAND_RANDO_ELEMENTAL);
    assert(ExtInv_IsCustomItemsEnabled() && ExtInv_IsMmMasksEnabled());
    assert(ExtEquip_IsEnabled());
    std::memset(gSaveContext.inventory.items, ITEM_NONE, sizeof(gSaveContext.inventory.items));
    VanillaRocsFeather_Grant();
    assert(nei.separateRocsFlags == 1 && INV_CONTENT(ITEM_NAYRUS_LOVE) == ITEM_ROCS_FEATHER);
    VanillaRocsFeather_Grant();
    assert(nei.separateRocsFlags == 1 && INV_CONTENT(ITEM_NAYRUS_LOVE) == ITEM_ROCS_FEATHER);
    nei.separateRocsFlags = 1;
    INV_CONTENT(ITEM_NAYRUS_LOVE) = ITEM_NAYRUS_LOVE;
    assert(Enhancement_GetNextNayrusItem() == ITEM_ROCS_FEATHER);
    assert(nei.separateRocsFlags == 3); // Preserve the actually earned native spell.
    INV_CONTENT(ITEM_NAYRUS_LOVE) = ITEM_ROCS_FEATHER;
    assert(Enhancement_GetPrevNayrusItem() == ITEM_NAYRUS_LOVE);
    nei.separateRocsFlags = 1;
    assert(Enhancement_GetNextNayrusItem() == ITEM_NONE); // Feather cannot invent the spell.
    optionValue = 1;
    assert(!ExtInv_IsOnlyTransformation());
    assert(BombArrows_RandoMode() == BOMB_ARROWS_RANDO_OFF);
    std::memset(gSaveContext.inventory.items, ITEM_NONE, sizeof(gSaveContext.inventory.items));
    optionValue = 1; // Rito used to grant itself solely from its global toggle.
    RitoItem_SyncCell();
    assert((nei.ritoMaskFlags & RITO_FLAG_MASK_OWNED) == 0);
    nei.ritoMaskFlags = RITO_FLAG_MASK_OWNED;
    optionValue = 0;
    assert(GateOn(&form)); // Earned Vanilla ownership survives an off global toggle.
    gComboVanillaMode = 0;
    assert(Wand_RandoMode() == optionValue);
    assert(!ExtInv_IsCustomItemsEnabled() && !ExtInv_IsMmMasksEnabled() && !ExtEquip_IsEnabled());
    nei.ritoMaskFlags = 0;
    optionValue = 1;
    assert(ExtInv_IsOnlyTransformation());
    RitoItem_SyncCell();
    assert(nei.ritoMaskFlags & RITO_FLAG_MASK_OWNED); // Existing seeded/solo behavior.
    gComboGameMode = COMBO_GAME_OOT;
    gComboVanillaMode = 1;
    optionValue = 0;
    assert(Wand_RandoMode() == 0 && !ExtInv_IsCustomItemsEnabled());
    std::cout << "PASS Vanilla earned availability, exact rods, no free Rito, "
                 "seeded and solo controls\n";
}
