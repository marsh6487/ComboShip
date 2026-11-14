#pragma once

#include <string_view>

// Shared OoT markup; each host encodes its own colors and button glyphs.
// Keep these available without a loaded donor for MM-first pickups.
namespace ComboToolReceiptText {
struct Entry {
    const char* name;
    const char* english;
    const char* german;
    const char* french;
};

inline constexpr Entry kClock = {
    "Clock",
    "You got the %yClock%w!&"
    "The flow of time has returned&"
    "to %gHyrule%w.^"
    "It takes effect %cautomatically%w.&"
    "Wait in %gHyrule Field%w&"
    "for %yday or night%w to come.",
    "Du hast die %yUhr%w erhalten!&"
    "Die Zeit in %gHyrule%w&"
    "vergeht nun wieder.^"
    "Sie wirkt %cautomatisch%w.&"
    "Warte in der %gHylianischen&"
    "Steppe%w auf %yTag oder Nacht%w.",
    "Vous obtenez l'%yHorloge%w !&"
    "Le temps reprend son cours&"
    "en %gHyrule%w !^"
    "Son effet est %cautomatique%w.&"
    "Dans la %gPlaine d'Hyrule%w,&"
    "attendez le %yjour ou la nuit%w.",
};

inline constexpr Entry kPhantomHourglass = {
    "Phantom Hourglass",
    "You got the %yPhantom Hourglass%w!&"
    "Equip to %y\xA1%w or %yD-Pad%w.^"
    "Press to %cstop time and aim%w.&"
    "Press again: %crewind the target%w&"
    "along its %crecorded path%w.^"
    "Press its button to %cstop%w.&"
    "%y\xA0%w %ccancels aiming%w.&"
    "Rewinding %cuses magic%w.^"
    "While aiming, hold %y\xA3%w&"
    "to %crewind your own path%w.&"
    "Release %y\xA3%w to %caim again%w.",
    "Du hast die %yPhantom-Sanduhr%w!&"
    "Lege sie auf %y\xA1%w oder %yD-Pad%w.^"
    "Drücke: %cZeitstopp und Zielen%w.&"
    "Nochmals: %cZiel zurückspulen%w&"
    "auf seinem %cgespeicherten Weg%w.^"
    "Item-Taste: %cSpulen stoppen%w.&"
    "%y\xA0%w %cbeendet das Zielen%w.&"
    "Zurückspulen %ckostet Magie%w.^"
    "Halte beim Zielen %y\xA3%w:&"
    "%cSpule deinen Weg zurück%w.&"
    "Lass %y\xA3%w los: %cwieder zielen%w.",
    "Vous obtenez le %ySablier Fantôme%w!&"
    "Assignez-le à %y\xA1%w ou au %yD-Pad%w.^"
    "Appuyez: %ctemps figé et visée%w.&"
    "Encore: %crembobinez la cible%w&"
    "sur son %ctrajet enregistré%w.^"
    "Appuyez encore: %carrêt%w.&"
    "%y\xA0%w %cannule la visée%w.&"
    "Rembobiner %ccoûte de la magie%w.^"
    "En visant, maintenez %y\xA3%w&"
    "pour %crevenir sur vos pas%w.&"
    "Relâchez %y\xA3%w: %cvisez à nouveau%w.",
};

inline constexpr Entry kShadowCrystal = {
    "Shadow Crystal",
    "You got the %pShadow Crystal%w!&"
    "Equip to %y\xA1%w or %yD-Pad%w.^"
    "Press to become %pWolf Link%w.&"
    "%y\xA0%w chains %rbites and swipes%w.&"
    "Moving + %y\x9F%w: %cdash%w.^"
    "Use crystal again&"
    "to %creturn to normal%w.",
    "Du hast den %pSchattenkristall%w!&"
    "Lege ihn auf %y\xA1%w oder %yD-Pad%w.^"
    "Drücke für %pWolf-Link%w.&"
    "%y\xA0%w: %rBisse und Hiebe%w.&"
    "In Bewegung + %y\x9F%w: %cSprint%w.^"
    "Nutze den Kristall erneut&"
    "für die %cnormale Form%w.",
    "Vous obtenez le %pCristal des Ombres%w!&"
    "Assignez-le à %y\xA1%w ou au %yD-Pad%w.^"
    "Appuyez: %pLink loup%w.&"
    "%y\xA0%w: %rmorsures et coups%w.&"
    "En mouvement + %y\x9F%w: %cruée%w.^"
    "Réutilisez le cristal&"
    "pour %credevenir normal%w.",
};

inline constexpr const Entry* Find(std::string_view name) {
    if (name == kClock.name)
        return &kClock;
    if (name == kPhantomHourglass.name)
        return &kPhantomHourglass;
    if (name == kShadowCrystal.name)
        return &kShadowCrystal;
    return nullptr;
}
} // namespace ComboToolReceiptText
