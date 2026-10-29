#!/usr/bin/env python3
"""Run the actual XML sample/font factories and native silence branches.

Archive lookup and codec workers are controlled boundaries; XML, resource
types/lifetimes, sample metadata, font parsing and synthesis branches are real.
No game assets, audio-device library or system XML installation are required.
"""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r"^(?:[\w:<>,*&]+[ \t\n]+)+(?:SOH::)?" + re.escape(name) + r"\([^;]*?\)\s*\{", source, re.M)
    if match is None:
        raise RuntimeError(f"Cannot locate production function {name}")
    depth = 0
    for pos in range(source.index("{", match.start()), len(source)):
        depth += (source[pos] == "{") - (source[pos] == "}")
        if depth == 0:
            return source[match.start():pos + 1]
    raise RuntimeError(f"Unterminated production function {name}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--case", choices=("all", "valid", "missing", "font"), default="all")
    parser.add_argument("--game", choices=("all", "oot", "mm"), default="all")
    parser.add_argument("--mode", choices=("all", "solo", "combo"), default="all")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="missing-audio-") as directory:
        build = Path(directory)
        (build / "spdlog").mkdir()
        (build / "spdlog/spdlog.h").write_text("#pragma once\n#define SPDLOG_TRACE(...) ((void)0)\n"
            "#ifndef SPDLOG_ERROR\n#define SPDLOG_ERROR(...) ((void)0)\n#endif\n")
        flags = ["-std=c++20", "-g", "-Wall", "-Wextra", "-Wno-unused-variable",
                 "-Wno-unused-parameter", "-pthread", *shlex.split(os.environ.get("AUDIO_MISSING_CXXFLAGS", ""))]
        if args.sanitize:
            flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer",
                      "-fno-pie", "-no-pie"]
        for game, source_root, type_root, native in (
            ("oot", "soh/soh", "soh", "soh/src/code/audio_synthesis.c"),
            ("mm", "mm/2s2h", "2s2h", "mm/src/audio/lib/synthesis.c"),
        ):
            if args.game not in ("all", game):
                continue
            sample = (ROOT / source_root / "resource/importer/AudioSampleFactory.cpp").read_text()
            font = (ROOT / source_root / "resource/importer/AudioSoundFontFactory.cpp").read_text()
            bodies = [function(sample, "ResourceFactoryXMLAudioSampleV0::ReadResource"),
                      function(sample, "ResourceFactoryXMLAudioSampleV0::CodecStrToInt")]
            bodies += [function(font, "ResourceFactoryXMLSoundFontV0::" + name) for name in
                       ("MediumStrToInt", "CachePolicyToInt", "ParseDrums", "ParseInstruments", "ParseSfxTable",
                        "ParseEnvelopes", "ReadResource")]
            (build / "factory.inc").write_text("namespace SOH {\n" + "\n".join(bodies) + "\n}\n")
            synthesis = (ROOT / native).read_text()
            start = synthesis.index("                    case CODEC_S16_INMEMORY:")
            end = synthesis.index("                    case CODEC_S16:", start)
            # MM shares the native silence branch with CODEC_UNK6. OoT's
            # following branch is CODEC_S16/OPUS. Preserve each exact branch.
            silence = synthesis[start:end]
            if game == "mm":
                silence = silence[:silence.index("                    case CODEC_OPUS:")]
            (build / "silence.inc").write_text(silence)
            native_header = "soh/include/z64audio.h" if game == "oot" else "mm/include/audio/soundfont.h"
            enums = (ROOT / native_header).read_text()
            (build / "codec.inc").write_text("\n".join(re.search(
                r"typedef enum(?: " + name + r")?\s*\{[^}]*\}\s*" + name + r";", enums, re.S)[0]
                for name in ("SampleCodec", "SampleMedium")))
            cache = (ROOT / "mm/include/audio/load.h").read_text() if game == "mm" else enums
            with (build / "codec.inc").open("a") as output:
                output.write("\n" + re.search(r"typedef enum\s*\{[^}]*\}\s*AudioCacheType;", cache)[0])
            for combo in (False, True):
                if args.mode != "all" and (args.mode == "combo") != combo:
                    continue
                binary = build / (game + ("-combo" if combo else "-solo"))
                command = [*shlex.split(os.environ.get("CXX", "c++")), *flags,
                           "-DTEST_MM=" + str(int(game == "mm")),
                           '-DTEST_GAME="' + game + '"', '-DTEST_TYPE_ROOT="' + type_root + '"',
                           "-I" + str(build), "-I" + str(ROOT / "libultraship/include"),
                           "-I" + str(ROOT / ("mm" if game == "mm" else "soh")),
                           "-I" + str(ROOT / source_root),
                           "-I" + str(ROOT / "ZAPDTR/lib/tinyxml2")]
                if combo:
                    command.append("-DCOMBO_BUILD")
                command += [str(ROOT / "tests/audio_missing_sample/factory_test.cpp"),
                            str(ROOT / "libultraship/src/ship/resource/Resource.cpp"),
                            str(ROOT / "libultraship/src/ship/resource/ResourceFactoryXML.cpp"),
                            str(ROOT / source_root / "resource/type/AudioSample.cpp"),
                            str(ROOT / source_root / "resource/type/AudioSoundFont.cpp"),
                            str(ROOT / "ZAPDTR/lib/tinyxml2/tinyxml2.cpp"), "-o", str(binary)]
                subprocess.run(command, check=True)
                subprocess.run([str(binary), args.case], check=True, timeout=15)


if __name__ == "__main__":
    main()
