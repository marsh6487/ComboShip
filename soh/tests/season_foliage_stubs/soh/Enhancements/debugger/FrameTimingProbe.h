#pragma once
#include <cstdint>
#include <map>
#include <string>

struct FrameTimingSpan {
    uint64_t epoch = 0, start = 0;
};
namespace FoliageTraceFixture {
inline bool active = false;
inline std::map<std::string, uint64_t> counters;
inline uint64_t spans = 0;
} // namespace FoliageTraceFixture
inline int FrameTiming_IsActive() {
    return FoliageTraceFixture::active;
}
inline FrameTimingSpan FrameTiming_BeginSpan() {
    return { 1, 1 };
}
inline void FrameTiming_EndNamedSpan(const char*, const char*, FrameTimingSpan) {
    ++FoliageTraceFixture::spans;
}
inline void FrameTiming_Count(const char* name, uint64_t amount) {
    FoliageTraceFixture::counters[name] += amount;
}
