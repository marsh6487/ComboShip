#pragma once
#include <cstring>
#include <string_view>
#include "ZeldaRun.h"

namespace ZeldaRun {
enum Age { Child = ZELDA_RUN_CHILD, Adult = ZELDA_RUN_ADULT, Both = ZELDA_RUN_BOTH };
constexpr char kHeader[] = "objects/optional_zelda_run/poc7/gRun";
constexpr char kData[] = "objects/optional_zelda_run/poc7/gRunData";
constexpr int kFrames = 20;
constexpr int kWordsPerFrame = 67;
inline bool AgeAllowed(int ages, bool adult) {
    return ages == Both || (ages == Adult && adult) || (ages == Child && !adult);
}
inline bool IsFreeRun(std::string_view path) {
    if (path.starts_with("__OTR__"))
        path.remove_prefix(7);
    if (path.starts_with("alt/"))
        path.remove_prefix(4);
    return path == "objects/gameplay_keep/gPlayerAnim_link_normal_run_free";
}
} // namespace ZeldaRun
