#include "FrameTimingProbe.h"

#include <chrono>
#include <memory>
#include <nlohmann/json.hpp>
#include <ship/Context.h>
#include <spdlog/spdlog.h>

namespace {
thread_local FrameTiming::Recorder recorder;
std::shared_ptr<spdlog::logger> diagnosticLogger;
int previousEnabled = -1;

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

const std::shared_ptr<spdlog::logger>& DiagnosticLogger() {
    // Reuse the application's existing sinks and asynchronous writer, but give
    // this diagnostic its own level. A saved Warn/Off setting must not hide the
    // capture, and normal resource logging must retain the user's preference.
    if (!diagnosticLogger) {
        // Windows statically links spdlog into each game/engine DLL. The game
        // DLL's default logger is not the engine Context's file logger.
        diagnosticLogger = Ship::Context::GetRawInstance()->GetLogger()->clone("OoTFrameTimingProbe");
        diagnosticLogger->set_level(spdlog::level::info);
        diagnosticLogger->flush_on(spdlog::level::info);
    }
    return diagnosticLogger;
}

void ReportOutputState(bool enabled) {
    if (previousEnabled == static_cast<int>(enabled)) {
        return;
    }
    const nlohmann::json state = {
        { "event", previousEnabled == -1 ? "startup" : "state" },
        { "schema", 1 },
        { "enabled", enabled },
        { "phase_count", FRAME_TIMING_PHASE_COUNT },
        { "interval_ms", 1000 },
        { "normal_log_level_independent", true },
        { "scope", "all OoT gameplay scenes; no slow-frame threshold" },
    };
    DiagnosticLogger()->info("[FrameTimingProbe] {}", state.dump());
    previousEnabled = static_cast<int>(enabled);
}
} // namespace

extern "C" void FrameTiming_Shutdown(void) {
    // Release shared sinks while both game DLLs (which may own their formatter
    // vtables) remain mapped. Context teardown drains the async queue afterward.
    recorder = {};
    if (diagnosticLogger) {
        diagnosticLogger->flush();
        diagnosticLogger.reset();
    }
    previousEnabled = -1;
}

extern "C" void FrameTiming_BeginFrame(FrameTimingContext context, int enabled) {
    ReportOutputState(enabled != 0);
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
        { "event", "sample" },
        { "schema", 1 },
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
    DiagnosticLogger()->info("[FrameTimingProbe] {}", record.dump());
}
