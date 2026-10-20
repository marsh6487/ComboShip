#!/usr/bin/env python3
"""Run native MM message opening/decoding on the real custom Saria payloads.

The C units and engine layouts are production code. Graphics, audio, item-icon
loading and the hook registry are boundary doubles; no game or save is run.
"""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    end = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


custom = (ROOT / "mm/2s2h/CustomMessage/CustomMessage.cpp").read_text()
saria = (ROOT / "mm/2s2h/Enhancements/Gameplay/ComboSaria.cpp").read_text()
helpers = "\n".join(function(custom, signature) for signature in (
    "void CustomMessage::Replace(", "void CustomMessage::ReplaceColorChars(",
    "void CustomMessage::AddLineBreaks(", "void CustomMessage::EnsureMessageEnd(",
    "void CustomMessage::LoadCustomMessageIntoFont(",
))
hook = function(saria, "[](u16* textId, bool* fromTable)")
closure = function(saria, "    if (state.talking) {")
fixture = (ROOT / "tests/custom_messages/decoder_test.cpp").read_text()
fixture = fixture.replace("/* CUSTOM_MESSAGE_HELPERS */", helpers)
fixture = fixture.replace("/* SARIA_HOOK */", hook)
fixture = fixture.replace("/* SARIA_QUEUE */", function(saria, "void QueueSariaSong()"))
fixture = fixture.replace("/* SARIA_CLOSURE */", closure)
includes = [ROOT, *[ROOT / "mm" / p for p in ("", "include", "include/PR", "src", "assets", "mods", "2s2h")],
            *[ROOT / p for p in ("combo", "combo/menu", "libultraship/include", "libultraship")]]
flags = ["-g", "-O1", "-w", "-DCOMBO_BUILD", "-DMM_BUILD_DLL", "-DF3DEX_GBI_2",
         "-DCONTROLLERBUTTONS_T=uint32_t", "-DLOG_LEVEL_GAME_PRINTS=6",
         "-ffunction-sections", "-fdata-sections", "-fsanitize=address,undefined",
         "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie",
         *["-I" + str(path) for path in includes]]
with tempfile.TemporaryDirectory(prefix="mm-custom-message-") as directory:
    folder = Path(directory)
    units = []
    for name in ("z_message", "z_message_nes", "z_message_staff", "z_kanfont"):
        obj = folder / (name + ".o")
        subprocess.run([os.environ.get("CC", "cc"), "-std=gnu17", *flags, "-c",
                        str(ROOT / "mm/src/code" / (name + ".c")), "-o", str(obj)], check=True)
        units.append(str(obj))
    cpp = folder / "decoder.cpp"
    cpp.write_text(fixture)
    binary = folder / "decoder"
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", *flags, str(cpp),
                    *units, "-Wl,--gc-sections", "-no-pie", "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, timeout=10,
                   env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
