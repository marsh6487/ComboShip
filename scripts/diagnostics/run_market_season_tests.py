#!/usr/bin/env python3
"""Check Market suite tint eligibility and actual resource-cache bindings."""
from pathlib import Path
import argparse
import json
import os
import re
import subprocess
import tempfile


def function(source, name):
    match = re.search(r'^.*\b' + name + r'\([^;{}]*\)\s*\{', source, re.M)
    assert match, name
    end = source.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--spdlog-include', type=Path)
    parser.add_argument('--resolver-only', action='store_true')
    parser.add_argument('--archive-fixtures', type=Path)
    args = parser.parse_args()
    root = args.root.resolve()
    data = json.loads((root / 'tests/seasons/market_suite_targets.json').read_text())
    table = (root / 'soh/include/tables/scene_table.h').read_text()
    scene_ids = {name: int(value, 16) for value, name in
                 re.findall(r'/\* (0x[0-9A-F]+) \*/ DEFINE_SCENE\([^,]+,[^,]+, (SCENE_\w+),', table)}
    names = data['scenes'] + ['SCENE_KOKIRI_FOREST', 'SCENE_POTION_SHOP_MARKET']
    enums = 'enum { ' + ', '.join(name + '=' + str(scene_ids[name]) for name in names) + ' };'
    weather = (root / 'soh/soh/Enhancements/audio/SeasonWeather.cpp').read_text()
    controller = function(weather, 'ResolveSeasonForPlay')
    for name in ('SeasonWeather_IsMarketSuiteExterior', 'SeasonWeather_FoliageSeasonForPlay'):
        if re.search(r'\b' + name + r'\([^;{}]*\)\s*\{', weather):
            controller += '\n' + function(weather, name)
    controller += '\n' + function(weather, 'SeasonWeather_SeasonForPlay')
    selected = 'SeasonWeather_FoliageSeasonForPlay' if 'SeasonWeather_FoliageSeasonForPlay' in controller else 'SeasonWeather_SeasonForPlay'
    source = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
struct Camera { unsigned unk_14C = 0; } camera;
struct PlayState {
    int sceneNum = 0;
    struct { bool indoors = true, skyboxDisabled = true, customSkyboxFilter = false; } envCtx;
    struct { int state = 0; } csCtx, gameOverCtx;
};
struct { int gameMode = 0; } gSaveContext;
constexpr int CS_STATE_IDLE = 0, GAMEOVER_INACTIVE = 0, GAMEMODE_NORMAL = 0, SEASON_COUNT = 4;
#define GET_ACTIVE_CAM(play) (&::camera)
#define SOH_SEASON_WEATHER_CVAR(name) name
int selection = 3, owned = 1, savedSeason = 2, getterCalls = 0;
int CVarGetInteger(const char*, int) { return selection; }
int Seasons_SeasonCount() { return owned; }
uint8_t Seasons_GetSeason() { ++getterCalls; return savedSeason; }
/* ENUMS */
/* CONTROLLER */
int main() {
    PlayState play;
    for (int scene : {/* MARKET_SCENES */}) {
        play.sceneNum = scene;
        for (selection = 0; selection < 4; ++selection) {
            assert(/* SELECTED */(&play) == selection);
            assert(SeasonWeather_SeasonForPlay(&play) == -1);
        }
        selection = -2; owned = 0;
        assert(/* SELECTED */(&play) == -1);
        assert(getterCalls == 0);
        owned = 1;
        assert(/* SELECTED */(&play) == 2);
        getterCalls = 0; selection = 3;
        play.csCtx.state = 1; assert(/* SELECTED */(&play) == -1); play.csCtx.state = 0;
        camera.unk_14C = 0x100; assert(/* SELECTED */(&play) == -1); camera.unk_14C = 0;
        play.envCtx.customSkyboxFilter = true; assert(/* SELECTED */(&play) == -1); play.envCtx.customSkyboxFilter = false;
        play.gameOverCtx.state = 1; assert(/* SELECTED */(&play) == -1); play.gameOverCtx.state = 0;
        gSaveContext.gameMode = 1; assert(/* SELECTED */(&play) == -1); gSaveContext.gameMode = 0;
        selection = -1; assert(/* SELECTED */(&play) == -1);
    }
    selection = 3;
    play.sceneNum = SCENE_POTION_SHOP_MARKET; assert(/* SELECTED */(&play) == -1);
    play.sceneNum = SCENE_KOKIRI_FOREST; assert(/* SELECTED */(&play) == -1);
    play.envCtx.indoors = play.envCtx.skyboxDisabled = false;
    assert(/* SELECTED */(&play) == 3 && SeasonWeather_SeasonForPlay(&play) == 3);
    play.envCtx.skyboxDisabled = true; assert(/* SELECTED */(&play) == -1);
    assert(/* SELECTED */(nullptr) == -1);
    std::puts("PASS all 11 Market exterior tint guards; weather, indoor shops, ownership, Native and story guards retained");
}
'''
    source = source.replace('/* ENUMS */', enums).replace('/* CONTROLLER */', controller)
    source = source.replace('/* MARKET_SCENES */', ', '.join(data['scenes'])).replace('/* SELECTED */', selected)
    # Match the existing foliage gate's compiled-library logging boundary.
    # Pixel/cache fixtures do not log; avoid linking system spdlog/fmt bodies.
    flags = ['-std=c++20', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
             '-DSPDLOG_ACTIVE_LEVEL=6', '-DSPDLOG_COMPILED_LIB']
    if args.sanitize:
        flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie']
    with tempfile.TemporaryDirectory(prefix='market-season-') as folder:
        out = Path(folder)
        cpp = out / 'resolver.cpp'; cpp.write_text(source)
        subprocess.run(['c++', *flags, str(cpp), '-o', str(out / 'resolver')], check=True)
        subprocess.run([str(out / 'resolver')], check=True)
        if args.resolver_only:
            return
        fixture = (root / 'soh/tests/season_foliage_market_test.cpp').read_text()
        cases = ',\n'.join('{' + json.dumps(t['path']) + ', ' + t['scene'] + ', Surface::' + t['surface'] +
                           ', {' + ', '.join(map(str, t['region'])) + '}, PixelMask::' + t['mask'] + '}'
                           for t in data['targets'])
        fixture = fixture.replace('/* MARKET_TARGETS */', cases)
        cpp = out / 'bindings.cpp'; cpp.write_text(fixture)
        includes = ['-I' + str(root / p) for p in ('soh/tests/season_foliage_stubs', 'soh/tests', 'soh', 'libultraship/include')]
        if args.spdlog_include:
            includes += ['-I' + str(args.spdlog_include.resolve())]
        subprocess.run(['c++', *flags, *includes, str(cpp),
                        str(root / 'libultraship/src/fast/resource/type/Texture.cpp'),
                        str(root / 'libultraship/src/ship/resource/Resource.cpp'), '-o', str(out / 'bindings')], check=True)
        arguments = []
        if args.archive_fixtures:
            for path, file in json.loads(args.archive_fixtures.read_text()):
                texture = Path(file)
                if not texture.is_absolute():
                    texture = args.archive_fixtures.resolve().parent / texture
                arguments += [path, str(texture)]
        subprocess.run([str(out / 'bindings'), *arguments], check=True)


if __name__ == '__main__':
    main()
