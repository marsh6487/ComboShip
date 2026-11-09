#!/usr/bin/env python3
"""Native MM CVar input -> ShipInit -> real pool metrics/exclusions, with real ImGui."""
import os
from pathlib import Path
import sys
import subprocess

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tests/shuffle_parity/boss_keys'))
from run_tests import function


def prepare(build):
    menu_path = 'mm/2s2h/Rando/Menu.cpp'
    menu = subprocess.check_output(['git', 'show', os.environ['UI_TEST_MENU_REV'] + ':' + menu_path],
                                   cwd=ROOT, text=True) if os.environ.get('UI_TEST_MENU_REV') else (ROOT / menu_path).read_text()
    widgets = (ROOT / 'mm/2s2h/BenGui/UIWidgets.cpp').read_text()
    foundation = (ROOT / 'tests/shuffle_parity/key_rings/native_test.cpp').read_text().split('int main()')[0]
    foundation = foundation.replace('int32_t CVarGetInteger(const char*, int32_t fallback) { return fallback; }',
        'int32_t CVarGetInteger(const char* key, int32_t fallback) {\n'
        '    auto found = values.find(key); return found == values.end() ? fallback : found->second;\n}')
    (build / 'foundation.inc').write_text(foundation)
    start = menu.index('static const float POOL_BALANCE_UNLIKELY_RATIO')
    end = menu.index('// Shop checks', start)
    (build / 'metrics.inc').write_text(menu[start:end])
    # Overloaded helpers are extracted by complete signature (the common native
    # function extractor intentionally returns just the first matching overload).
    def exact(signature):
        start = widgets.index(signature)
        return widgets[start:widgets.index('\n}', start) + 2]
    (build / 'widgets.inc').write_text('\n'.join(exact(s) for s in (
        'std::string WrappedText(const char*', 'std::string WrappedText(const std::string&',
        'void PushStyleCheckbox(const ImVec4&', 'void PushStyleCheckbox(Colors',
        'void PopStyleCheckbox()', 'void RenderText(', 'bool Checkbox(', 'bool CVarCheckbox(')))
    start = menu.index('    const char* ringLabels[]')
    end = menu.index('    ImGui::SeparatorText("Other Dungeon Items")', start)
    (build / 'rings.inc').write_text(menu[start:end])
    start = menu.index('    CVarCheckbox("Rusty Overworld Door Keys"')
    # Include the surrounding release scope when present, while retaining the
    # original unscoped body for the pre-fix red test.
    scope = menu.rfind('    {\n', 0, start)
    if scope >= 0 and 'WhatsNew::Scope' in menu[scope:start]:
        start = scope
    end = menu.index('    ImGui::SeparatorText("Small-Key Rings")', start)
    (build / 'rusty.inc').write_text(menu[start:end])
    (build / 'exclusion.inc').write_text(function(menu, 'IsCheckShownAsShuffled'))
    generation = (ROOT / 'mm/2s2h/Rando/Logic/SeedGeneration.cpp').read_text()
    (build / 'generation.inc').write_text(function(generation, 'GenerateSeedInSaveContext'))
    tracker_path = 'mm/2s2h/Rando/CheckTracker/CheckTracker.cpp'
    tracker = subprocess.check_output(['git', 'show', os.environ['VANILLA_TEST_TRACKER_REV'] + ':' + tracker_path],
                                     cwd=ROOT, text=True) if os.environ.get('VANILLA_TEST_TRACKER_REV') else (ROOT / tracker_path).read_text()
    (build / 'tracker_total.inc').write_text(function(tracker, 'GetTotalCheckCount'))
    legacy_policy = ''
    if 'std::vector<int32_t> incompatibleWithVanilla' in menu:
        start = menu.index('std::vector<int32_t> incompatibleWithVanilla')
        legacy_policy = menu[start:menu.index('\n};', start) + 3] + '\n'
    (build / 'compatibility.inc').write_text(legacy_policy + function(menu, 'ClearIncompatibleSetting') + '\n' +
                                           function(menu, 'IncompatibleWithLogicSetting'))
    import_path = 'mm/2s2h/Rando/Spoiler/Apply.cpp'
    imports = subprocess.check_output(['git', 'show', os.environ['VANILLA_TEST_IMPORT_REV'] + ':' + import_path],
                                      cwd=ROOT, text=True) if os.environ.get('VANILLA_TEST_IMPORT_REV') else (ROOT / import_path).read_text()
    (build / 'import.inc').write_text(function(imports, 'ApplyToSaveContext'))
    starts = (ROOT / 'mm/2s2h/Rando/StartingItems.cpp').read_text()
    priority = (ROOT / 'mm/2s2h/Rando/MiscBehavior/SariasSongHint.cpp').read_text()
    (build / 'spoiler_start.inc').write_text(function(starts, 'GetStartingItemsFromSpoiler') + '\n' +
        function(priority, 'GetSariaPriorityItemsFromSpoiler').replace('Rando::GetSariaPriorityItemsFromSpoiler', 'GetSariaPriorityItemsFromSpoiler'))


# Reuse the native pool fixture's compile protocol and exact catalogue/service
# extraction; add only UI sources and this probe, without a second model pool.
source = (ROOT / 'tests/shuffle_parity/key_rings/run_tests.py').read_text()
source = source.replace('ROOT = Path(__file__).resolve().parents[3]', 'ROOT = Path(' + repr(str(ROOT)) + ')')
source = source.replace("build = Path(td)", "build = Path(td)\n    prepare(build)")
imgui = Path(os.environ['IMGUI_DIR'])
source = source.replace("flags = ['-std=c++20'", "flags = ['-DIMGUI_DEFINE_MATH_OPERATORS=', '-I" + str(imgui) + "', '-std=c++20'")
source = source.replace("str(args.fixture.resolve()),", "str(args.fixture.resolve()), *" + repr([str(imgui / n) for n in ('imgui.cpp', 'imgui_draw.cpp', 'imgui_tables.cpp', 'imgui_widgets.cpp')]) + ",")
exec(compile(source, str(ROOT / 'tests/shuffle_parity/key_rings/run_tests.py'), 'exec'))
