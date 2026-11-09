#include "SoulShuffleShared.h"
#include "SoulShuffle.h"
#include "../../../combo/ComboExport.h"
#ifdef COMBO_BUILD
#include "../../../combo/rando/SharedSoulRuntime.h"
#include "../../../combo/ComboResolve.h"
#include "../../../combo/vanilla/ComboVanillaMode.h"
#include "Rando.h"
#include "ActorBehavior/Souls.h"
extern "C" {
#include "variables.h"
#include "functions.h"
extern int gMMComboSharedMask;
void MM_GrantSharedItem(const char* name);
}
namespace {
struct NativeSharedAdapter {
    static ComboRando::GameId Game() {
        return ComboRando::GAME_MM;
    }
    static int Slot() {
        return gSaveContext.fileNum;
    }
    static uint32_t Mask() {
        if (!Combo_IsCombinedGame() || Combo_IsVanilla() || !IS_RANDO || Slot() < 0 || Slot() > 2)
            return 0;
        uint32_t mask = static_cast<uint32_t>(gMMComboSharedMask) & ComboRando::SS_ALL;
        if (!RANDO_SAVE_OPTIONS[RO_SHUFFLE_ENEMY_SOULS])
            mask &= ~ComboRando::SS_ENEMY;
        if (!RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS])
            mask &= ~ComboRando::SS_NPC;
        if (!RANDO_SAVE_OPTIONS[RO_SHUFFLE_ANIMAL_SOULS])
            mask &= ~ComboRando::SS_ANIMAL;
        if (!RANDO_SAVE_OPTIONS[RO_SHUFFLE_MISC_SOULS])
            mask &= ~ComboRando::SS_MISC;
        return mask;
    }
    struct Item {
        int id;
        const char* name;
    };
    static const auto& Items() {
        static const Item items[] = {
#define SHARED_SOUL(category, oot, mm, ootName, mmName) { RI_SOUL_##category##_##mm, mmName },
#include "../../../combo/rando/SharedSoulCatalog.inc"
#undef SHARED_SOUL
        };
        return items;
    }
    static std::string ItemName(int id) {
        for (const auto& item : Items())
            if (item.id == id)
                return item.name;
        return {};
    }
    static std::string LegacyPeerName(int id) {
        return {};
    }
    static bool Owned(const std::string& name) {
        for (const auto& item : Items()) {
            if (name != item.name)
                continue;
            const auto* soul = Rando::SoulShuffle::Find(static_cast<RandoItemId>(item.id));
            if (soul)
                return Flags_GetRandoInf(soul->flag);
            if (item.id >= RI_SOUL_ENEMY_ALIEN && item.id <= RI_SOUL_ENEMY_WOLFOS)
                return Flags_GetRandoInf(SOUL_RI_TO_RANDO_INF(item.id));
            return false;
        }
        return false;
    }
    static void GrantNative(const char* name) {
        MM_GrantSharedItem(name);
    }
    static void GrantPeer(const char* name, int sourceSlot, uint32_t sourceMask) {
        using Grant = void (*)(const char*, int, uint32_t);
        static Grant grant = nullptr;
        if (!grant)
            grant = reinterpret_cast<Grant>(Combo_ResolveSym("soh", "SOH_GrantSharedSoulForSlot"));
        if (grant)
            grant(name, sourceSlot, sourceMask);
    }
};
using Runtime = ComboRando::SharedSoulRuntime<NativeSharedAdapter>;
} // namespace
#endif
extern "C" int MM_ShareSoulReceipt(int item) {
#ifdef COMBO_BUILD
    return Runtime::OnObtained(item);
#else
    return 0;
#endif
}
extern "C" COMBO_EXPORT void MM_GrantSharedSoulForSlot(const char* name, int sourceSlot, uint32_t sourceMask) {
#ifdef COMBO_BUILD
    if (name)
        Runtime::Receive(name, sourceSlot, sourceMask);
#endif
}
extern "C" COMBO_EXPORT const char* MM_ExtractSharedSoulState(void) {
#ifdef COMBO_BUILD
    static std::string state;
    state = Runtime::Extract().dump();
    return state.c_str();
#else
    return "{}";
#endif
}
extern "C" void MM_ApplySharedSoulState(const char* state) {
#ifdef COMBO_BUILD
    if (state) {
        try {
            Runtime::Apply(nlohmann::json::parse(state));
        } catch (...) {}
    }
#endif
}
