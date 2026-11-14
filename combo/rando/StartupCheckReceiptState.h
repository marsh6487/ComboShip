#ifndef COMBO_STARTUP_CHECK_RECEIPT_STATE_H
#define COMBO_STARTUP_CHECK_RECEIPT_STATE_H

#include "StartupCheckReceipt.h"
#include <cstring>
#include <limits>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <type_traits>

namespace ComboStartupReceipt {
inline bool ValidText(const char* text, size_t size, bool required) {
    return text && std::memchr(text, '\0', size) && (!required || text[0] != '\0');
}

inline bool Valid(const CwStartupReceipt& receipt) {
    if (receipt.version != CW_STARTUP_RECEIPT_VERSION || receipt.itemGame < 0 || receipt.itemGame > 1 ||
        (receipt.state != CW_STARTUP_READY && receipt.state != CW_STARTUP_WAIT_MM) ||
        (receipt.state == CW_STARTUP_WAIT_MM && receipt.itemGame != 1) || receipt.category < 0 ||
        receipt.category > 6 || receipt.importance < CW_STARTUP_JUNK || receipt.importance > CW_STARTUP_MAJOR ||
        receipt.dungeonInfo < 0 || receipt.dungeonInfo > 1 || receipt.trap < 0 || receipt.trap > 1 ||
        !ValidText(receipt.itemName, sizeof(receipt.itemName), true) ||
        !ValidText(receipt.displayName, sizeof(receipt.displayName), receipt.state == CW_STARTUP_READY))
        return false;
    const int32_t unsignedFields[] = { receipt.modIndex, receipt.getItemId, receipt.itemId, receipt.tableId,
                                       receipt.objectId, receipt.field,     receipt.gid,    receipt.drawModIndex };
    for (int32_t value : unsignedFields)
        if (value < 0 || value > std::numeric_limits<uint16_t>::max())
            return false;
    // MM custom items and traps use native GID_NONE (-1); their concrete item
    // ID, not this optional draw ID, selects the native presentation recipe.
    const int32_t minimumDrawId = receipt.itemGame == 1 ? -1 : 0;
    if (receipt.drawItemId < minimumDrawId || receipt.drawItemId > std::numeric_limits<uint16_t>::max())
        return false;
    return receipt.gi >= std::numeric_limits<int16_t>::min() && receipt.gi <= std::numeric_limits<int16_t>::max();
}

inline nlohmann::json Encode(const CwStartupReceipt& r) {
    return { { "version", r.version },
             { "generation", r.generation },
             { "state", r.state },
             { "itemGame", r.itemGame },
             { "modIndex", r.modIndex },
             { "getItemId", r.getItemId },
             { "itemId", r.itemId },
             { "tableId", r.tableId },
             { "objectId", r.objectId },
             { "field", r.field },
             { "gi", r.gi },
             { "gid", r.gid },
             { "drawItemId", r.drawItemId },
             { "drawModIndex", r.drawModIndex },
             { "category", r.category },
             { "importance", r.importance },
             { "dungeonInfo", r.dungeonInfo },
             { "trap", r.trap },
             { "itemName", r.itemName },
             { "displayName", r.displayName },
             { "source", CW_STARTUP_SOURCE },
             { "shown", false } };
}

inline bool Decode(const nlohmann::json& value, CwStartupReceipt& out) {
    try {
        CwStartupReceipt r{};
        if (!value.is_object() || value.at("source") != CW_STARTUP_SOURCE)
            return false;
        auto read = [&](const char* key, auto& field) {
            const auto& entry = value.at(key);
            if (!entry.is_number_integer())
                throw std::invalid_argument("startup receipt integer required");
            if (entry.is_number_unsigned() && entry.get<uint64_t>() > std::numeric_limits<int64_t>::max())
                throw std::out_of_range("startup receipt unsigned integer range");
            const int64_t number = entry.get<int64_t>();
            using Value = std::remove_reference_t<decltype(field)>;
            if (number < std::numeric_limits<Value>::min() || number > std::numeric_limits<Value>::max())
                throw std::out_of_range("startup receipt integer range");
            field = static_cast<Value>(number);
        };
        read("version", r.version);
        read("generation", r.generation);
        read("state", r.state);
        read("itemGame", r.itemGame);
        read("modIndex", r.modIndex);
        read("getItemId", r.getItemId);
        read("itemId", r.itemId);
        read("tableId", r.tableId);
        read("objectId", r.objectId);
        read("field", r.field);
        read("gi", r.gi);
        read("gid", r.gid);
        read("drawItemId", r.drawItemId);
        read("drawModIndex", r.drawModIndex);
        read("category", r.category);
        read("importance", r.importance);
        read("dungeonInfo", r.dungeonInfo);
        read("trap", r.trap);
        const auto name = value.at("itemName").get<std::string>();
        const auto display = value.at("displayName").get<std::string>();
        if (name.size() >= sizeof(r.itemName) || display.size() >= sizeof(r.displayName) ||
            name.find('\0') != std::string::npos || display.find('\0') != std::string::npos ||
            !value.at("shown").is_boolean())
            return false;
        std::memcpy(r.itemName, name.c_str(), name.size() + 1);
        std::memcpy(r.displayName, display.c_str(), display.size() + 1);
        if (!r.generation || !Valid(r))
            return false;
        out = r;
        return true;
    } catch (const nlohmann::json::exception&) { return false; } catch (const std::logic_error&) {
        return false;
    }
}

inline const nlohmann::json* Record(const nlohmann::json& document) {
    if (!document.is_object())
        return nullptr;
    const auto combo = document.find("combo");
    if (combo == document.end() || !combo->is_object())
        return nullptr;
    const auto receipt = combo->find("startupReceipt");
    return receipt == combo->end() ? nullptr : &*receipt;
}

inline bool BeginFile(nlohmann::json& document, uint32_t generation) {
    if (!generation || (!document.is_null() && !document.is_object()) ||
        (document.contains("combo") && !document["combo"].is_object() && !document["combo"].is_null()))
        return false;
    auto& combo = document["combo"];
    if (combo.is_null())
        combo = nlohmann::json::object();
    combo.erase("startupReceipt");
    combo["startupReceiptGeneration"] = generation;
    return true;
}

inline bool Capture(nlohmann::json& document, CwStartupReceipt receipt) {
    if (!Valid(receipt) || Record(document) || !document.is_object())
        return false;
    const auto metadata = document.find("combo");
    if (metadata == document.end() || !metadata->is_object())
        return false;
    const auto& combo = *metadata;
    const auto generation = combo.find("startupReceiptGeneration");
    if (generation == combo.end() || !generation->is_number_integer() ||
        (generation->is_number_unsigned() && generation->get<uint64_t>() > std::numeric_limits<uint32_t>::max()) ||
        generation->get<int64_t>() <= 0 || generation->get<int64_t>() > std::numeric_limits<uint32_t>::max())
        return false;
    receipt.generation = generation->get<uint32_t>();
    if (!receipt.generation)
        return false;
    document["combo"]["startupReceipt"] = Encode(receipt);
    return true;
}

inline bool ReadPending(const nlohmann::json& document, int state, CwStartupReceipt& out) {
    const auto* value = Record(document);
    CwStartupReceipt r{};
    if (!value || !Decode(*value, r) || r.state != state || value->at("shown").get<bool>())
        return false;
    out = r;
    return true;
}

inline bool Peek(const nlohmann::json& document, CwStartupReceipt& out) {
    return ReadPending(document, CW_STARTUP_READY, out);
}

inline bool PendingMm(const nlohmann::json& document, CwStartupReceipt& out) {
    return ReadPending(document, CW_STARTUP_WAIT_MM, out);
}

inline bool Acknowledge(nlohmann::json& document, uint32_t generation) {
    CwStartupReceipt receipt{};
    if (!Peek(document, receipt) || receipt.generation != generation)
        return false;
    document["combo"]["startupReceipt"]["shown"] = true;
    return true;
}

inline bool CompleteMm(nlohmann::json& document, const CwStartupReceipt& receipt, const nlohmann::json& mm) {
    CwStartupReceipt pending{};
    if (!PendingMm(document, pending) || !Valid(receipt) || receipt.state != CW_STARTUP_READY ||
        receipt.itemGame != 1 || receipt.generation != pending.generation || !mm.is_object())
        return false;
    document["mm"] = mm;
    document["combo"]["startupReceipt"] = Encode(receipt);
    return true;
}

// In-process completion keeps the awarded MM snapshot away from the durable
// pending check until both can be committed in one atomic container write.
// A failed write retries this result, never the native grant.
struct MmCompletion {
    CwStartupReceipt receipt{};
    nlohmann::json nativeSave;
    nlohmann::json nativeOotSave;
    bool peerCaptureArmed = false;
    bool peerCaptureComplete = false;
    bool running = false;

