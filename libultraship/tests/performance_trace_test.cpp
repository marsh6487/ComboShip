#include "ship/diagnostics/PerformanceTrace.h"
#include <cassert>
#include <thread>
#include <future>
#include <iostream>
using namespace Ship::PerformanceTrace;
int main() {
    assert(!Enabled());
    Record("resource.cold_load", "disabled", 0, 1);
    assert(EndTick()["events"].empty());
    BeginTick(1);
    const auto origin = CaptureContext();
    std::promise<void> ready, finish;
    auto gate = finish.get_future();
    std::thread worker([&] {
        ContextScope binding(origin);
        Scope scope("resource.cold_load", "objects/example", 123, 456);
        ready.set_value();
        gate.wait();
        Record("resource.archive_read", "objects/example", 10, 40, 123, 456, "file");
        Record("resource.deserialize_import", "objects/example", 40, 90, 123, 456, "success");
        Count("resource.cache_misses", 3);
    });
    ready.get_future().wait();
    auto first = EndTick();
    assert(first["events"].empty());
    assert(first["outstanding_scopes"] == 1);
    BeginTick(2);
    finish.set_value();
    worker.join();
    auto second = EndTick();
    assert(second["events"].size() == 3);
    assert(second["outstanding_scopes"] == 0);
    assert(second["collector_record_calls"] == 3);
    assert(second["collector_count_calls"] == 1);
    assert(second["collector_record_ns"].get<uint64_t>() > 0);
    assert(second["collector_count_ns"].get<uint64_t>() > 0);
    assert(second["collector_errors"] == 0);
    assert(second["late_events"] == 3);
    assert(second["late_counters"] == 1);
    assert(second["counts"]["resource.cache_misses"] == 3);
    for (const auto& event : second["events"]) {
        assert(event["origin_tick"] == 1 && event["late"] == true);
        assert(event["manager"] == 123 && event["cache_owner"] == 456);
        assert(event["name"] == "objects/example");
    }
    BeginTick(3);
    for (int i = 0; i < 10000; ++i) Record("actor.update", "Player", 0, 100);
    Record("resource.cold_load", "first_frame", 0, 1, 9, 10);
    auto normal = EndTick();
    assert(normal["events"].size() == 1);
    assert(normal["overflow_events"] == 0);
    assert(normal["aggregates"][0]["count"] == 10000);
    BeginTick(4);
    for (int i = 0; i < 1400; ++i) Record("actor.draw", "slow", 0, 3000000);
    for (int i = 0; i < 1400; ++i) Record("resource.cold_load", "cold", 0, 1);
    auto bounded = EndTick();
    assert(bounded["events"].size() == 1280);
    assert(bounded["peak_records"] == 1280);
    assert(bounded["lifetime_peak_records"] == 1280);
    assert(bounded["overflow_events"] == 1520);
    BeginTick(5);
    const auto pending = CaptureContext();
    EndTick();
    { ContextScope binding(pending); Record("resource.cold_load", "after_close", 0, 1); }
    BeginTick(6);
    auto late = EndTick();
    assert(late["events"][0]["origin_tick"] == 5);
    assert(late["late_events"] == 1);
    BeginTick(7);
    { ContextScope disabled({}); assert(!Enabled()); Record("resource.cold_load", "disabled_worker", 0, 1); }
    assert(Enabled());
    assert(EndTick()["events"].empty());
    BeginTick(8);
    for (int i = 0; i < 4200; ++i) Record("actor", std::to_string(i), 0, i);
    auto cap = EndTick();
    assert(cap["aggregates"].size() == 4096);
    assert(cap["aggregate_keys"] == 4096);
    assert(cap["overflow_aggregates"] == 104);
    assert(cap["aggregate_keys_omitted"] == 0);
    BeginTick(9);
    // Saturate general aggregates first: fast fixed phases and paired actor
    // update/draw must remain observable after a cold-resource flood.
    for (int i = 0; i < 4300; ++i) Record("resource.cold_load", std::to_string(i), 0, 10000000);
    const char* fixedCategories[] = {"play_draw", "play_draw_detail", "play_update", "actor_draw_all",
                                    "actor_draw_detail", "actor_update_detail", "weather"};
    for (const auto category : fixedCategories) {
        for (int i = 0; i < 10; ++i) {
            Record(category, std::to_string(i), 0, 1);
            Record(category, std::to_string(i), 0, 2);
        }
    }
    for (int i = 0; i < 80; ++i) {
        const auto name = "id=0001 params=" + std::to_string(i);
        Record("actor_update", name, 0, 1000 + i);
        Record("actor_update", name, 0, 1);
        Record("actor_draw", name, 0, 1);
    }
    const auto reserved = EndTick();
    assert(reserved["phase_aggregates"].size() == 70);
    assert(reserved["overflow_phases"] == 0);
    for (const auto& phase : reserved["phase_aggregates"]) {
        assert(phase["count"] == 2 && phase["total_ns"] == 3);
    }
    assert(reserved["actor_aggregates"].size() == 80);
    assert(reserved["actor_keys"] == 80 && reserved["actor_keys_omitted"] == 0);
    assert(reserved["overflow_actors"] == 0);
    for (const auto& actor : reserved["actor_aggregates"]) {
        assert(actor["update_count"] == 2 && actor["draw_count"] == 1);
        assert(actor["draw_ns"] == 1);
    }
    assert(reserved["resource_aggregates"].size() == 4096);
    assert(reserved["resource_keys"] == 4096 && reserved["resource_key_limit"] == 4096);
    assert(reserved["overflow_resources"] == 204);
    assert(reserved["emission_truncated"] == false);
    BeginTick(10);
    for (int i = 0; i < 4300; ++i) Record("noise", std::to_string(i), 0, 1);
    for (int i = 0; i < 90; ++i) {
        const auto name = "texture/" + std::to_string(i);
        Record("resource.queue_delay", name, 0, 100, 123, 456);
        Record("resource.deserialize_import", name, 100, 300, 123, 456);
    }
    const auto complete = EndTick();
    assert(complete["resource_aggregates"].size() == 90);
    assert(complete["overflow_resources"] == 0 && complete["overflow_aggregates"].get<int>() > 0);
    for (const auto& resource : complete["resource_aggregates"]) {
        assert(resource["manager"] == 123 && resource["cache_owner"] == 456 && resource["origin_tick"] == 10);
        assert(resource["stages"]["resource.queue_delay"]["total_ns"] == 100);
        assert(resource["stages"]["resource.deserialize_import"]["total_ns"] == 200);
    }
    BeginTick(11);
    for (int i = 0; i < 20; ++i)
        Record(("resource.stage." + std::to_string(i)).c_str(), "many-stages", 0, 1);
    Record("resource.wait", "", 0, 1);
    { Scope longName("resource.cold_load", std::string(700, 'x')); }
    const std::string category = "resource." + std::string(200, 'c'), detail(200, 'd');
    Record(category.c_str(), "long-category", 0, 1, 0, 0, detail.c_str());
    const auto explicitLimits = EndTick();
    assert(explicitLimits["overflow_resource_stages"] == 4);
    assert(explicitLimits["truncated_names"] == 1 && explicitLimits["truncated_categories"] == 1);
    assert(explicitLimits["truncated_details"] == 1 && explicitLimits["unknown_names"] == 1);
    assert(explicitLimits["resource_stage_limit"] == 16);
    const auto reset = EndTick();
    assert(reset["resource_aggregates"].empty() && reset["overflow_resource_stages"] == 0 && reset["unknown_names"] == 0);
    std::cout << "PerformanceTrace: worker origin, late, disabled, stage attribution, aggregates and bounds passed\n";
}
