#!/usr/bin/env python3
"""Exercise production seasonal cycle, rain ownership and decoded PCM with native headers."""
from pathlib import Path
import os
import subprocess
import tempfile
import sys

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='rain-transition-') as directory:
    out = Path(directory)
    sanitize = ['-fsanitize=address,undefined,bounds', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-fno-pie'] if '--sanitize' in sys.argv else []
    common = ['-g', '-O1', '-ffunction-sections', '-fdata-sections', *sanitize,
              '-DCVAR_PREFIX_AUDIO="gAudioEditor"', '-DCVAR_PREFIX_SETTING="gSettings"',
              '-Itests/rain_transition/stubs', '-Isoh/tests/rain_engine_stubs', '-Isoh/tests',
              '-Isoh/include', '-Isoh/src', '-Isoh/assets', '-Isoh', '-Ilibultraship/include', '-Icombo']
    sources = ['tests/rain_transition/continuity_test.cpp',
               'soh/soh/Enhancements/audio/SeasonWeather.cpp',
               'soh/soh/Enhancements/audio/GlobalOutdoorRain.cpp',
               'soh/soh/Enhancements/audio/WeatherSamplePlayer.cpp',
               'soh/src/code/concurrent_weather_audio.c']
    objects = []
    for source in sources:
        cpp = source.endswith('.cpp')
        obj = out / (Path(source).name + '.o')
        subprocess.run([os.environ.get('CXX' if cpp else 'CC', 'c++' if cpp else 'cc'),
                        '-std=c++20' if cpp else '-std=c11', *common, '-c', source, '-o', str(obj)],
                       cwd=ROOT, check=True)
        objects.append(str(obj))
    binary = out / 'continuity_test'
    subprocess.run([os.environ.get('CXX', 'c++'), *sanitize, *(['-no-pie'] if sanitize else []),
                    *objects, '-Wl,--gc-sections', '-lm', '-o', str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], cwd=ROOT, check=True,
                   env={**os.environ, "ASAN_OPTIONS": os.environ.get("ASAN_OPTIONS", "") + ":detect_leaks=0"})
