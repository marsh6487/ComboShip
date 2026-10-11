#pragma once

#include <cstdint>
#include <string>
#include <string_view>

// Prefer the real OoT get-item message banks. These lessons also remain
// available for MM-first pickups before those banks have been loaded.
namespace ComboVanillaItemReceiptText {
enum class Kind { Sword, Master, Knife, Biggoron, Shield, Mirror, Fish, Bugs, Poe, BigPoe };
enum class Language { English, German, French };
struct Entry {
    const char* name;
    uint16_t textId;
    const char* englishName;
    const char* germanName;
    const char* frenchName;
    Kind kind;
};
inline constexpr Entry entries[] = {
    { "Kokiri Sword", 0xA4, "the Kokiri Sword", "das Kokiri-Schwert", "l'Épée Kokiri", Kind::Sword },
    { "Master Sword", 0, "the Master Sword", "das Master-Schwert", "l'Épée de Légende", Kind::Master },
    { "Giant's Knife", 0x4B, "the Giant's Knife", "das Langschwert", "la Lame des Géants", Kind::Knife },
    { "Biggoron's Sword", 0x0C, "Biggoron's Sword", "das Biggoron-Schwert", "l'Épée de Biggoron", Kind::Biggoron },
    { "Deku Shield", 0x4C, "a Deku Shield", "einen Deku-Schild", "un Bouclier Mojo", Kind::Shield },
    { "Hylian Shield", 0x4D, "a Hylian Shield", "einen Hylia-Schild", "un Bouclier Hylien", Kind::Shield },
    { "Mirror Shield", 0x4E, "the Mirror Shield", "den Spiegelschild", "le Bouclier Miroir", Kind::Mirror },
    { "Bottle with Fish", 0x47, "a Bottle with Fish", "eine Flasche mit Fisch", "une Bouteille avec un Poisson",
      Kind::Fish },
    { "Bottle with Bugs", 0x7A, "a Bottle with Bugs", "eine Flasche mit Wanzen", "une Bouteille avec des Insectes",
      Kind::Bugs },
    { "Bottle with Poe", 0x97, "a Bottle with Poe", "eine Flasche mit einem Geist", "une Bouteille avec un Esprit",
      Kind::Poe },
    { "Bottle with Big Poe", 0xF9, "a Bottle with Big Poe", "eine Flasche mit Seele", "une Bouteille avec une Âme",
      Kind::BigPoe },
};
inline const Entry* Find(std::string_view name) {
    for (const auto& entry : entries)
        if (name == entry.name)
            return &entry;
    return nullptr;
}
inline std::string Body(const Entry& entry, Language language = Language::English) {
    const bool german = language == Language::German, french = language == Language::French;
    const bool sword = entry.kind <= Kind::Biggoron;
    const bool shield = entry.kind == Kind::Shield || entry.kind == Kind::Mirror;
    const std::string color = sword ? "%c" : shield ? "%b" : "%g";
    std::string body = (german   ? "Du hast "
                        : french ? "Vous obtenez "
                                 : "You got ") +
                       color +
                       (german   ? entry.germanName
                        : french ? entry.frenchName
                                 : entry.englishName) +
                       "%w!^";
    if (sword || shield) {
        body += german   ? "Wähle es im Ausrüstungs-Menü&und lege es mit %y\x9F%w an.^"
                : french ? "Sélectionnez-le dans l'Équipement&et équipez-le avec %y\x9F%w.^"
                         : "Select it on the Equipment&Subscreen and equip with %y\x9F%w.^";
        body += sword ? (german   ? "%y\xA0%w: Hieb. Halte %y\xA0%w für&eine Wirbelattacke."
                         : french ? "%y\xA0%w: frappez. Maintenez %y\xA0%w&pour une attaque circulaire."
                                  : "Press %y\xA0%w to swing. Hold %y\xA0%w&to charge a Spin Attack.")
                      : (german   ? "Halte %y\xA3%w, um den Schild&zur Abwehr zu heben."
                         : french ? "Maintenez %y\xA3%w pour lever&le bouclier et vous protéger."
                                  : "Hold %y\xA3%w to raise the shield&and defend yourself.");
    } else {
        body += german   ? "Lege die Flasche auf %y\xA1%w.&Drücke die Taste, um sie zu nutzen.^"
                : french ? "Assignez la bouteille à %y\xA1%w.&Appuyez pour l'utiliser.^"
                         : "Assign the bottle to %y\xA1%w.&Press its button to use it.^";
    }
    switch (entry.kind) {
        case Kind::Master:
            body += german   ? "^Die legendäre Klinge&bannt das Böse."
                    : french ? "^Cette lame légendaire&repousse le mal."
                             : "^This legendary blade&repels evil.";
            break;
        case Kind::Knife:
        case Kind::Biggoron:
            body += german   ? "^Die Zweihandklinge verhindert&die gleichzeitige Schildabwehr."
                    : french ? "^Cette lame à deux mains empêche&de lever le bouclier en frappant."
                             : "^This two-handed blade prevents&shield defense while wielded.";
            break;
        case Kind::Mirror:
            body += german   ? "^Seine polierte Fläche reflektiert&Licht und bestimmte Angriffe."
                    : french ? "^Sa surface réfléchit la lumière&et certaines attaques."
                             : "^Its polished surface reflects&light and certain attacks.";
            break;
        case Kind::Fish:
            body += german   ? "Du kannst den Fisch&aus der Flasche freilassen."
                    : french ? "Vous pouvez relâcher le poisson&contenu dans la bouteille."
                             : "You can release the fish&from its bottle.";
            break;
        case Kind::Bugs:
            body += german   ? "Lass die Käfer frei. Sie suchen&nach kleinen Löchern im Boden."
                    : french ? "Relâchez les insectes. Ils cherchent&de petits trous dans le sol."
                             : "Release the bugs. They look for&small holes in the ground.";
            break;
        case Kind::Poe:
            body += german   ? "Trinke den Geist auf eigene Gefahr:&Die Wirkung auf Herzen schwankt!"
                    : french ? "Buvez cet esprit à vos risques:&son effet sur la vie varie!"
                             : "Drink this spirit at your own risk:&its effect on hearts varies!";
            break;
        case Kind::BigPoe:
            body += german   ? "Bring diese Seele zum Geisterladen&am Eingang von Hyrule-Stadt."
                    : french ? "Apportez cette âme au marchand&de fantômes à l'entrée du Bourg."
                             : "Take this soul to the Ghost Shop&at the entrance to Hyrule Market.";
            break;
        default:
            break;
    }
    return body;
}
} // namespace ComboVanillaItemReceiptText
