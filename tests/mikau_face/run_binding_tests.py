#!/usr/bin/env python3
"""Compile the actual face-binding block in a small GBI-output harness.

This checks the pointers emitted by the source fragment, not a second
implementation of its selection logic. It does not compile ComboShip, load
archives, run the renderer, or prove the in-game appearance.

The regression caught here is returning native raw pixels for Mikau instead of
stable, aligned resource paths for the selected eye and mouth states. The same
harness also rejects changes to other actors' existing raw-pointer bindings.
"""

import argparse
import hashlib
import json
import os
import shlex
from pathlib import Path
import subprocess
import tempfile


PREFIX = r'''
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define ALIGN_ASSET(n) __attribute__((aligned(n)))
enum { STATIC_STORY_ACTOR_MIKAU = 26, TEST_OTHER_ACTOR = 22 };
typedef struct { struct { uintptr_t w0, w1; } words; } Gfx;
typedef struct {
    struct { int type; void* mmEyeTextures[8]; void* mmMouthTextures[4]; } staticState;
} EnViewer;
typedef struct {
    unsigned char eyeCount, mouthCount, eyeSegment, mouthSegment;
} StaticStoryMmPresentation;
typedef struct { uint8_t eye, mouth; } StaticStoryMmFace;

/* Boundary shim: record the F3DEX2 segment command submitted by the actual
 * binding block. Resource lookup and rendering are intentionally outside this
 * harness. The real gSPSegment wrapper preserves texture paths. */
static void gSPSegment(Gfx* packet, int segment, uintptr_t target) {
    packet->words.w0 = UINT32_C(0xDB060000) | (uint32_t)(segment * 4);
    packet->words.w1 = target;
}

static size_t RunBinding(EnViewer* this, const StaticStoryMmPresentation* presentation,
                         StaticStoryMmFace face, Gfx* commands) {
    Gfx* POLY_OPA_DISP = commands;
'''

SUFFIX = r'''
    return (size_t)(POLY_OPA_DISP - commands);
}

/* Literal expectations are the five independently inspected Mikau resources.
 * They are not generated from the source under test. */
static const char* const expectedEyes[] = {
    "__OTR__objects/object_zog/object_zog_Tex_024750",
    "__OTR__objects/object_zog/object_zog_Tex_024F50",
    "__OTR__objects/object_zog/object_zog_Tex_025750",
};
static const char* const expectedMouths[] = {
    "__OTR__objects/object_zog/object_zog_Tex_025F50",
    "__OTR__objects/object_zog/object_zog_Tex_026750",
};
static _Alignas(2) unsigned char rawEyes[8][32];
static _Alignas(2) unsigned char rawMouths[4][32];
static uintptr_t firstEyes[3], firstMouths[2];
static unsigned checks, failures;

static void Check(int condition, const char* message) {
    ++checks;
    if (!condition) {
        ++failures;
        fprintf(stderr, "FAIL: %s\n", message);
    }
}

static EnViewer MakeActor(int type) {
    EnViewer actor = {0};
    actor.staticState.type = type;
    for (unsigned i = 0; i < 8; ++i)
        actor.staticState.mmEyeTextures[i] = rawEyes[i];
    for (unsigned i = 0; i < 4; ++i)
        actor.staticState.mmMouthTextures[i] = rawMouths[i];
    return actor;
}

static int IsExpectedPath(uintptr_t pointer, const char* expected) {
    if (!pointer || (pointer & 1))
        return 0;
    /* The native test buffers have at least seven readable bytes. Check the
     * signature before comparing a complete string, so baseline failure does
     * not require treating arbitrary pixel bytes as an unbounded C string. */
    return memcmp((const void*)pointer, "__OTR__", 7) == 0 &&
           strcmp((const char*)pointer, expected) == 0;
}

static void ClobberStack(void) {
    volatile unsigned char scratch[4096];
    for (unsigned i = 0; i < sizeof(scratch); ++i)
        scratch[i] = (unsigned char)(i ^ 0xA5);
}

static void CheckMikau(uint8_t eye, uint8_t mouth, unsigned expectedEye,
                       unsigned expectedMouth, int repeat) {
    EnViewer actor = MakeActor(STATIC_STORY_ACTOR_MIKAU);
    EnViewer before = actor;
    StaticStoryMmPresentation presentation = {3, 2, 8, 9};
    StaticStoryMmFace face = {eye, mouth};
    Gfx commands[4] = {0};
    size_t count = RunBinding(&actor, &presentation, face, commands);
    Check(count == 2, "Mikau emits exactly two face segment commands");
    Check(commands[0].words.w0 == UINT32_C(0xDB060020), "Mikau eye binds segment 8");
    Check(commands[1].words.w0 == UINT32_C(0xDB060024), "Mikau mouth binds segment 9");
    Check(IsExpectedPath(commands[0].words.w1, expectedEyes[expectedEye]),
          "Mikau selected eye emits its aligned resource path, not raw native pixels");
    Check(IsExpectedPath(commands[1].words.w1, expectedMouths[expectedMouth]),
          "Mikau selected mouth emits its aligned resource path, not raw native pixels");
    Check(memcmp(&actor, &before, sizeof(actor)) == 0, "Mikau actor state remains unchanged");
    if (repeat) {
        Check(commands[0].words.w1 == firstEyes[expectedEye], "eye path address has static lifetime");
        Check(commands[1].words.w1 == firstMouths[expectedMouth], "mouth path address has static lifetime");
    } else {
        firstEyes[expectedEye] = commands[0].words.w1;
        firstMouths[expectedMouth] = commands[1].words.w1;
    }
}

static void CheckOther(uint8_t eye, uint8_t mouth, uint8_t eyes, uint8_t mouths) {
    EnViewer actor = MakeActor(TEST_OTHER_ACTOR);
    EnViewer before = actor;
    /* Lulu uses the opposite face segments; the generic contract must survive. */
    StaticStoryMmPresentation presentation = {eyes, mouths, 9, 8};
    StaticStoryMmFace face = {eye, mouth};
    Gfx commands[4] = {0};
    size_t count = RunBinding(&actor, &presentation, face, commands);
    size_t i = 0;
    Check(count == (size_t)((eyes != 0) + (mouths != 0)),
          "other actors retain their eye/mouth count gates");
    if (eyes) {
        Check(commands[i].words.w0 == UINT32_C(0xDB060024), "other actor keeps its eye segment");
        Check(commands[i++].words.w1 == (uintptr_t)rawEyes[eye], "other actor keeps its selected raw eye pointer");
    }
    if (mouths) {
        Check(commands[i].words.w0 == UINT32_C(0xDB060020), "other actor keeps its mouth segment");
        Check(commands[i].words.w1 == (uintptr_t)rawMouths[mouth], "other actor keeps its selected raw mouth pointer");
    }
    Check(memcmp(&actor, &before, sizeof(actor)) == 0, "other actor state remains unchanged");
}

int main(void) {
    for (uint8_t eye = 0; eye < 3; ++eye)
        for (uint8_t mouth = 0; mouth < 2; ++mouth) {
            CheckMikau(eye, mouth, eye, mouth, 0);
            CheckOther(eye, mouth, 3, 2);
        }
    CheckOther(1, 1, 3, 0);
    CheckOther(1, 1, 0, 2);
    CheckOther(1, 1, 0, 0);
    /* Stop here on the unpatched source: bounded fallback is not present yet,
     * and its unbounded raw-array indexing must not create an unrelated crash. */
    if (!failures) {
        ClobberStack();
        for (uint8_t eye = 0; eye < 3; ++eye)
            for (uint8_t mouth = 0; mouth < 2; ++mouth)
                CheckMikau(eye, mouth, eye, mouth, 1);
        CheckMikau(UINT8_MAX, UINT8_MAX, 0, 0, 1);
    }
    printf("%s: %u checks, %u failures\n", failures ? "FAIL" : "PASS", checks, failures);
    return failures ? 1 : 0;
}
'''


