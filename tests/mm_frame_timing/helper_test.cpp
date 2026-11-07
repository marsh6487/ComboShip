#include "2s2h/Enhancements/Debug/FrameTiming.h"
#include <cassert>
#include <stdexcept>

static int begins = 0, ends = 0, shutdowns = 0, finished = 0;
static bool active = false;
extern "C" int FrameTiming_IsActive(void) { return active; }
extern "C" void FrameTiming_BeginFrame(FrameTimingContext, int enabled) { ++begins; active = enabled; }
extern "C" void FrameTiming_EndFrame(FrameTimingContext, int) { ++ends; active = false; }
extern "C" void FrameTiming_Shutdown(void) { ++shutdowns; }
extern "C" void FrameTiming_Count(const char*, uint64_t) { }
static void Finish() { ++finished; }

struct Window {
    bool collecting = false;
    int writes = 0;
    void SetCollectFrameTimings(bool value) { collecting = value; ++writes; }
};
struct Interpreter {
    bool collecting = false;
    bool mRenderCostEveryFrame = false;
    bool failEnable = false;
    int writes = 0;
    void SetCollectRenderCosts(bool value) {
        collecting = value;
        ++writes;
        if (value && failEnable) throw std::runtime_error("enable");
    }
};

static void EarlyReturn(Window& window, Interpreter& interpreter) {
    MMFrameTiming::CollectionScope guard(&window, &interpreter, true, true, true);
    assert(window.collecting && interpreter.collecting && interpreter.mRenderCostEveryFrame);
    return;
}
int main() {
    for (unsigned bits = 0; bits != 16; ++bits) {
        assert(MMFrameTiming::ShouldCapture(bits & 1, bits & 2, bits & 4, bits & 8) == (bits == 15));
    }
    MMFrameTiming::Session session;
    const FrameTimingContext context{ 5, 0, 0, 0, 0, 60 };
    session.Suspend();
    session.Begin(context, false);
    session.End(context, false);
    session.Shutdown();
    assert(begins == 0 && ends == 0 && shutdowns == 0);
    session.Begin(context, true);
    assert(active && begins == 1);
    session.End(context, true);
    assert(!active && ends == 1);
    session.Begin(context, false); // Leaving shop, inactive host or toggle off flushes once.
    assert(begins == 2);
    session.Begin(context, false);
    assert(begins == 2);
    session.Begin(context, true);
    session.End(context, false); // Eligibility can change during the tick.
    assert(!active && begins == 4 && ends == 2);
    session.Begin(context, true);
    session.Begin(context, true); // An early bridge exit must not orphan the old trace.
    assert(ends == 3);
    session.Shutdown();
    assert(!active && shutdowns == 1);
    session.Shutdown();
    assert(shutdowns == 1);
    MMFrameTiming::Session handoff;
    handoff.Begin(context, true);
    handoff.End(context, true);
    const int beginsBeforeHandoff = begins, endsBeforeHandoff = ends, shutdownsBeforeHandoff = shutdowns;
    handoff.Suspend(); // Ownership can change after the last tick has already ended.
    assert(!handoff.Engaged() && !active);
    assert(begins == beginsBeforeHandoff + 1 && ends == endsBeforeHandoff + 1);
    assert(shutdowns == shutdownsBeforeHandoff); // Keep cumulative file/time budget.
    handoff.Suspend();
    assert(begins == beginsBeforeHandoff + 1);
    handoff.Begin(context, true);
    assert(handoff.Engaged() && active);
    handoff.Shutdown();
    try {
        MMFrameTiming::FrameEndScope end(Finish);
        throw std::runtime_error("early tick exit");
    } catch (const std::runtime_error&) { }
    assert(finished == 1);
    {
        MMFrameTiming::FrameEndScope end(Finish);
        end.Finish();
    }
    assert(finished == 2);
    Window window;
    Interpreter interpreter;
    interpreter.mRenderCostEveryFrame = true; // Disabled path must preserve pre-existing state too.
    {
        MMFrameTiming::CollectionScope guard(&window, &interpreter, false, true, false);
    }
    assert(window.writes == 0 && interpreter.writes == 0 && interpreter.mRenderCostEveryFrame);
    interpreter.mRenderCostEveryFrame = false;
    EarlyReturn(window, interpreter);
    assert(!window.collecting && !interpreter.collecting && !interpreter.mRenderCostEveryFrame);
    try {
        MMFrameTiming::CollectionScope guard(&window, &interpreter, true, true, true);
        throw std::runtime_error("draw");
    } catch (const std::runtime_error&) { }
    assert(!window.collecting && !interpreter.collecting && !interpreter.mRenderCostEveryFrame);
    interpreter.failEnable = true;
    try {
        MMFrameTiming::CollectionScope guard(&window, &interpreter, true, true, true);
        assert(false);
    } catch (const std::runtime_error&) { }
    assert(!window.collecting && !interpreter.collecting && !interpreter.mRenderCostEveryFrame);
    interpreter.failEnable = false;
    {
        MMFrameTiming::CollectionScope guard(&window, &interpreter, true, false, false);
        assert(window.collecting && !interpreter.collecting && !interpreter.mRenderCostEveryFrame);
    }
    assert(!window.collecting && !interpreter.collecting && !interpreter.mRenderCostEveryFrame);
}
