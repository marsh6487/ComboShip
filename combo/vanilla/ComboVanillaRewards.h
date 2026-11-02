#ifndef COMBO_VANILLA_REWARDS_H
#define COMBO_VANILLA_REWARDS_H

#include "ComboVanillaSave.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace ComboVanilla {
enum class Game { Oot = 0, Mm = 1 };
enum class RewardKind { Item, Cane, Wand, Rune, Season, SeasonRod, Equipment, Upgrade, Retained };
enum class Upgrade {
    Clawshot = 1,
    Ultrashot,
    HammerAxe,
    TrueMasterSword,
    Twilight,
    BombArrows,
    Quartz,
    RitoMask,
    SeparateRocsFeather,
    Net,
    BottomlessBottle,
    MagicCape
};
struct Reward {
    const char* checkId;
    Game game;
    const char* location;
    const char* prerequisite;
    RewardKind kind;
    uint16_t payload;
    uint16_t price;
    const char* label;
};
#define VANILLA_REWARD(id, game, location, prerequisite, kind, payload, price, label) \
    { "vanilla.nei." id,                                                              \
      Game::game,                                                                     \
      location,                                                                       \
      prerequisite,                                                                   \
      RewardKind::kind,                                                               \
      static_cast<uint16_t>(payload),                                                 \
      price,                                                                          \
      label },
#define VANILLA_DEFERRED(id, game, label, reason)
inline constexpr Reward kRewards[] = {
#include "ComboVanillaPlacements.inc"
};
#undef VANILLA_REWARD
#undef VANILLA_DEFERRED
struct DeferredReward {
    const char* checkId;
    Game game;
    const char* label;
    const char* reason;
};
#define VANILLA_REWARD(id, game, location, prerequisite, kind, payload, price, label)
#define VANILLA_DEFERRED(id, game, label, reason) { "vanilla.nei." id, Game::game, label, reason },
inline constexpr DeferredReward kDeferredRewards[] = {
#include "ComboVanillaPlacements.inc"
};
#undef VANILLA_REWARD
#undef VANILLA_DEFERRED

constexpr const Reward* FindReward(std::string_view check) {
    for (const auto& reward : kRewards)
        if (check == reward.checkId)
            return &reward;
    return nullptr;
}
constexpr bool ValidRewardManifest() {
    for (const auto& reward : kRewards) {
        if (!std::string_view(reward.checkId).starts_with("vanilla.nei.") ||
            std::string_view(reward.location).empty() || std::string_view(reward.prerequisite).empty() ||
            std::string_view(reward.label).empty() ||
            (reward.price && reward.price != 40 && reward.price != 60 && reward.price != 80 && reward.price != 90))
            return false;
        if ((reward.kind == RewardKind::Cane || reward.kind == RewardKind::Wand) && reward.payload >= 6)
            return false;
        if (reward.kind == RewardKind::Rune && reward.payload >= 5)
            return false;
        if (reward.kind == RewardKind::Season && reward.payload >= 4)
            return false;
        if (reward.kind == RewardKind::Equipment &&
            ((reward.payload >> 8) >= 4 || (reward.payload & 255) < 1 || (reward.payload & 255) > 3))
            return false;
        for (const auto& other : kRewards)
            if (&reward != &other && std::string_view(reward.checkId) == other.checkId)
                return false;
    }
    return true;
}
static_assert(ValidRewardManifest());

/* Per-DLL setter exports install this launcher callback. Query: -1 invalid,
 * 0 available (including crash-recoverable pending), 1 collected, 2 live busy.
 * Begin/BeginOwned/Commit return 1 only after their durable write succeeds.
 * BeginOwned acknowledges an already-owned alias without any shop debit.
 * The native adapter snapshots/rolls back RAM on failure. It drains OoT saves
 * before Begin and again after granting, then supplies a synchronous final
 * native snapshot to Commit. Failure drains while staging is still active,
 * restores RAM, then aborts; no old worker may outlive either boundary.
 * MM clears stale capture via PrepareCapture and then captures through
 * SaveCurrentForCombo(true) into the active host stage.
 * No staged native write can reach disk without its permanent collection ID. */
enum class RewardAction { Query = 0, Begin = 1, Commit = 2, Abort = 3, BeginOwned = 4, PrepareCapture = 5 };
using RewardHost = int (*)(int action, int game, int slot, const char* checkId, const char* nativeJson);

