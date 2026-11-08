#!/usr/bin/env python3
"""Execute the native MM UI widget registration against its registry boundary."""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT/'mm/2s2h/BenGui/BenMenu.cpp').read_text()
match = re.search(r'    // UI visibility\n(.*?)    // HUD Editor', source, re.S)
registration = match.group(1) if match else ''
fixture = r'''
#include <cassert>
#include <string>
#include <vector>
enum { SECTION_COLUMN_1, WIDGET_CVAR_CHECKBOX };
struct WidgetPath { std::string menu, category; int column; };
struct CheckboxOptions {
    bool value = false; std::string tooltip;
    CheckboxOptions& DefaultValue(bool v) { value = v; return *this; }
    CheckboxOptions& Tooltip(std::string v) { tooltip = v; return *this; }
};
struct Widget {
    WidgetPath path; std::string label, cvar; CheckboxOptions options;
    Widget& CVar(std::string v) { cvar = v; return *this; }
    Widget& Options(CheckboxOptions v) { options = v; return *this; }
};
std::vector<Widget> widgets;
int sidebars;
void AddSidebarEntry(std::string menu, std::string category, int columns) {
    assert(menu=="Enhancements" && category=="UI" && columns==1); ++sidebars;
}
Widget& AddWidget(WidgetPath path, std::string label, int type) {
    assert(type==WIDGET_CVAR_CHECKBOX); widgets.push_back({path,label}); return widgets.back();
}
int main() {
    WidgetPath path;
    /* PRODUCTION */
    assert(sidebars==1 && widgets.size()==2);
    assert(widgets[0].label=="Minimal UI" && widgets[0].cvar=="gEnhancements.MinimalUI");
    assert(widgets[1].label=="True Hidden UI" && widgets[1].cvar=="gEnhancements.TrueHiddenUI");
    for (auto& w : widgets) {
        assert(w.path.menu=="Enhancements" && w.path.category=="UI");
        assert(!w.options.value && w.options.tooltip.find("L")!=std::string::npos);
    }
    assert(widgets[1].options.tooltip.find("settings")!=std::string::npos);
}
'''
with tempfile.TemporaryDirectory(prefix='mm-ui-menu-') as directory:
    path = Path(directory)/'test.cpp'
    path.write_text(fixture.replace('/* PRODUCTION */', registration))
    binary = Path(directory)/'test'
    subprocess.run(['c++','-std=c++20',str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('PASS native MM UI registration: category, both disabled defaults, L/recovery tooltips')
