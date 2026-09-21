"""Compile selected MM production function bodies against focused fixtures."""
import pathlib
import re
import os
import shlex
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
FUNCTIONS = {
    "load.c": ["AudioLoad_IsFontLoadComplete", "AudioLoad_IsSeqLoadComplete",
               "AudioLoad_SetFontLoadStatus", "AudioLoad_SetSeqLoadStatus",
               "AudioLoad_GetFontsForSequence", "AudioLoad_SyncInitSeqPlayerInternal"],
    "heap.c": ["AudioHeap_ReleaseNotesForFont", "AudioHeap_Alloc",
               "AudioHeap_SearchPermanentCache", "AudioHeap_AllocPermanent"],
    "playback.c": ["AudioPlayback_GetInstrumentInner"],
    "seqplayer.c": ["AudioScript_SequencePlayerSetupChannels", "AudioScript_SelectChannelFont"],
    "../sequence.c": ["AudioSeq_ResolveSequence", "AudioSeq_StartSequence", "AudioSeq_QueueSeqCmd"],
}


def function_body(source, name):
    match = re.search(r"^(?:static\s+)?[\w* ]+\b" + re.escape(name) + r"\([^;]*?\)\s*\{", source, re.M)
    if match is None:
        raise RuntimeError(f"cannot locate production function {name}")
    depth = 0
    for pos in range(source.find("{", match.start()), len(source)):
        if source[pos] == "{":
            depth += 1
        elif source[pos] == "}":
            depth -= 1
            if depth == 0:
                return match.start(), pos + 1
    raise RuntimeError(f"unterminated production function {name}")


def main(build, compiler="cc"):
    build = pathlib.Path(build).resolve()
    build.mkdir(parents=True, exist_ok=True)
    bodies = []
    for filename, names in FUNCTIONS.items():
        path = (ROOT / "mm/src/audio/lib" / filename).resolve()
        source = path.read_text()
        for name in names:
            start, end = function_body(source, name)
            line = source.count("\n", 0, start) + 1
            bodies.append(f'#line {line} "{path}"\n{source[start:end]}\n')
    (build / "mm_audio_runtime_functions.inc").write_text("\n".join(bodies))
    common = [compiler, "-std=c11", "-g", "-DNDEBUG", "-Wall", "-Werror",
              "-Wno-unused-variable", "-Wno-unused-but-set-variable",
              *shlex.split(os.environ.get("MM_AUDIO_TEST_CFLAGS", "")),
              "-I" + str(ROOT / "mm/tests"), "-I" + str(ROOT / "mm/include"),
              "-I" + str(ROOT / "libultraship/include")]
    storage_exe = build / "mm_audio_font_id_test"
    subprocess.run([*common, str(ROOT / "mm/tests/audio_font_id_test.c"), "-o", str(storage_exe)], check=True)
    subprocess.run([str(storage_exe)], check=True)
    exe = build / "mm_audio_stream_runtime_test"
    subprocess.run([*common, "-I" + str(build),
                    str(ROOT / "mm/tests/audio_stream_runtime_test.c"), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main(*sys.argv[1:])
