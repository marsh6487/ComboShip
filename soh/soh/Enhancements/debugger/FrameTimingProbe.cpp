#include "FrameTimingProbe.h"
#include "fast/RenderCostProbe.h"

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

extern "C" void FrameTiming_LogRenderCost(const Fast::RenderCostReport& report, FrameTimingContext context) {
    if (!report.sampled) {
        return;
    }
    auto counter = [](const Fast::RenderCostCounter& cost) {
        return nlohmann::json{ { "calls", cost.calls }, { "ms", cost.nanos / 1000000.0 } };
    };
    uint64_t commandNanos = 0;
    nlohmann::json opcodes = nlohmann::json::array();
    for (size_t i = 0; i < report.opcodes.size(); ++i) {
        const auto& cost = report.opcodes[i];
        if (cost.calls == 0) {
            continue;
        }
        commandNanos += cost.nanos;
        auto entry = counter(cost);
        entry["ucode"] = i / 256;
        entry["opcode"] = i % 256;
        entry["name"] = cost.name ? cost.name : "<unknown>";
        opcodes.push_back(std::move(entry));
    }
    // Sort indices only. Reports keep no resource ownership and remain unchanged.
    std::vector<size_t> order;
    for (size_t i = 0; i < report.resources.size(); ++i) {
        if (report.resources[i].commands.calls != 0) {
            order.push_back(i);
        }
    }
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        return report.resources[a].commands.nanos > report.resources[b].commands.nanos;
    });
    nlohmann::json resources = nlohmann::json::array();
    for (size_t i = 0; i < std::min<size_t>(12, order.size()); ++i) {
        const auto& cost = report.resources[order[i]];
        auto entry = counter(cost.commands);
        entry["path"] = cost.path;
        resources.push_back(std::move(entry));
    }
    order.clear();
    for (size_t i = 0; i < report.textures.size(); ++i) {
        if (report.textures[i].upload.calls != 0) {
            order.push_back(i);
        }
    }
    std::sort(order.begin(), order.end(),
              [&](size_t a, size_t b) { return report.textures[a].upload.nanos > report.textures[b].upload.nanos; });
    nlohmann::json textures = nlohmann::json::array();
    for (size_t i = 0; i < std::min<size_t>(12, order.size()); ++i) {
        const auto& cost = report.textures[order[i]];
        auto entry = counter(cost.upload);
        entry["path"] = cost.path;
        entry["bytes"] = cost.bytes;
        textures.push_back(std::move(entry));
    }
    const nlohmann::json record = {
        { "event", "sample" },
        { "schema", 1 },
        { "scene", context.scene },
        { "room", context.room },
        { "age", context.age },
        { "alt_assets", context.altAssets != 0 },
        { "paused", context.paused != 0 },
        { "target_fps", context.targetFps },
        { "frame_index", report.frameIndex },
        { "game_tick", report.gameTick },
        { "interpolation_index", report.interpolationIndex },
        { "interpolation_t", report.interpolationT },
        { "sample_every", Fast::RenderCostProbe::SampleEvery },
        { "backend", report.backend },
        { "render_width", report.renderWidth },
        { "render_height", report.renderHeight },
        { "window_width", report.windowWidth },
        { "window_height", report.windowHeight },
        { "msaa", report.msaa },
        { "total_ms", report.totalNanos / 1000000.0 },
        { "command_sum_ms", commandNanos / 1000000.0 },
        { "non_command_ms", (report.totalNanos >= commandNanos ? report.totalNanos - commandNanos : 0) / 1000000.0 },
        { "opcodes", std::move(opcodes) },
        { "top_resources", std::move(resources) },
        { "top_texture_uploads", std::move(textures) },
        { "resource_overflow", report.resourceOverflow },
        { "scopes",
          { { "texture_import", counter(report.textureImport) },
            { "upload", counter(report.upload) },
            { "driver_draw", counter(report.driverDraw) },
            { "shader_create", counter(report.shaderCreate) },
            { "framebuffer_setup", counter(report.framebufferSetup) },
            { "framebuffer_finish", counter(report.framebufferFinish) } } },
        { "cache",
          { { "hits", report.cacheHits },
            { "misses", report.cacheMisses },
            { "evictions", report.cacheEvictions },
            { "entries", report.cacheEntries },
            { "clears_total", report.cacheClearsTotal },
            { "deletes_total", report.cacheDeletesTotal } } },
        { "alt_render_lookup",
          { { "enabled", report.altRenderLookup },
            { "vertex_hits", report.vertexLookups.hits },
            { "vertex_fallbacks", report.vertexLookups.fallbacks },
            { "display_list_hits", report.displayListLookups.hits },
            { "display_list_fallbacks", report.displayListLookups.fallbacks } } },
        { "upload_bytes", report.uploadBytes },
        { "vertices", report.vertices },
        { "triangles", report.triangles },
        { "submitted_triangles", report.submittedTriangles },
        { "semantics", "sampled CPU elapsed including driver waits, not GPU timestamps; nested scopes overlap; "
                       "resource times are own commands, excluding child lists; total includes profiling overhead; "
                       "cache clears/deletes are cumulative since interpreter initialization" },
    };
    DiagnosticLogger()->info("[RenderCostProbe] {}", record.dump());
}
