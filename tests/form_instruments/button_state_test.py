#!/usr/bin/env python3
"""Exercise current engine note identity before display-staff publication.

The production accessors and pitch mapper run against their actual enum/table
definitions. Only the surrounding audio engine's storage is fixture-owned.
"""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/goron_audio"))
from run_tests import function


def main():
    audio = (ROOT / "soh/src/code/code_800EC960.c").read_text()
    header = (ROOT / "soh/include/z64audio.h").read_text()
    enums = "\n".join(re.search(
        r"typedef enum " + name + r" \{.*?\} " + name + ";", header, re.S
    ).group(0) for name in ("OcarinaButtonIndex", "OcarinaPitch", "OcarinaInstrumentId"))
    bank_end = header.index("} SoundBankTypes;") + len("} SoundBankTypes;")
    enums += "\n" + header[header.rfind("typedef enum", 0, bank_end):bank_end]
    table = re.search(r"u8 sPitchToButtonMap\[16\] = \{.*?\};", audio, re.S).group(0)
    bank = (ROOT / "soh/src/code/code_800F7260.c").read_text()
    functions = function(bank, "Audio_SetSfxBanksMute") + "\n" + "\n".join(function(audio, name) for name in (
        "AudioOcarina_MapNoteToButton", "AudioOcarina_GetCurrentButtonIndex",
        "AudioOcarina_GetPlaybackButtonIndex", "AudioOcarina_IsInstrumentSfxMuted",
        "AudioOcarina_GetInstrumentId", "AudioOcarina_SetInstrument"))
    fixture = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
typedef uint8_t u8;
typedef uint16_t u16;
/* ENUMS */
/* TABLE */
#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
#define SEQ_PLAYER_SFX 2
#define SFX_CHANNEL_OCARINA 13
#define NA_SE_OC_OCARINA 0x5800
static void* gSoundBanks[7];
static u8 gSoundBankMuted[7];
static u8 sCurOcarinaButtonIndex;
static u8 sCurOcarinaPitch;
static u8 sPlaybackPitch;
static u8 sPlaybackState;
static u8 sOcarinaInstrumentId;
static u16 sOcarinaInputButtonCur, sOcarinaInputButtonPrev, sOcarinaInputButtonPress;
static u16 sOcarinaInputButtonStart, sPlaybackStaffPos;
static bool sIsOcarinaInputEnabled;
static unsigned sOcarinaFlags;
static struct { u8 seqReplaced[4]; } gAudioContext;
static unsigned noteOffCalls;
static u16 AudioEditor_GetReplacementSeq(u16 id) { return id; }
static void Audio_SeqCmd8(u8 p, u8 a, u8 ch, u8 id) { (void)p; (void)a; (void)ch; (void)id; }
static void AudioOcarina_PlayControllerInput(bool read) { (void)read; }
static void Audio_StopSfxById(u16 id) { assert(id == NA_SE_OC_OCARINA); }
static void Audio_ClearBGMMute(u8 ch) { assert(ch == SFX_CHANNEL_OCARINA); }
static void Audio_QueueSeqCmdMute(u8 ch) { assert(ch == SFX_CHANNEL_OCARINA); }
static void AudioOcarina_ReadControllerInput(void) {}
static void __attribute__((unused)) GameInteractor_ExecuteOnOcarinaPlaybackNote(u8 pitch, float bend) {
    assert(pitch == OCARINA_PITCH_NONE && bend == 1.0f);
    ++noteOffCalls;
}
/* FUNCTIONS */
int main(void) {
    unsigned checks = 0;
    // Live hooks precede staff publication. Read the raw button independently
    // from semitone changes: R/Z may alter pitch without changing drum/pipe.
    for (unsigned raw = 0; raw < 256; ++raw) {
        sCurOcarinaButtonIndex = raw;
        sCurOcarinaPitch = OCARINA_PITCH_A4;
        const u8 button = raw & 0x3F;
        const u8 expected = button < 5 ? button : OCARINA_BTN_INVALID;
        assert(AudioOcarina_GetCurrentButtonIndex() == expected);
        assert(sCurOcarinaButtonIndex == raw);
        ++checks;
        sCurOcarinaPitch = OCARINA_PITCH_NONE;
        assert(AudioOcarina_GetCurrentButtonIndex() == OCARINA_BTN_INVALID);
        ++checks;
    }
    for (unsigned mask = 0; mask < 128; ++mask) {
        Audio_SetSfxBanksMute(mask);
        assert(AudioOcarina_IsInstrumentSfxMuted() == ((mask & 0x20) != 0));
        ++checks;
    }
    for (unsigned instrument = 0; instrument < 256; ++instrument) {
        sOcarinaInstrumentId = instrument;
        assert(AudioOcarina_GetInstrumentId() == instrument);
        assert(sOcarinaInstrumentId == instrument);
        ++checks;
    }
    // OFF must explicitly cancel direct replay, including an already-off host
    // whose form has been reset. Other instrument selections are not a cancel.
    sOcarinaInstrumentId = OCARINA_INSTRUMENT_DEFAULT;
    sPlaybackState = 1;
    AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
    assert(noteOffCalls == 1 && sPlaybackState == 0);
    AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
    assert(noteOffCalls == 2);
    AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_DEFAULT);
    assert(noteOffCalls == 2);
    checks += 3;
    // Independently specified pitch-to-button expectations. The sounding
    // B-flat resolves to the actual originating button through its high bits.
    const u8 expectedButtons[16] = {0,0,0,0,1,1,1,2,2,2,3,3,3,4,4,4};
    for (unsigned raw = 0; raw < 256; ++raw) {
        sPlaybackPitch = raw;
        sPlaybackState = 1;
        const unsigned pitch = raw & 0x3F;
        u8 expected = pitch < 16 ? expectedButtons[pitch] : OCARINA_BTN_INVALID;
        if (pitch == 10 && (raw & 0x80)) expected = 2;
        assert(AudioOcarina_GetPlaybackButtonIndex() == expected);
        assert(sPlaybackPitch == raw && sPlaybackState == 1);
        ++checks;
        sPlaybackState = 0;
        assert(AudioOcarina_GetPlaybackButtonIndex() == OCARINA_BTN_INVALID);
        ++checks;
    }
    printf("PASS current native instrument button identity: %u cases\n", checks);
    return 0;
}
'''
    fixture = fixture.replace("/* ENUMS */", enums).replace("/* TABLE */", table)
    fixture = fixture.replace("/* FUNCTIONS */", functions)
    if "--cancel-negative-control" in sys.argv:
        fixture = fixture.replace(
            "GameInteractor_ExecuteOnOcarinaPlaybackNote(OCARINA_PITCH_NONE, 1.0f);",
            "/* Negative control: native OFF omits the direct instrument cancel. */")
    with tempfile.TemporaryDirectory(prefix="form-instrument-buttons-") as temporary:
        directory = Path(temporary)
        source = directory / "buttons.c"
        source.write_text(fixture)
        for combined in (False, True):
            output = directory / ("combo" if combined else "solo")
            flags = ["-DCOMBO_BUILD", "-DSOH_BUILD_DLL"] if combined else []
            subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra", "-Werror",
                            "-O1", "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                            "-fno-pie", "-no-pie", *flags, str(source), "-o", str(output)], check=True)
            subprocess.run([str(output)], check=True,
                           env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


if __name__ == "__main__":
    main()
