#pragma once

#include "../../../../soh/soh/Enhancements/debugger/FrameTimingProbe.h"

namespace MMFrameTiming {
constexpr bool ShouldCapture(bool requested, bool liveGameplay, bool activeHost, bool shopInterior) {
    return requested && liveGameplay && activeHost && shopInterior;
}

// Only touch the shared renderer while this MM capture owns a collected draw.
// In particular the default disabled path does not reset another host's flags.
template <class Window, class Interpreter> class CollectionScope {
  public:
    CollectionScope(Window* window, Interpreter* interpreter, bool enabled, bool renderCosts, bool everyFrame)
        : window(window), interpreter(interpreter), enabled(enabled) {
        if (!enabled)
            return;
        try {
            window->SetCollectFrameTimings(true);
            interpreter->SetCollectRenderCosts(renderCosts);
            interpreter->mRenderCostEveryFrame = renderCosts && everyFrame;
        } catch (...) {
            Reset();
            throw;
        }
    }
    ~CollectionScope() {
        Reset();
    }
    CollectionScope(const CollectionScope&) = delete;
    CollectionScope& operator=(const CollectionScope&) = delete;

  private:
    void Reset() noexcept {
        if (!enabled)
            return;
        window->SetCollectFrameTimings(false);
        interpreter->SetCollectRenderCosts(false);
        interpreter->mRenderCostEveryFrame = false;
        enabled = false;
    }
    Window* window;
    Interpreter* interpreter;
    bool enabled;
};

class NamedScope {
  public:
    NamedScope(const char* category, const char* name) : category(category), name(name), span(FrameTiming_BeginSpan()) {
    }
    ~NamedScope() {
        FrameTiming_EndNamedSpan(category, name, span);
    }
    NamedScope(const NamedScope&) = delete;
    NamedScope& operator=(const NamedScope&) = delete;

  private:
    const char* category;
    const char* name;
    FrameTimingSpan span;
};

// The native adapter calls Begin only for eligible MM gameplay, or once to
// flush a capture that just lost eligibility. Initially disabled means no file,
// shared trace generation, clock reads or recorder-state changes.
class Session {
  public:
    bool Engaged() const {
        return engaged;
    }
    void Begin(FrameTimingContext context, bool eligible) {
        if (!eligible && !engaged)
            return;
        if (FrameTiming_IsActive()) {
            FrameTiming_Count("tick.previous_end_missing", 1);
            FrameTiming_EndFrame(lastContext, false);
        }
        FrameTiming_BeginFrame(context, eligible);
        engaged = eligible;
        initialized = true;
        lastContext = context;
    }
    void End(FrameTimingContext context, bool eligible) {
        if (!engaged)
            return;
        FrameTiming_EndFrame(context, eligible);
        if (!eligible) {
            FrameTiming_BeginFrame(context, false);
            engaged = false;
        }
    }
    void Suspend() {
        End(lastContext, false);
    }
    void Shutdown() {
        if (!initialized)
            return;
        End(lastContext, false);
        FrameTiming_Shutdown();
        engaged = initialized = false;
    }

  private:
    bool engaged = false;
    bool initialized = false;
    FrameTimingContext lastContext{};
};

// The Graph bridge has normal early exits and a render exception handler. Keep
// tick closure independent of each path through that function.
class FrameEndScope {
  public:
    explicit FrameEndScope(void (*finish)()) : finish(finish) {
    }
    ~FrameEndScope() {
        Finish();
    }
    void Finish() {
        if (finish) {
            const auto callback = finish;
            finish = nullptr;
            callback();
        }
    }
    FrameEndScope(const FrameEndScope&) = delete;
    FrameEndScope& operator=(const FrameEndScope&) = delete;

  private:
    void (*finish)();
};
} // namespace MMFrameTiming