inline bool RewardMetadataValid(const nlohmann::json& document) {
    if (FileMode(document) != Mode::Vanilla)
        return false;
    const auto& vanilla = document["combo"]["vanilla"];
    return vanilla.contains("collected") && vanilla["collected"].is_object() &&
           (!vanilla.contains("pending") || vanilla["pending"].is_object());
}
inline nlohmann::json RewardRecord(const Reward& reward, bool alreadyOwned) {
    return { { "version", 1 },
             { "game", static_cast<int>(reward.game) },
             { "kind", static_cast<int>(reward.kind) },
             { "payload", reward.payload },
             { "price", alreadyOwned ? 0 : reward.price } };
}
inline bool RewardRecordValid(const nlohmann::json& record, const Reward& reward) {
    if (!record.is_object())
        return false;
    const auto exact = RewardRecord(reward, false);
    for (const auto* key : { "version", "game", "kind", "payload" })
        if (!record.contains(key) || !record[key].is_number_integer() || record[key] != exact[key])
            return false;
    return record.contains("price") && record["price"].is_number_integer() &&
           (record["price"] == 0 || record["price"] == reward.price);
}
inline int RewardStatus(const nlohmann::json& document, const Reward& reward) {
    if (!RewardMetadataValid(document))
        return -1;
    const auto& vanilla = document["combo"]["vanilla"];
    const auto obtained = vanilla["collected"].find(reward.checkId);
    if (obtained != vanilla["collected"].end())
        return RewardRecordValid(*obtained, reward) ? 1 : -1;
    if (vanilla.contains("pending")) {
        const auto pending = vanilla["pending"].find(reward.checkId);
        if (pending != vanilla["pending"].end() && !RewardRecordValid(*pending, reward))
            return -1;
    }
    return 0;
}

inline bool RewardNativeValid(int game, const nlohmann::json& native) {
    if (!native.is_object())
        return false;
    try {
        if (game == static_cast<int>(Game::Oot))
            return native.at("fileType").is_number_integer() && native.at("fileType") == 0 &&
                   native.at("version") == 1 && native.at("sections").at("base").at("data").is_object();
        return game == static_cast<int>(Game::Mm) && native.at("type") == "2S2H_SAVE" &&
               native.at("version").is_number_integer() &&
               native.at("newCycleSave").at("save").at("shipSaveInfo").at("saveType") == 0;
    } catch (const nlohmann::json::exception&) { return false; }
}

/* Launcher-owned under the container mutex. The durable document remains old
 * native state + pending until one atomic replacement contains the final native
 * grant/debit, permanent check and cleared pending receipt together. A failed
 * writer never changes the cache. A process interruption loses only staging. */
class Receipt {
  public:
    template <class Writer>
    bool Begin(nlohmann::json& document, const Reward& reward, bool alreadyOwned, Writer&& writer) {
        if (active || RewardStatus(document, reward) != 0)
            return false;
        auto draft = document;
        draft["combo"]["vanilla"]["pending"][reward.checkId] = RewardRecord(reward, alreadyOwned);
        if (!std::forward<Writer>(writer)(draft))
            return false;
        document = std::move(draft);
        check = reward.checkId;
        game = static_cast<int>(reward.game);
        active = true;
        staged = nullptr;
        return true;
    }
    bool Stage(int sourceGame, const nlohmann::json& native) {
        if (!active || sourceGame != game || !RewardNativeValid(game, native))
            return false;
        staged = native;
        return true;
    }
    void PrepareCapture() {
        staged = nullptr;
    }
    template <class Writer> bool Commit(nlohmann::json& document, Writer&& writer) {
        const auto* reward = FindReward(check);
        if (!active || !reward || !RewardMetadataValid(document) || !RewardNativeValid(game, staged))
            return false;
        const auto& metadata = std::as_const(document)["combo"]["vanilla"];
        if (!metadata.contains("pending"))
            return false;
        const auto& pending = metadata["pending"];
        if (!pending.contains(check) || !RewardRecordValid(pending[check], *reward))
            return false;
        auto draft = document;
        draft[game == 0 ? "oot" : "mm"] = staged;
        auto& vanilla = draft["combo"]["vanilla"];
        vanilla["collected"][check] = vanilla["pending"][check];
        vanilla["pending"].erase(check);
        if (!std::forward<Writer>(writer)(draft))
            return false;
        document = std::move(draft);
        Abort();
        return true;
    }
    void Abort() {
        active = false;
        check.clear();
        staged = nullptr;
    }
    bool Active() const {
        return active;
    }
    bool Matches(int sourceGame, std::string_view sourceCheck) const {
        return active && sourceGame == game && sourceCheck == check;
    }
    int GameId() const {
        return game;
    }

  private:
    bool active = false;
    int game = -1;
    std::string check;
    nlohmann::json staged;
};
} // namespace ComboVanilla
#endif
