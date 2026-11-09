#pragma once

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace ComboRando {
// A diagnostic is not a usable native pool or metadata snapshot. Keep this
// check shared by generation, replay and the native metadata consumers.
inline nlohmann::json ParseStaticDataDump(const std::string& dumpJson, const char* game) {
    auto dump = nlohmann::json::parse(dumpJson);
    if (!dump.is_object())
        throw std::runtime_error(std::string(game) + ": native static data is not an object");
    if (dump.contains("error"))
        throw std::runtime_error(std::string(game) + ": " + dump.at("error").get<std::string>());
    return dump;
}

// Older callers can omit optional metadata. A supplied failure is authoritative.
inline nlohmann::json ParseOptionalStaticDataDump(const std::string& dumpJson, const char* game) {
    return dumpJson.empty() ? nlohmann::json::object() : ParseStaticDataDump(dumpJson, game);
}
} // namespace ComboRando
