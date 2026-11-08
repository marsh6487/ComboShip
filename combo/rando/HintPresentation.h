#pragma once

#include <string>
#include <string_view>

namespace ComboHintPresentation {
// The cross-game pool stores this placement sentence as plain text. Decorate
// only its known display form, keeping saved text, RNG and tracker rows intact.
inline std::string MmForeignGossip(const std::string& text) {
    std::string plain;
    plain.reserve(text.size());
    constexpr std::string_view colors = "wrgbcpyB";
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '%' && i + 1 < text.size() && colors.find(text[i + 1]) != std::string_view::npos)
            ++i;
        else
            plain += text[i];
    }
    constexpr std::string_view suffix = " (OOT)";
    if (!plain.ends_with(suffix))
        return text;
    for (const std::string_view connector : { " can be found at ", " can be found hoarded in " }) {
        const auto split = plain.find(connector);
        if (split == std::string::npos || split == 0)
            continue;
        const auto location = split + connector.size();
        if (location + suffix.size() >= plain.size())
            continue;
        return "%r" + plain.substr(0, split) + "%w" + std::string(connector) + "%y" + plain.substr(location) + "%w";
    }
    return text;
}
} // namespace ComboHintPresentation
