#!/usr/bin/env python3
"""Exercise the exact two menu CVar bindings and live independent preferences."""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
oot = (ROOT / 'soh/soh/SohGui/SohMenuRandomizer.cpp').read_text()
mm = (ROOT / 'mm/2s2h/Rando/Menu.cpp').read_text()
bindings = []
for category in ('Enemy', 'NPC'):
    label = 'Simpler ' + category + ' Soul Models'
    left = re.search(r'AddWidget\(\s*path\s*,\s*"' + label + r'"\s*,\s*WIDGET_CVAR_CHECKBOX\s*\)\s*\.CVar\(([^)]+)\)', oot)
    right = re.search(r'CVarCheckbox\(\s*"' + label + r'"\s*,\s*([^,]+),', mm)
    assert left and right, 'Both menus must expose ' + label
    bindings.append((left.group(1), right.group(1)))

with tempfile.TemporaryDirectory(prefix='soul-settings-') as directory:
    build = Path(directory)
    bridge = build / 'libultraship/bridge/consolevariablebridge.h'
    bridge.parent.mkdir(parents=True)
    bridge.write_text('#pragma once\nint CVarGetInteger(const char*, int);\n')
    code = '''#include <cassert>
#include <cstring>
#include <string>
#include <unordered_map>
std::unordered_map<std::string,int> preferences;
int CVarGetInteger(const char* key, int fallback) {
    auto it = preferences.find(key); return it == preferences.end() ? fallback : it->second;
}
#include "combo/menu/ComboSoulModelSettings.h"
int main() {
'''
    for i, (left, right) in enumerate(bindings):
        code += f'const char* oot{i} = {left}; const char* mm{i} = {right};\n'
        code += f'assert(std::strcmp(oot{i}, mm{i}) == 0);\n'
    code += '''
    assert(!ComboSoulModels::SimplerEnemy() && !ComboSoulModels::SimplerNpc());
    assert(ComboSoulModels::PreferenceMask() == 0);
    preferences[oot0] = 1;
    assert(CVarGetInteger(mm0, 0) && ComboSoulModels::SimplerEnemy());
    assert(!ComboSoulModels::SimplerNpc() && ComboSoulModels::PreferenceMask() == 1);
    preferences[mm1] = 1;
    assert(CVarGetInteger(oot1, 0) && ComboSoulModels::SimplerNpc());
    assert(ComboSoulModels::PreferenceMask() == 3);
    preferences[mm0] = 0;
    assert(!CVarGetInteger(oot0, 0) && !ComboSoulModels::SimplerEnemy());
    assert(ComboSoulModels::SimplerNpc() && ComboSoulModels::PreferenceMask() == 2);
    preferences[oot1] = 0;
    assert(!CVarGetInteger(mm1, 0) && ComboSoulModels::PreferenceMask() == 0);
    preferences["gRandoEnhancements.SimplerBossSoulModels"] = 1;
    assert(!ComboSoulModels::SimplerEnemy() && !ComboSoulModels::SimplerNpc());
}
'''
    (build / 'test.cpp').write_text(code)
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-I' + str(build), '-I' + str(ROOT),
                    str(build / 'test.cpp'), '-o', str(build / 'test')], check=True)
    subprocess.run([str(build / 'test')], check=True)
print('PASS: exact menu bindings share two live preferences, default detailed, all four states and independent boss option')