    bool Begin(const nlohmann::json& document) {
        return PendingMm(document, receipt);
    }
    bool Ready() const {
        return receipt.state == CW_STARTUP_READY && receipt.itemGame == 1 && Valid(receipt);
    }
    bool Matches(const nlohmann::json& document) const {
        CwStartupReceipt pending{};
        return PendingMm(document, pending) && pending.generation == receipt.generation;
    }
    bool Stage(const nlohmann::json& mm) {
        if (!mm.is_object())
            return false;
        nativeSave = mm;
        return true;
    }
    bool StagePeer(const nlohmann::json& oot) {
        if (!oot.is_object())
            return false;
        nativeOotSave = oot;
        peerCaptureComplete = peerCaptureArmed;
        return true;
    }
    void PreparePeerCapture() {
        nativeOotSave = nullptr;
        peerCaptureArmed = true;
        peerCaptureComplete = false;
    }
    template <typename Writer> bool Commit(nlohmann::json& document, Writer&& write) {
        if (!Ready() || !nativeSave.is_object() || !nativeOotSave.is_object() || !peerCaptureComplete)
            return false;
        auto draft = document;
        if (!CompleteMm(draft, receipt, nativeSave))
            return false;
        draft["oot"] = nativeOotSave;
        if (!write(draft))
            return false;
        document = std::move(draft);
        return true;
    }
};
} // namespace ComboStartupReceipt
#endif
