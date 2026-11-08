#include <cassert>
#include <string>
#include <vector>

enum { SECTION_COLUMN_1, WIDGET_CVAR_CHECKBOX };
struct WidgetPath { std::string menu, category; int column; };
struct CheckboxOptions {
    bool value = false;
    std::string tooltip;
    CheckboxOptions& DefaultValue(bool v) { value = v; return *this; }
    CheckboxOptions& Tooltip(std::string v) { tooltip = v; return *this; }
};
struct Widget {
    WidgetPath path;
    std::string label, cvar;
    CheckboxOptions options;
    Widget& CVar(std::string v) { cvar = v; return *this; }
    Widget& Options(CheckboxOptions v) { options = v; return *this; }
};
static std::vector<Widget> widgets;
Widget& AddWidget(WidgetPath path, std::string label, int type) {
    assert(type == WIDGET_CVAR_CHECKBOX);
    widgets.push_back({path, label});
    return widgets.back();
}

int main() {
    WidgetPath path = { "Enhancements", "Items/Songs", SECTION_COLUMN_1 };
    WidgetPath masksPath = { "Skijer's NEI", "Masks", SECTION_COLUMN_1 };
    /* NATIVE_WIDGETS */
    assert(widgets.size() == 2);
    assert(widgets[0].path.menu == "Enhancements" && widgets[0].path.category == "Items/Songs");
    assert(widgets[1].path.menu == "Skijer's NEI" && widgets[1].path.category == "Masks");
    for (const auto& widget : widgets) {
        assert(widget.cvar == "gEnhancements.HideNonTransformationMasks");
        assert(!widget.options.value);
        assert(widget.options.tooltip.find("effects") != std::string::npos);
        assert(widget.options.tooltip.find("Transformation masks remain visible") != std::string::npos);
    }
}
