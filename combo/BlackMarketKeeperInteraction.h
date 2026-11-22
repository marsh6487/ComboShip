#pragma once

#include <cstdint>
#include <string>
#include <string_view>

// Presentation only. EnOssan still owns input, messages, cameras and receipts.
namespace BlackMarketKeeperInteraction {
enum class Clip { Idle, Sleep, Wake, EnterTalk, Talk, Raise, Ask };
struct State {
    Clip clip = Clip::Idle;
    bool talking = false;
    bool sleepClips = false;

    template <typename Change> void Update(bool active, bool finished, bool nextPage, bool refusal, Change change) {
        auto set = [&](Clip next, float morph) {
            clip = next;
            change(next, morph);
        };
        const Clip rest = sleepClips ? Clip::Sleep : Clip::Idle;
        // These are EnTakaraya_Wait/Talk/SetupWait's native transitions. The
        // swaying talk loop never reports completion; once-only clips do.
        if (finished) {
            if (clip == Clip::Wake)
                set(active ? Clip::Talk : rest, -6.f);
            else if (clip == Clip::EnterTalk)
                set(talking ? Clip::Talk : Clip::Raise, talking ? 0.f : 5.f);
            else if (clip == Clip::Raise)
                set(talking ? Clip::Ask : rest, talking ? 0.f : -4.f);
            else if (clip == Clip::Ask)
                set(talking ? Clip::Idle : rest, talking ? 0.f : -4.f);
        }
        if (active && !talking && clip != Clip::Wake) {
            const bool waking = clip == Clip::Sleep;
            set(waking ? Clip::Wake : Clip::EnterTalk, waking ? -10.f : -4.f);
        } else if (!active && talking && clip == Clip::Talk)
            set(Clip::Raise, 5.f);
        else if (!active && talking && clip == Clip::Idle && sleepClips)
            set(rest, -6.f);
        if (active && nextPage && clip != Clip::Wake)
            set(Clip::Raise, 5.f);
        if (refusal && clip == Clip::Idle)
            set(Clip::Ask, 5.f);
        talking = active;
    }
};

// Native gal voices are SFX controls in her message-table entries. Reuse the
// first cue from the selected game's message, without importing its minigame
// text, choice prices, timers, progression flags or a guessed sound number.
inline std::string FirstMessageSfx(std::string_view message) {
    for (size_t i = 0; i < message.size();) {
        const auto command = static_cast<uint8_t>(message[i]);
        if (command == 0xBF)
            break;
        const size_t size = command == 0x14 ? 2 : command >= 0x1B && command <= 0x1F ? 3 : 1;
        if (size > message.size() - i)
            return {};
        if (command == 0x1E)
            return std::string(message.substr(i, size));
        i += size;
    }
    return {};
}
inline uint16_t Greeting(unsigned form) {
    constexpr uint16_t texts[] = { 0x76D, 0x76E, 0x76F, 0x76C, 0x76D };
    return texts[form < 5 ? form : 4];
}
inline uint16_t Decline(unsigned form) {
    constexpr uint16_t texts[] = { 0x775, 0x776, 0x777, 0x774, 0x775 };
    return texts[form < 5 ? form : 4];
}
inline uint16_t Talk(unsigned form) {
    constexpr uint16_t texts[] = { 0x771, 0x772, 0x773, 0x770, 0x771 };
    return texts[form < 5 ? form : 4];
}
} // namespace BlackMarketKeeperInteraction
