#pragma once

#include <string>
#include <string_view>

namespace ComboHintPresentation {
// Older saved direct-message hints can contain a plain placement sentence
// rather than an authored template. Emphasize only that known grammar at
// display time; existing colored/obscure prose is returned unchanged.
inline std::string PlainPlacement(const std::string& text) {
    // Formatted native commands and authored hashtag spans carry their own
    // presentation. Do not parse their operands as placement sentence text.
    for (unsigned char c : text)
        if (c < 0x20 || c == '#')
            return text;
    constexpr std::string_view colors = "wrgbcpyB";
    for (size_t i = 0; i + 1 < text.size(); ++i)
        if (text[i] == '%' && colors.find(text[i + 1]) != std::string_view::npos)
            return text;
    size_t begin = 0;
    for (std::string_view prefix : { "They say that ", "They say ", "Wise choice... They say " })
        if (text.starts_with(prefix)) {
            begin = prefix.size();
            break;
        }
    for (std::string_view connector : { " can be found at ", " can be found hoarded in ", " is hidden " }) {
        const auto split = text.find(connector, begin);
        if (split == std::string::npos || split == begin || split + connector.size() >= text.size())
            continue;
        return text.substr(0, begin) + "%g" + text.substr(begin, split - begin) + "%w" + std::string(connector) + "%y" +
               text.substr(split + connector.size()) + "%w";
    }
    return text;
}

// The cross-game pool stores this placement sentence as plain text. Decorate
// only its known display form, keeping saved text, RNG and tracker rows intact.
inline std::string MmForeignGossip(const std::string& text) {
    for (unsigned char c : text)
        if (c < 0x20)
            return text;
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
        return PlainPlacement(text);
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
