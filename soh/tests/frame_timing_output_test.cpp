#include "soh/Enhancements/debugger/FrameTimingProbe.h"

#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <spdlog/async_logger.h>
#include <spdlog/details/thread_pool.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>
#include <thread>

int main(int argc, char** argv) {
    assert(argc == 2);
    auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(argv[1], 1024 * 1024, 1);
    sink->set_pattern("%v");
    auto pool = std::make_shared<spdlog::details::thread_pool>(8192, 1);
    auto gameLogger = std::make_shared<spdlog::async_logger>("game", sink, pool, spdlog::async_overflow_policy::block);
    spdlog::set_default_logger(gameLogger);
    gameLogger->set_level(spdlog::level::off);

    // Exercise the production adapter and the same asynchronous rotating-file
    // sink used by release builds, with normal game logging disabled.
    FrameTimingContext title{ -1, 0, 0, 1, 0, 60 };
    FrameTiming_BeginFrame(title, 1);
    FrameTiming_EndFrame(title, 1);
    FrameTimingContext play{ 82, 0, 0, 1, 0, 60 };
    for (auto level : { spdlog::level::off, spdlog::level::warn }) {
        gameLogger->set_level(level);
        FrameTiming_BeginFrame(play, 1);
        auto span = FrameTiming_BeginSpan();
        FrameTiming_EndSpan(FRAME_TIMING_PLAY_UPDATE, span);
        FrameTiming_AddDuration(FRAME_TIMING_DRAW_PRESENT, 1000000);
        FrameTiming_AddDuration(FRAME_TIMING_PRESENT, 500000);
        std::this_thread::sleep_for(std::chrono::milliseconds(1050));
        FrameTiming_EndFrame(play, 1);
        assert(gameLogger->level() == level); // Preserve the user's normal logging preference.
    }
    FrameTiming_BeginFrame(play, 0);
    FrameTiming_EndFrame(play, 0);
    FrameTiming_Shutdown();
    // Thread-pool destruction drains queued log records and joins the writer.
    pool.reset();
    std::weak_ptr<spdlog::sinks::rotating_file_sink_mt> sinkLifetime = sink;
    spdlog::drop_all();
    gameLogger.reset();
    sink.reset();
    assert(sinkLifetime.expired()); // No probe-owned formatter may survive game DLL teardown.

    std::ifstream log(argv[1]);
    std::string line;
    int samples = 0;
    bool armed = false;
    bool disabled = false;
    while (std::getline(log, line)) {
        const auto marker = line.find("[FrameTimingProbe] ");
        if (marker == std::string::npos) {
            continue;
        }
        const auto record = nlohmann::json::parse(line.substr(marker + 19));
        assert(record.at("schema") == 1);
        const auto event = record.at("event").get<std::string>();
        if (event == "startup") {
            armed = record.at("enabled").get<bool>();
            assert(record.at("phase_count") == FRAME_TIMING_PHASE_COUNT);
            assert(record.at("normal_log_level_independent").get<bool>());
        } else if (event == "state") {
            disabled = !record.at("enabled").get<bool>();
        } else if (event == "sample") {
            ++samples;
            assert(record.at("scene") == play.scene);
            assert(record.at("target_fps") == 60);
            assert(record.at("game_ticks") == 1);
            assert(record.at("presented_frames") == 1);
            assert(record.at("present_rate").get<double>() > 0);
            const auto& phases = record.at("phases");
            assert(phases.size() == FRAME_TIMING_PHASE_COUNT);
            assert(phases.at("play_update").at("calls") == 1);
            assert(phases.at("present").at("mean_ms_per_tick") == 0.5);
        }
    }
    assert(armed);
    assert(disabled);
    assert(samples == 2);

    // A fresh Context in the same process must announce itself and bind to its
    // new sink instead of retaining the old file, pool, or enabled state.
    const std::string nextPath = std::string(argv[1]) + ".next";
    sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(nextPath, 1024 * 1024, 1);
    sink->set_pattern("%v");
    pool = std::make_shared<spdlog::details::thread_pool>(8192, 1);
    gameLogger = std::make_shared<spdlog::async_logger>("new-game", sink, pool, spdlog::async_overflow_policy::block);
    gameLogger->set_level(spdlog::level::off);
    spdlog::set_default_logger(gameLogger);
    FrameTiming_BeginFrame(title, 1);
    FrameTiming_EndFrame(title, 1);
    FrameTiming_Shutdown();
    pool.reset();
    spdlog::drop_all();
    sinkLifetime = sink;
    gameLogger.reset();
    sink.reset();
    assert(sinkLifetime.expired());
    std::ifstream nextLog(nextPath);
    assert(std::getline(nextLog, line));
    const auto restarted = nlohmann::json::parse(line.substr(line.find("[FrameTimingProbe] ") + 19));
    assert(restarted.at("event") == "startup");
    assert(restarted.at("enabled").get<bool>());
    std::cout << "Production timing output: startup, all phases, Warn/Off immunity, teardown and restart passed\n";
}
