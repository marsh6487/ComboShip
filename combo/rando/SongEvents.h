#pragma once
// Stable song/event identities follow OoTMM 0c0f8a8e. Host adapters own native usability.
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>
namespace ComboRando::SongEvents {
enum class Game { Oot, Mm };
enum OotEvent {
    OOT_TEMPLE_OF_TIME,
    OOT_WINDMILL,
    OOT_GRAVEYARD,
    OOT_ZORA_RIVER,
    OOT_GORON_CITY,
    OOT_GREAT_FAIRY_SPELL_WIND,
    OOT_GREAT_FAIRY_SPELL_FIRE,
    OOT_GREAT_FAIRY_SPELL_LOVE,
    OOT_GREAT_FAIRY_UPGRADE_MAGIC,
    OOT_GREAT_FAIRY_UPGRADE_MAGIC2,
    OOT_GREAT_FAIRY_UPGRADE_DEFENSE,
    OOT_TEMPLE_WATER,
    OOT_TEMPLE_SHADOW,
    OOT_TEMPLE_SPIRIT_STATUE,
    OOT_TEMPLE_SPIRIT_LOWER,
    OOT_TEMPLE_SPIRIT_HIGHER,
    OOT_TEMPLE_BOTW,
    OOT_TEMPLE_GANON,
};
enum MmEvent {
    MM_TEMPLE_WOODFALL,
    MM_TEMPLE_SNOWHEAD,
    MM_TEMPLE_GREATBAY,
    MM_HEALING_POEHUT,
    MM_HEALING_DARMANI,
    MM_HEALING_PAMELA_FATHER,
    MM_HEALING_KAMARO,
    MM_HEALING_MIKAU,
    MM_AWAKENING_KEETA,
    MM_AWAKENING_SCRUB,
    MM_LULLABY_KID,
    MM_STORMS_COMPOSER,
    MM_CLOCK_TOWER_ROOF,
};
inline constexpr const char* Songs[] = {
    "ZELDAS_LULLABY",
    "EPONAS",
    "SARIAS",
    "STORMS",
    "SUNS",
    "TIME",
    "MINUET",
    "BOLERO",
    "SERENADE",
    "REQUIEM",
    "NOCTURNE",
    "PRELUDE",
    "HEALING",
    "SOARING",
    "SONATA",
    "GORON_LULLABY",
    "GORON_LULLABY_INTRO",
    "NEW_WAVE",
    "ELEGY",
    "OATH",
};
inline constexpr const char* OotNames[] = {
    "TEMPLE_OF_TIME",
    "WINDMILL",
    "GRAVEYARD",
    "ZORA_RIVER",
    "GORON_CITY",
    "GREAT_FAIRY_SPELL_WIND",
    "GREAT_FAIRY_SPELL_FIRE",
    "GREAT_FAIRY_SPELL_LOVE",
    "GREAT_FAIRY_UPGRADE_MAGIC",
    "GREAT_FAIRY_UPGRADE_MAGIC2",
    "GREAT_FAIRY_UPGRADE_DEFENSE",
    "TEMPLE_WATER",
    "TEMPLE_SHADOW",
    "TEMPLE_SPIRIT_STATUE",
    "TEMPLE_SPIRIT_LOWER",
    "TEMPLE_SPIRIT_HIGHER",
    "TEMPLE_BOTW",
    "TEMPLE_GANON",
};
inline constexpr const char* MmNames[] = {
    "TEMPLE_WOODFALL",       "TEMPLE_SNOWHEAD", "TEMPLE_GREATBAY",  "HEALING_POEHUT",  "HEALING_DARMANI",
    "HEALING_PAMELA_FATHER", "HEALING_KAMARO",  "HEALING_MIKAU",    "AWAKENING_KEETA", "AWAKENING_SCRUB",
    "LULLABY_KID",           "STORMS_COMPOSER", "CLOCK_TOWER_ROOF",
};
inline constexpr const char* SongLabels[] = {
    "Zelda's Lullaby",    "Epona's Song",        "Saria's Song",        "Song of Storms",     "Sun's Song",
    "Song of Time",       "Minuet of Forest",    "Bolero of Fire",      "Serenade of Water",  "Requiem of Spirit",
    "Nocturne of Shadow", "Prelude of Light",    "Song of Healing",     "Song of Soaring",    "Sonata of Awakening",
    "Goron Lullaby",      "Goron Lullaby Intro", "New Wave Bossa Nova", "Elegy of Emptiness", "Oath to Order",
};
inline constexpr const char* OotLabels[] = {
    "Door of Time",
    "Kakariko Windmill",
    "Royal Family's Tomb",
    "Zora's River Waterfall",
    "Darunia's Door",
    "Farore's Wind Fairy",
    "Din's Fire Fairy",
    "Nayru's Love Fairy",
    "Death Mountain Magic Fairy",
    "Crater Double Magic Fairy",
    "Castle Defense Fairy",
    "Water Temple Emblems",
    "Shadow Temple Boat",
    "Spirit Statue",
    "Spirit Lower Emblem",
    "Spirit Upper Emblem",
    "Bottom of the Well",
    "Ganon's Castle Light Trial",
};
inline constexpr const char* MmLabels[] = {
    "Woodfall Temple",         "Snowhead Temple",
    "Great Bay Temple",        "Poe Hut Healing",
    "Darmani Healing",         "Pamela's Father Healing",
    "Kamaro Healing",          "Mikau Healing",
    "Captain Keeta Awakening", "Spider House Scrub Awakening",
    "Baby Goron Lullaby",      "Sharp's Curse",
    "Clock Tower Rooftop",
};
inline std::vector<uint8_t> Defaults(Game game) {
    return game == Game::Oot ? std::vector<uint8_t>{ 5, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
                             : std::vector<uint8_t>{ 14, 15, 17, 12, 12, 12, 12, 12, 14, 14, 15, 3, 19 };
}
inline bool Playable(Game game, uint8_t song) {
    if (game == Game::Oot)
        return song < 12;
    return song == 1 || song == 3 || song == 5 || song == 12 || song == 13 || song == 14 || song == 15 || song == 16 ||
           song == 17 || song == 19;
}
inline void Validate(Game game, const std::vector<uint8_t>& songs) {
    if (songs.size() != Defaults(game).size())
        throw std::invalid_argument("Wrong song-event mapping length");
    for (auto song : songs)
        if (!Playable(game, song))
            throw std::invalid_argument("Unplayable song-event assignment");
}
template <class Json> inline std::vector<uint8_t> ParseMapping(Game game, const Json& source) {
    if (!source.is_array())
        throw std::invalid_argument("Song-event mapping must be an array");
    std::vector<uint8_t> songs;
    for (const auto& value : source) {
        if (!value.is_number_integer() || value < 0 || value >= 20)
            throw std::invalid_argument("Invalid song-event song identity");
        songs.push_back(value.template get<uint8_t>());
    }
    Validate(game, songs);
    return songs;
}
// Local fixed-width PRNG never consumes or reseeds either host's item RNG.
inline uint32_t Next(uint32_t& seed) {
    seed += 0x9e3779b9u;
    auto z = seed;
    z = (z ^ (z >> 16)) * 0x85ebca6bu;
    z = (z ^ (z >> 13)) * 0xc2b2ae35u;
    return z ^ (z >> 16);
}
inline std::vector<uint8_t> Generate(Game game, uint32_t seed, bool shuffle) {
    auto result = Defaults(game);
    if (!shuffle)
        return result;
    std::vector<uint8_t> pool;
    for (uint8_t song = 0; song < 20; ++song)
        if (Playable(game, song))
            pool.push_back(song);
    seed ^= game == Game::Oot ? 0x534f4f54u : 0x534d4d21u;
    for (auto& song : result)
        song = pool[Next(seed) % pool.size()];
    return result;
}
inline bool ClockLocked(bool enabled, bool owned) {
    return enabled && !owned;
}
inline uint16_t ClockStart(unsigned mode, uint32_t seed) {
    return mode == 1 || (mode == 2 && (Next(seed) & 1)) ? 0xe000 : 0x8000;
}
} // namespace ComboRando::SongEvents
