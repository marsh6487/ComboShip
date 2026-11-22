#pragma once

#include <libultraship/bridge/consolevariablebridge.h>

namespace ComboRando::SongEvents {

enum class GlyphDisplay : int32_t { GlyphAndSongName = 0, GlyphOnly = 1 };

inline constexpr char kGlyphDisplayCvar[] = "gRandoEnhancements.SongEventGlyphDisplay";
inline constexpr char kGlyphDisplayLabel[] = "Song Event Marker";
inline constexpr char kGlyphDisplayTooltip[] =
    "Choose whether to show the song name beside each shuffled song glyph. Changes apply immediately.";
inline constexpr char kGlyphAndSongNameLabel[] = "Glyph and Song Name";
inline constexpr char kGlyphOnlyLabel[] = "Glyph Only";

inline bool ShowSongEventName() {
    return CVarGetInteger(kGlyphDisplayCvar, static_cast<int32_t>(GlyphDisplay::GlyphAndSongName)) !=
           static_cast<int32_t>(GlyphDisplay::GlyphOnly);
}

} // namespace ComboRando::SongEvents
