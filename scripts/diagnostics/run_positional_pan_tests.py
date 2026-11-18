"""Exercise both production channel-command handlers with native audio layouts."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

from run_mm_audio_runtime_test import function_body

ROOT = Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="positional-pan-") as directory:
    build = Path(directory)
    for game, path, function, command in (
        ("mm", "mm/src/audio/lib/thread.c", "AudioThread_ProcessChannelCmd", "AUDIOCMD_OP_CHANNEL_SET_PAN_WEIGHT"),
        ("oot", "soh/src/code/code_800E4FE0.c", "func_800E6300", "CHAN_UPD_PAN_UNSIGNED"),
    ):
        source = (ROOT / path).read_text()
        begin, end = function_body(source, function)
        preamble = '#include "global.h"\n#include <assert.h>\n#include <string.h>\n#include <stdio.h>\n'
        if game == "oot":
            preamble += source[source.index("typedef enum {"):source.index("} ChannelUpdateType;") + len("} ChannelUpdateType;")]
            includes = ("soh/include", "soh/src", "soh/assets", "soh")
        else:
            # Filter commands are outside the panning probe; never cross this boundary.
            preamble += '\nvoid AudioHeap_LoadFilter(s16* p, s32 lo, s32 hi) { (void)p; (void)lo; (void)hi; assert(0); }\n'
            includes = ("mm/include", "mm/include/PR", "mm/src", "mm/assets", "mm/2s2h", "mm")
        fixture = preamble + "\n" + source[begin:end] + r'''
int main(void) {
    // A point source at center may request pan=64 and weight=64. The weight
    // must change even though its payload happens to equal the current pan.
    for (unsigned pan = 0; pan < 128; ++pan) {
        for (unsigned oldWeight = 0; oldWeight <= 128; ++oldWeight) {
            for (unsigned weight = 0; weight <= 128; ++weight) {
                SequenceChannel channel = {0};
                channel.newPan = pan;
                channel.panChannelWeight = oldWeight;
                channel.volume = .625f;
                channel.volumeScale = .375f;
                channel.freqScale = .75f;
                SequenceChannel expected = channel;
                expected.panChannelWeight = weight;
                expected.changes.s.pan = oldWeight != weight;
                AudioCmd cmd = {0};
                cmd.op = COMMAND;
                cmd.asUbyte = weight;
                HANDLER(&channel, &cmd);
                assert(!memcmp(&channel, &expected, sizeof(channel)));
            }
        }
    }
    puts("PASS GAME native pan-weight commands: centered/equal-pan payload, every pan and weight, identity/no-op, unrelated audio attributes");
}
'''
        fixture = fixture.replace("COMMAND", command).replace("HANDLER", function).replace("GAME", game)
        src = build / (game + ".c")
        src.write_text(fixture)
        flags = ["-std=gnu11", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0", "-Wno-int-conversion",
                 "-Werror=implicit-function-declaration", "-ffunction-sections", "-fdata-sections",
                 "-g", "-fsanitize=address,undefined", "-fno-pie", "-no-pie"]
        if game == "oot":
            for cvars in ("CMake/soh-cvars.cmake", "CMake/lus-cvars.cmake"):
                for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / cvars).read_text()):
                    flags.append(f'-D{key}="{value}"')
        flags += ["-I" + str(ROOT / p) for p in (*includes, "libultraship/include")]
        binary = build / game
        subprocess.run([os.environ.get("CC", "cc"), *flags, str(src), "-Wl,--gc-sections", "-lm", "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env=dict(os.environ, ASAN_OPTIONS="detect_leaks=0"))
