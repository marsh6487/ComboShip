#include <functional>
#include <unordered_map>

#include "soh/SohGui/MenuTypes.h"
#include "soh/SohGui/UIWidgets.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace UIWidgets;

static void Check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL SoH menu popout: %s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

// Only the menu registry is a boundary. Both injected production registration
// chains use the real WidgetInfo::Options overloads and native option payloads.
struct MenuBoundary {
    WidgetInfo widget{};
    unsigned registrations = 0;

    WidgetInfo& AddWidget(WidgetPath&, const char* name, WidgetType type) {
        Check(type == WIDGET_WINDOW_BUTTON, "registration creates a window button");
        widget = WidgetInfo{};
        widget.name = name;
        widget.type = type;
        ++registrations;
        return widget;
    }
};

static void CheckPopout(const WidgetInfo& widget, const char* cvar, const char* windowName) {
    Check(widget.cVar && std::strcmp(widget.cVar, cvar) == 0, "popout retains its visibility CVar");
    Check(widget.windowName && std::strcmp(widget.windowName, windowName) == 0,
          "popout routes to its registered window");
    Check(widget.hideInSearch, "popout stays outside searchable setting results");
    Check(widget.options != nullptr, "native options are retained");
    // This is the same payload type consumed by the native Menu::MenuDrawItem.
    auto options = std::static_pointer_cast<WindowButtonOptions>(widget.options);
    Check(options->showButton, "the popout button remains visible");
    Check(!options->embedWindow, "the popout does not embed its window contents");
    Check(!options->tooltip.empty(), "the button retains a tooltip");
}

int main() {
    auto mSohMenu = std::make_shared<MenuBoundary>();
    auto AddWidget = [&](WidgetPath& path, const char* name, WidgetType type) -> WidgetInfo& {
        return mSohMenu->AddWidget(path, name, type);
    };
    WidgetPath path{};

    // Construction must not require a game or ImGui context, including a fresh
    // registration after the previous menu's options have been released.
    for (unsigned attempt = 0; attempt < 2; ++attempt) {
        try {
            /* SOH_PAK_POPOUT_REGISTRATION */
            Check(mSohMenu->registrations == attempt * 2 + 1, "PAK registers exactly one widget");
            CheckPopout(mSohMenu->widget, "gOpenWindows.PakLoader", "PAK Loader");

            /* SOH_SPIN_POPOUT_REGISTRATION */
            Check(mSohMenu->registrations == attempt * 2 + 2, "OoT spin registers exactly one widget");
            CheckPopout(mSohMenu->widget, "gOpenWindows.SpinAttack", "Spin Attack##OOT");
        } catch (const std::bad_variant_access& error) {
            std::fprintf(stderr, "FAIL SoH menu popout: native registration threw %s\n", error.what());
            return EXIT_FAILURE;
        }
    }
    std::puts("PASS native SoH PAK and spin popout registration, typed options and window routing "
              "without game/UI initialization");
}
