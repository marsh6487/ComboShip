#!/usr/bin/env python3
"""Execute production cel command handlers, shader features and native GBI producers.

Only GPU submission/shadow-volume boundaries are replaced by observers. Handler
bodies, shader ID packing, feature decoding and command registrations come from
the selected engine source; packet producers use the engine's actual headers.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--sanitize", action="store_true")
parser.add_argument("--engine", type=Path, default=ROOT / "libultraship",
                    help="Engine tree to test without changing the worktree")
args = parser.parse_args()
engine = args.engine.resolve()
source = (engine / "src/fast/interpreter.cpp").read_text()
header = (engine / "include/fast/interpreter.h").read_text()


def block(text, pattern):
    match = re.search(pattern, text, re.M)
    if not match:
        raise RuntimeError("Missing production feature: " + pattern)
    opening = text.index("{", match.start())
    # Ignore comments and strings while balancing C++ compound statements.
    token = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', re.S)
    depth = 0
    for item in token.finditer(text, opening):
        if item.group() == "{":
            depth += 1
        elif item.group() == "}":
            depth -= 1
            if depth == 0:
                return text[match.start():item.end()]
    raise RuntimeError("Unbalanced production block: " + pattern)


shader = block(header, r"^enum\s*\{\s*SHADER_0") + ";\n"
shader += block(header, r"^enum class ShaderOpts\b") + ";\n"
shader += re.search(r"^#define SHADER_OPT\([^\n]+", header, re.M).group() + "\n"
shader += block(header, r"^struct CCFeatures\b") + ";\n"
shader += "namespace Fast {\n"
shader += re.search(r"^constexpr size_t SHADER_ID_SHIFT[^;]+;", header, re.M).group() + "\n"
shader += block(header, r"^constexpr uint64_t ShaderIdMask\b") + "\n"
shader += block(header, r"^constexpr int16_t ShaderIdUnmask\b") + "\n}\n"
shader += block(source, r"^void gfx_cc_get_features\b") + "\n"

handler_names = ("gfx_set_toon_handler_custom", "gfx_set_toon_key_handler_custom",
                 "gfx_set_toon_shadow_handler_custom", "gfx_set_stencil_handler_custom")
handlers = "\n".join(block(source, r"^bool " + name + r"\b") for name in handler_names)
# Keep the real command-to-handler associations, not a second hand-written map.
registrations = re.findall(
    r'\{\s*OTR_G_SET(?:TOON(?:KEY|SHADOW)?|STENCIL)\s*,\s*\{\s*"[^"\n]+"\s*,\s*\w+\s*\}\s*\}', source)
if len(registrations) != 4:
    raise RuntimeError(f"Expected four production cel registrations, found {len(registrations)}")

lus_gbi = (engine / "include/fast/lus_gbi.h").read_text()
opcodes = re.findall(r"^constexpr int8_t ((?:OTR|RDP)_G_\w+)\s*=\s*OPCODE\(0x[0-9a-fA-F]+\)", lus_gbi, re.M)

with tempfile.TemporaryDirectory(prefix="cel-interpreter-") as temporary:
    directory = Path(temporary)
    (directory / "production_shader.inc").write_text(shader)
    (directory / "production_handlers.inc").write_text(handlers)
    (directory / "production_registrations.inc").write_text(",\n".join(registrations))
    (directory / "production_opcodes.inc").write_text(
        ",\n".join('{"' + name + '", static_cast<uint8_t>(Fast::' + name + ')}' for name in opcodes))
    command = [os.environ.get("CXX", "c++"), "-std=c++20", "-DF3DEX_GBI_2",
               "-DGBI_FLOAT_MTX", "-DGBI_S32_VTX", "-Wall", "-Wextra", "-Werror",
               "-Wno-parentheses", "-I" + str(engine / "include"), "-I" + str(directory)]
    if args.sanitize:
        command += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                    "-fno-omit-frame-pointer", "-no-pie"]
    binary = directory / "native-test"
    subprocess.run([*command, str(ROOT / "tests/cel_interpreter/native_test.cpp"), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, timeout=20,
                   env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