def extract_binding(source: str):
    function = source.index("static void EnViewer_DrawStaticMmActor(")
    anchor = "            face = StaticStoryMm_KafeiFace(this->staticState.mmAppearance);\n"
    begin = source.index(anchor, function) + len(anchor)
    end = source.index("        gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);", begin)
    return source[begin:end], source[:begin].count("\n") + 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    root = Path(__file__).resolve().parents[2]
    parser.add_argument("--source", type=Path,
                        default=root / "soh/src/overlays/actors/ovl_En_Viewer/z_en_viewer.c")
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    source_path = args.source.resolve()
    source_bytes = source_path.read_bytes()
    block, line = extract_binding(source_bytes.decode("utf-8"))
    harness = (PREFIX + f'\n#line {line} "{source_path.as_posix()}"\n' + block +
               '\n#line 1 "mikau_face_binding_tests"\n' + SUFFIX)
    with tempfile.TemporaryDirectory(prefix="mikau_binding_harness_") as temporary:
        harness_path = Path(temporary) / "binding_test.c"
        harness_path.write_text(harness)
        executable = Path(temporary) / "binding_test"
        command = [*shlex.split(os.environ.get("CC", "cc")), "-std=c11", "-O2", "-Wall",
                   "-Wextra", "-Werror", "-pedantic"]
        if args.sanitize:
            command += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                        "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
        command += [str(harness_path), "-o", str(executable)]
        build = subprocess.run(command, capture_output=True, text=True)
        environment = dict(os.environ)
        if args.sanitize:
            environment["ASAN_OPTIONS"] = "detect_leaks=0"
        result = (subprocess.run([str(executable)], capture_output=True, text=True, env=environment)
                  if build.returncode == 0 else None)
    report = {
        "scope": "Actual extracted C binding block with GBI-output boundary shim; no engine resource lookup or rendering",
        "source_sha256": hashlib.sha256(source_bytes).hexdigest(),
        "binding_start_line": line,
        "binding_sha256": hashlib.sha256(block.encode()).hexdigest(),
        "sanitizers": args.sanitize,
        "compile_returncode": build.returncode,
        "compile_stdout": build.stdout,
        "compile_stderr": build.stderr,
        "test_returncode": result.returncode if result else None,
        "test_stdout": result.stdout if result else "",
        "test_stderr": result.stderr if result else "",
    }
    print(json.dumps(report, indent=2))
    return build.returncode if build.returncode else result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
