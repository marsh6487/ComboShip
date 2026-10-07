#!/usr/bin/env python3
"""Execute the cape receipt loader and native confirmation with actual engine/save headers."""
from pathlib import Path
import argparse
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
sys.path.insert(0, str(ROOT / 'tests/mm_presentation'))
from run_foreign_scale_tests import function


def flags(game):
    paths = (game, f'{game}/include', f'{game}/include/PR', f'{game}/src', f'{game}/assets',
             f'{game}/2s2h' if game == 'mm' else 'soh/soh', 'libultraship/include',
             'combo', 'combo/menu')
    return ['-DF3DEX_GBI_2', '-DCOMBO_BUILD', '-DCONTROLLERBUTTONS_T=uint32_t',
            '-DLOG_LEVEL_GAME_PRINTS=0', '-DNON_EQUIVALENT', '-DNON_MATCHING',
            *['-I' + str(ROOT / path) for path in paths]]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--queued-fallback', type=Path, action='append', default=[])
    args = parser.parse_args()
    sanitize = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g', '-fno-pie', '-no-pie'] \
        if args.sanitize else []
    with tempfile.TemporaryDirectory(prefix='cape-choice-') as directory:
        build = Path(directory)
        source = (ROOT / 'mm/2s2h/CustomMessage/CustomMessage.cpp').read_text()
        names = ('CustomMessage::Replace', 'CustomMessage::ReplaceColorChars', 'CustomMessage::AddLineBreaks',
                 'CustomMessage::EnsureMessageEnd', 'CustomMessage::LoadCustomMessageIntoFont')
        loader = (ROOT / 'tests/cape_choice/loader_test.cpp').read_text()
        loader = loader.replace('/* PRODUCTION_FORMATTERS */', '\n'.join(function(source, name) for name in names))
        test = build / 'loader.cpp'
        test.write_text(loader)
        binary = build / 'loader'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', *flags('mm'), *sanitize,
                        str(test), '-o', str(binary)], check=True, capture_output=True, text=True)
        subprocess.run([str(binary), *map(str, args.queued_fallback)], check=True)
        for game, path in (('mm', 'mm/src/code/z_message.c'), ('soh', 'soh/src/code/z_message_PAL.c')):
            engine = (ROOT / path).read_text()
            if 'Message_HandleCapeVisibilityChoice' in engine:
                production = ('static u8 sCapeVisibilityChoice;\n' +
                              function(engine, 'Message_SetCapeVisibilityChoice') + '\n' +
                              function(engine, 'Message_HandleCapeVisibilityChoice'))
            else:
                production = ('void Message_SetCapeVisibilityChoice(int enabled) { (void)enabled; }\n'
                              'static int Message_HandleCapeVisibilityChoice(PlayState* play) { (void)play; return 0; }')
            if game == 'soh':
                # Execute the actual registered message-state serializer, with
                # its original field declarations and the production macros.
                serializer = engine[engine.index('#define MESSAGE_PAL_SHIP_SAVESTATE_FIELDS'):]
                declarations = []
                for field in re.findall(r'F\((\w+)\)', serializer):
                    if field == 'sCapeVisibilityChoice':
                        continue
                    declarations.append(re.search(
                        r'^(?:static\s+)?(?:u8|s16|u16|u32)\s+' + field + r'\b[^;]*;',
                        engine, re.M).group(0))
                production += ('\n#include "soh/Enhancements/savestate_serialize.h"\n' +
                               '\n'.join(declarations) + '\n' + serializer)
            fixture = (ROOT / 'tests/cape_choice/native_choice_test.c').read_text()
            test = build / (game + '.c')
            test.write_text(fixture.replace('/* PRODUCTION_CHOICE */', production))
            binary = build / game
            extra = ['-DCAPE_TEST_MM'] if game == 'mm' else []
            subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu11', *flags(game), *extra, *sanitize,
                            str(test), '-o', str(binary)], check=True, capture_output=True, text=True)
            subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    try:
        main()
    except subprocess.CalledProcessError as error:
        if error.stdout:
            print(error.stdout)
        if error.stderr:
            print(error.stderr)
        raise
