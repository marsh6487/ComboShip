#pragma once

#include <cstdint>

namespace CustomEncounters {

inline constexpr uint16_t OotHintTextBase = 0x0600;
inline constexpr uint16_t MmHintTextBase = 0x1900;

struct EnemyHint {
    uint8_t ootId;
    uint8_t mmId;
    const char* name;
    const char* advice;
};

// Indexed by Enemy, starting at Miniblin. Keep the existing OoT Beetle text ID.
inline constexpr EnemyHint EnemyHints[] = {
    { 0xE0, 0xE0, "Miniblin",
      "It'll try to steal your Rupees!\nStun it with a Deku Nut,\nthen strike before it runs." },
    { 0xE1, 0xE1, "Molmauk",
      "Its hammers wield fire and ice.\nStrike during the big windup\nbefore both hammers come down!" },
    { 0x5D, 0xE2, "Scissors Beetle",
      "It throws its pincers like\nboomerangs! Keep moving, then\nstrike when they return!" },
    { 0xE3, 0xE3, "Lynel",
      "An arrow to the head can stun it.\nWhile it's down, get close to its\nback and press A to mount!" },
};

} // namespace CustomEncounters
