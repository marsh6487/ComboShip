#ifndef COMBO_VANILLA_SAVE_H
#define COMBO_VANILLA_SAVE_H

#include <nlohmann/json.hpp>
#include <utility>

namespace ComboVanilla {
enum class Mode { Invalid = -1, Randomizer = 0, Vanilla = 1 };

inline Mode FileMode(const nlohmann::json& document) {
    const auto combo = document.find("combo");
    if (!document.is_object() || combo == document.end() || !combo->is_object())
        return Mode::Invalid;
    const auto mode = combo->find("mode");
    const auto seed = combo->find("rando");
    const bool hasSeed = seed != combo->end() && !seed->is_null();
    if (mode == combo->end())
        return hasSeed && seed->is_object() && !seed->empty() ? Mode::Randomizer : Mode::Invalid;
    if (!mode->is_string())
        return Mode::Invalid;
    if (*mode == "randomizer")
        return hasSeed && seed->is_object() && !seed->empty() ? Mode::Randomizer : Mode::Invalid;
    if (*mode != "vanilla-nei" || hasSeed)
        return Mode::Invalid;
    const auto vanilla = combo->find("vanilla");
    if (vanilla == combo->end() || !vanilla->is_object())
        return Mode::Invalid;
    const auto version = vanilla->find("version");
    return version != vanilla->end() && version->is_number_integer() && *version == 1 ? Mode::Vanilla : Mode::Invalid;
}

/* The launcher owns this staging object under its existing container mutex.
 * No native half or metadata is written to disk until the complete pair is
 * ready. The writer must retain the old disk document when it reports failure. */
class Creation {
  public:
    bool Begin(int slot, const nlohmann::json& previous) {
        if (active || slot < 0 || slot > 2)
            return false;
        draft = { { "comboVersion", 1 },
                  { "slot", slot },
                  { "oot", nullptr },
                  { "mm", nullptr },
                  { "combo",
                    { { "mode", "vanilla-nei" },
                      { "lastGame", 0 },
                      { "notes", "" },
                      { "vanilla", { { "version", 1 }, { "collected", nlohmann::json::object() } } } } } };
        if (previous.is_object() && previous.contains("comboRelease"))
            draft["comboRelease"] = previous["comboRelease"];
        active = true;
        return true;
    }

    bool Stage(int game, const nlohmann::json& nativeSave) {
        if (!active || game < 0 || game > 1 || !nativeSave.is_object())
            return false;
        draft[game == 0 ? "oot" : "mm"] = nativeSave;
        return true;
    }

    bool Ready() const {
        if (!active || FileMode(draft) != Mode::Vanilla)
            return false;
        const auto& oot = draft["oot"];
        const auto& mm = draft["mm"];
        if (!oot.is_object() || !oot.contains("fileType") || oot["fileType"] != 0 || !oot.contains("version") ||
            !oot.contains("sections") || !oot["sections"].is_object() || !mm.is_object())
            return false;
        try {
            // Native wire values: FILE_TYPE_SAVE_VANILLA and SAVETYPE_VANILLA.
            const auto& base = oot.at("sections").at("base").at("data");
            const auto& ootName = base.at("playerName");
            const auto& mmSave = mm.at("newCycleSave").at("save");
            const auto& mmName = mmSave.at("saveInfo").at("playerData").at("playerName");
            return oot.at("version") == 1 && base.at("healthCapacity").is_number_integer() && ootName.is_array() &&
                   ootName.size() == 8 && mmName.is_array() && mmName.size() == 8 && mm.at("type") == "2S2H_SAVE" &&
                   mm.at("version").is_number_integer() && mmSave.at("shipSaveInfo").at("saveType") == 0;
        } catch (const nlohmann::json::exception&) { return false; }
    }

    template <class Writer> bool Commit(Writer&& writer) {
        if (!Ready() || !std::forward<Writer>(writer)(draft))
            return false;
        active = false;
        return true;
    }

    void Abort() {
        active = false;
        draft = nullptr;
    }
    bool Active() const {
        return active;
    }
    const nlohmann::json& Document() const {
        return draft;
    }

  private:
    bool active = false;
    nlohmann::json draft;
};
} // namespace ComboVanilla

#endif
