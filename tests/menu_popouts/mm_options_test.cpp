#include "2s2h/BenGui/MenuTypes.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

/* PRODUCTION_ENUM_LABEL_HELPER */

using namespace UIWidgets;

static void Check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL MM menu popout: %s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

// Only the menu registry is a boundary. The native WidgetInfo, Options overloads
// and WindowButtonOptions payload are compiled unchanged from the real headers.
static WidgetInfo sWidget;
static unsigned sRegistrations;
static WidgetInfo& AddWidget(WidgetPath& path, const char* name, WidgetType type) {
    Check(path.sectionName == "Enhancements", "registration keeps its menu section");
    Check(type == WIDGET_WINDOW_BUTTON, "registration creates a window button");
    sWidget = WidgetInfo{};
    sWidget.name = name;
    sWidget.type = type;
    ++sRegistrations;
    return sWidget;
}

static void RegisterPopout() {
    WidgetPath path{ "Enhancements", "Graphics", SECTION_COLUMN_1 };
    /* PRODUCTION_POPOUT_REGISTRATION */
}

int main() {
    // Menu export initializes MM while OoT may be active; construction must not
    // require a running MM game or an ImGui context, including repeated starts.
    for (unsigned attempt = 0; attempt < 2; ++attempt) {
        try {
            RegisterPopout();
        } catch (const std::bad_variant_access& error) {
            std::fprintf(stderr, "FAIL MM menu popout: native registration threw %s\n", error.what());
            return EXIT_FAILURE;
        }
        Check(sRegistrations == attempt + 1, "one widget is registered on each fresh construction");
        Check(sWidget.cVar && std::strcmp(sWidget.cVar, "gWindows.SpinAttackMM") == 0,
              "popout retains its visibility CVar");
        Check(sWidget.windowName && std::strcmp(sWidget.windowName, "Spin Attack##MM") == 0,
              "popout routes to the registered MM window");
        Check(sWidget.hideInSearch, "popout stays outside searchable setting results");
        Check(sWidget.options != nullptr, "native options are retained");
        // This is the same payload type consumed by Menu::MenuDrawItem.
        auto options = std::static_pointer_cast<WindowButtonOptions>(sWidget.options);
        Check(options->showButton, "the popout button remains visible");
        Check(!options->embedWindow, "the compact popout does not embed its window contents");
        Check(options->tooltip && options->tooltip[0] != '\0', "the button retains a tooltip");
    }
    std::puts("PASS native MM popout registration, typed options and window routing without game/UI initialization");
}
