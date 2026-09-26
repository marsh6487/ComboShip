#include "FrameTimingProbe.h"

#include <chrono>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace {
thread_local FrameTiming::Recorder recorder;

uint64_t Now() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

constexpr const char* names[] = { "tick_build",        "play_update",      "play_draw",        "frame_hooks",
                                  "cosmetics",         "mm_asset_lookup",  "audio_mutex_wait", "window_events",
                                  "interpolation",     "draw_and_present", "frame_ready",      "render_setup",
                                  "graphics_commands", "gui_finish",       "present" };
static_assert(sizeof(names) / sizeof(names[0]) == FRAME_TIMING_PHASE_COUNT);
} // namespace

extern "C" void FrameTiming_BeginFrame(FrameTimingContext context, int enabled) {
    recorder.BeginFrame(context, enabled != 0, enabled && context.scene >= 0 ? Now() : 0);
}

extern "C" FrameTimingSpan FrameTiming_BeginSpan(void) {
    return recorder.Active() ? recorder.BeginSpan(Now()) : FrameTimingSpan{};
}

extern "C" int FrameTiming_IsActive(void) {
    return recorder.Active();
}

extern "C" void FrameTiming_AddDuration(FrameTimingPhase phase, uint64_t nanos) {
    recorder.AddDuration(phase, nanos);
}

extern "C" void FrameTiming_EndSpan(FrameTimingPhase phase, FrameTimingSpan span) {
    if (span.epoch != 0 && recorder.Active()) {
        recorder.EndSpan(phase, span, Now());
    }
}

extern "C" void FrameTiming_EndFrame(FrameTimingContext context, int enabled) {
    if (!recorder.Active()) {
        return;
    }
    auto report = recorder.EndFrame(Now(), context, enabled != 0);
    if (!report) {
        return;
    }
    // Formatting and logging occur after the timed tick, once per second.
    const double perTickMillis = 1.0 / (1000000.0 * report->gameTicks);
    nlohmann::json phases = nlohmann::json::object();
    for (size_t i = 0; i < FRAME_TIMING_PHASE_COUNT; ++i) {
        phases[names[i]] = {
            { "mean_ms_per_tick", report->phaseNanos[i] * perTickMillis },
            { "max_ms_per_tick", report->phaseMaxNanos[i] / 1000000.0 },
            { "calls", report->calls[i] },
        };
    }
    const nlohmann::json record = {
        { "scene", report->context.scene },
        { "room", report->context.room },
        { "age", report->context.age },
        { "alt_assets", report->context.altAssets != 0 },
        { "paused", report->context.paused != 0 },
        { "target_fps", report->context.targetFps },
        { "capture_ms", report->captureNanos / 1000000.0 },
        { "game_ticks", report->gameTicks },
        { "graphics_calls", report->calls[FRAME_TIMING_DRAW_PRESENT] },
        { "presented_frames", report->calls[FRAME_TIMING_PRESENT] },
        { "present_rate", report->calls[FRAME_TIMING_PRESENT] * (1000000000.0 / report->captureNanos) },
        { "mean_tick_ms", report->tickNanos * perTickMillis },
        { "max_tick_ms", report->tickMaxNanos / 1000000.0 },
        { "between_ticks_ms", (report->captureNanos - report->tickNanos) / 1000000.0 },
        { "phases", std::move(phases) },
        { "semantics",
          "inclusive elapsed scopes; nested phases overlap; draw_and_present includes pacing and GPU waits" },
    };
    SPDLOG_INFO("[FrameTimingProbe] {}", record.dump());
}
