#!/usr/bin/env python3
"""Run the production game picker at a scripted renderer boundary."""
from pathlib import Path
import argparse
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()
screen = (ROOT/'combo/gui/ComboExtractScreen.cpp').read_text()
screen = '\n'.join(line for line in screen.splitlines() if not (
    line.startswith('#include "') or
    line.startswith(('#include <libultraship/', '#include <ship/', '#include <fast/', '#include <imgui.h>'))))
if 'int ComboUI_SelectGame(' not in screen:
    screen += '\nint ComboUI_SelectGame(int) { return COMBO_GAME_BOTH; }\n'
source = '#include "tests/rom_setup/screen_stubs.h"\n' + screen + r'''
int main(int argc, char** argv) {
    assert(argc==2);
    const int scenario=std::stoi(argv[1]);
    Test::selectGame=scenario<3 ? scenario : -2;
    Test::selectQuit=scenario==3;
    assert(ComboUI_SelectGame(COMBO_GAME_MM)==(scenario<3 ? scenario : -1));
    assert(Test::disabled.empty());
}
'''
with tempfile.TemporaryDirectory(prefix='solo-game-picker-') as tmp:
    cpp=Path(tmp)/'screen.cpp';binary=Path(tmp)/'screen';cpp.write_text(source)
    flags=['-std=c++20','-Wall','-Wextra','-Werror','-I',str(ROOT)]
    if args.sanitize: flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    subprocess.run(['g++',*flags,str(cpp),'-o',str(binary)],check=True)
    for scenario in range(5):
        subprocess.run([str(binary),str(scenario)],check=True,cwd=tmp)
print('Game picker: Combo/OoT/MM, Quit and window close PASS')
