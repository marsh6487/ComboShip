#pragma once

#include "ComboBossSoulReceiptText.h"
#include <initializer_list>
#include <string>
#include <string_view>

namespace ComboSoulReceiptText {
using Language = ComboBossSoulReceiptText::Language;
enum class Family { Enemy, Npc, Animal, Misc };
struct Soul {
    const char* name;
    Family family;
};

inline const Soul* Find(std::string_view name) {
    // Match catalog identities only: no suffix/prefix guess may turn an
    // unknown item or an ice-trap disguise into a revealed soul receipt.
    static constexpr Soul catalog[] = {
#define SOUL(game, family, key, name) { name, SOUL_FAMILY_##family },
#define SOUL_FAMILY_ENEMY Family::Enemy
#define SOUL_FAMILY_NPC Family::Npc
#define SOUL_FAMILY_ANIMAL Family::Animal
#define SOUL_FAMILY_MISC Family::Misc
#include "../rando/SoulShuffleCatalog.inc"
#undef SOUL_FAMILY_MISC
#undef SOUL_FAMILY_ANIMAL
#undef SOUL_FAMILY_NPC
#undef SOUL_FAMILY_ENEMY
#undef SOUL
        // MM's original enemy catalog predates SoulShuffleCatalog.inc. Keep
        // exact native identities here; the receipt test checks full equality.
        { "Soul of Aliens", Family::Enemy },        { "Soul of Armos", Family::Enemy },
        { "Soul of Bad Bats", Family::Enemy },      { "Soul of Beamos", Family::Enemy },
        { "Soul of Boes", Family::Enemy },          { "Soul of Bubbles", Family::Enemy },
        { "Soul of Captain Keeta", Family::Enemy }, { "Soul of Chuchus", Family::Enemy },
        { "Soul of Death Armos", Family::Enemy },   { "Soul of Deep Pythons", Family::Enemy },
        { "Soul of Deku Babas", Family::Enemy },    { "Soul of Dexihands", Family::Enemy },
        { "Soul of Dinolfos", Family::Enemy },      { "Soul of Dodongos", Family::Enemy },
        { "Soul of Dragonflies", Family::Enemy },   { "Soul of Eenos", Family::Enemy },
        { "Soul of Eyegores", Family::Enemy },      { "Soul of Freezards", Family::Enemy },
        { "Soul of Garos", Family::Enemy },         { "Soul of Gekkos", Family::Enemy },
        { "Soul of Giant Bees", Family::Enemy },    { "Soul of Gomess", Family::Enemy },
        { "Soul of Guays", Family::Enemy },         { "Soul of Hiploops", Family::Enemy },
        { "Soul of Igos du Ikana", Family::Enemy }, { "Soul of Iron Knuckles", Family::Enemy },
        { "Soul of Keese", Family::Enemy },         { "Soul of Leevers", Family::Enemy },
        { "Soul of Like Likes", Family::Enemy },    { "Soul of Mad Scrubs", Family::Enemy },
        { "Soul of Nejirons", Family::Enemy },      { "Soul of Octoroks", Family::Enemy },
        { "Soul of Peahats", Family::Enemy },       { "Soul of Pirates", Family::Enemy },
        { "Soul of Poes", Family::Enemy },          { "Soul of Redeads", Family::Enemy },
        { "Soul of Shellblades", Family::Enemy },   { "Soul of Skullfish", Family::Enemy },
        { "Soul of Skulltulas", Family::Enemy },    { "Soul of Snappers", Family::Enemy },
        { "Soul of Stalchildren", Family::Enemy },  { "Soul of Takkuri", Family::Enemy },
        { "Soul of Tektites", Family::Enemy },      { "Soul of Wallmasters", Family::Enemy },
        { "Soul of Warts", Family::Enemy },         { "Soul of Wizrobes", Family::Enemy },
        { "Soul of Wolfos", Family::Enemy },
    };
    for (const auto& soul : catalog)
        if (name == soul.name)
            return &soul;
    return nullptr;
}

inline const char* Color(Family family) {
    switch (family) {
        case Family::Enemy:
            return "%r";
        case Family::Npc:
            return "%p";
        case Family::Animal:
            return "%g";
        case Family::Misc:
            return "%y";
    }
    return "%w";
}

inline std::string Highlight(std::string_view identity, std::string_view shown) {
    const auto* soul = Find(identity);
    return soul ? Color(soul->family) + std::string(shown) + "%w" : std::string(shown);
}

inline std::string Body(const Soul& soul, Language language = Language::English, std::string_view localizedName = {}) {
    const auto name = localizedName.empty() ? std::string_view(soul.name) : localizedName;
    std::string body = language == Language::German   ? "Du hast "
                       : language == Language::French ? "Vous avez trouvé "
                                                      : "You found ";
    body += Highlight(soul.name, name);
    body += language == Language::German ? " gefunden!&" : "!&";
    if (language == Language::German) {
        body += soul.family == Family::Enemy ? "Diese Gegner kehren in die Welt zurück."
                : soul.family == Family::Npc
                    ? "Eine vertraute Seele kehrt zurück.&Diese Personen können nun erscheinen."
                : soul.family == Family::Animal ? "Neues Leben regt sich.&Diese Tiere können nun erscheinen."
                : std::string_view(soul.name) == "Business Scrub Misc Soul"
                    ? "Die Händler erwachen.&Geschäfts-Dekus können erscheinen."
                    : "Goldene Spinnen erwachen.&Goldene Skulltulas können erscheinen.";
    } else if (language == Language::French) {
        body += soul.family == Family::Enemy    ? "Ces ennemis reviennent dans le monde."
                : soul.family == Family::Npc    ? "Une présence familière revient.&Ces personnes peuvent apparaître."
                : soul.family == Family::Animal ? "La vie s'éveille.&Ces animaux peuvent apparaître."
                : std::string_view(soul.name) == "Business Scrub Misc Soul"
                    ? "Les marchands s'éveillent.&Les Pestes Marchandes apparaissent."
                    : "Les araignées dorées s'éveillent.&Les Skulltulas d'Or apparaissent.";
    } else {
        body += soul.family == Family::Enemy    ? "A restless spirit stirs.&These enemies can now appear."
                : soul.family == Family::Npc    ? "A familiar presence returns.&Those tied to this soul can now return."
                : soul.family == Family::Animal ? "Life stirs once more.&These creatures can now return."
                : std::string_view(soul.name) == "Business Scrub Misc Soul"
                    ? "The merchants stir again.&Business Scrubs can now appear."
                    : "Golden spiders stir again.&Gold Skulltulas can now appear.";
    }
    return body;
}
} // namespace ComboSoulReceiptText
