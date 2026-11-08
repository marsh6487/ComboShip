#include "libultraship/bridge/windowbridge.h"
#include "libultraship/bridge/uivisibilitybridge.h"
#include "libultraship/bridge/consolevariablebridge.h"
#include "ship/window/Window.h"
#include "ship/Context.h"
#include <atomic>

namespace {
// One shared-module state for both native game modules and the GUI. This is
// deliberately independent of saved Minimal UI and minimap visibility settings.
std::atomic<unsigned> sUiHidden{ 1 };
} // namespace

extern "C" {

int UiVisibility_IsEnabled(void) {
    return CVarGetInteger("gEnhancements.TrueHiddenUI", 0) != 0;
}

int UiVisibility_IsHidden(void) {
    return UiVisibility_IsEnabled() && sUiHidden.load(std::memory_order_relaxed);
}

void UiVisibility_Toggle(void) {
    if (UiVisibility_IsEnabled()) {
        sUiHidden.fetch_xor(1, std::memory_order_relaxed);
    }
}

uint32_t WindowGetWidth() {
    return Ship::Context::GetRawInstance()->GetWindow()->GetWidth();
}

uint32_t WindowGetHeight() {
    return Ship::Context::GetRawInstance()->GetWindow()->GetHeight();
}

float WindowGetAspectRatio() {
    return Ship::Context::GetRawInstance()->GetWindow()->GetCurrentAspectRatio();
}

bool WindowIsRunning() {
    return Ship::Context::GetRawInstance()->GetWindow()->IsRunning();
}

int32_t WindowGetPosX() {
    return Ship::Context::GetRawInstance()->GetWindow()->GetPosX();
}

int32_t WindowGetPosY() {
    return Ship::Context::GetRawInstance()->GetWindow()->GetPosY();
}

bool WindowIsFullscreen() {
    return Ship::Context::GetRawInstance()->GetWindow()->IsFullscreen();
}

void WindowClose() {
    Ship::Context::GetRawInstance()->GetWindow()->Close();
}
}
