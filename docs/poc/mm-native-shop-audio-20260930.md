# Native MM shop audio candidate

| Field | Record |
| --- | --- |
| Baseline | `392b4797a775d2d08f9e26238584b1e08df4d0a2`, the current dungeon-reward candidate on the progressive-fire / cap / Time Gate stack. These are retained candidates, not newly claimed runtime acceptance. |
| Candidate | `poc/mm-native-shop-audio-20260930` |
| Report | Cor hears severe crackling in 2Ship shops both while idle and while browsing/buying. Hold the separate SoH Shadow Scepter branch until this audio work is implemented and checked, then publish both. |
| Scope | Native MM audio production during rendering stalls; SSE2 native mixing in both ComboShip engines. |
| Preservation | Original sequences, instruments, samples, volumes, 32 kHz output, streamed PCM repair, private weather/Midna/SM64 cues, cap/Time Gate cleanup, game ownership and other cumulative features. No archive edits or platform-wide audio backend changes. |
| Verdict | Implemented and locally regression verified. Full builds and audible shop playback require separate evidence; no master promotion. |

MM's worker previously waited indefinitely for a graphics-frame notification.
A slow frame could drain the device queue, interrupting all native music and
sound effects. OoT already has an independent refill timeout and larger
reservoir. MM now waits at most 5 ms after its first ready frame and refills
within the backend's desired queue size. Its requested reservoir matches
OoT's 4096 stereo frames (128 ms at 32 kHz; previously 1680 / 52.5 ms).
This can add buffering latency; it does not change pitch, volume or sample rate.

The worker retains its own mutex for all production and request state. The
graphics thread publishes a bounded update divisor under that mutex. Refills
are bounded, remain unprimed until the first ready frame, and release the
graphics waiter even when the queue is full or shutdown occurs. A 528/544
sample cadence averages exactly 32000/60 samples per native audio update, so
independent refilling does not accelerate sequence tempo. The same final mix
and inactive-game mute ordering are preserved, including private voices.

Separately, both SSE2 mixers reused an already-unpacked low product vector
when constructing the high lanes. The candidate preserves the original words
until both halves have been unpacked. This is a low-bit arithmetic defect;
it is not asserted to explain the user's severe crackling by itself.

## Evidence

- The complete baseline MM worker fails the controlled device test after
  graphics wakes stop. The candidate passes repeated 75 ms device drains with
  no graphics wake at update divisors 1, 2 and 3; full-queue backpressure,
  fixed tempo, complete-output mute, mix-in ordering, pre-init shutdown and
  unprimed restart also pass. This exercises the actual production worker;
  the audio device and synthesis callbacks are controlled test boundaries.
- Both complete production mixers pass 132,529 cases each with zero sample
  differences against independent wide Q15 arithmetic and the scalar path.
  Coverage includes all 65,536 gains, zero/odd/native block sizes, saturation,
  unaligned addresses, in-place decay and repeated stereo reverb feedback.
  Before repair each engine produced 734,704 mismatched samples.
- The worker and both mixers pass ASan/UBSan. Local LeakSanitizer is disabled
  because this container cannot inspect its process threads; CI retains its
  normal sanitizer configuration.
- Existing streamed PCM parity passes for 32/44.1/48 kHz, including partial
  chunks; native audio translation units and runtime boundaries pass.
- Existing MM weather and Midna suites pass, including native headers and
  final mute/mix-in ordering. The weather fixture follows the relocated
  production submission block rather than replacing that logic.
- The uploaded native `mm.o2r` has SHA256
  `f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59`.
  Inspection confirms Shop_44 uses font 12 and KotakePotionShop_43 uses font 14.
  A separate arithmetic probe of 7,408,560 native ADPCM samples found no
  difference between the existing decoder and widened accumulation. No native
  sample data is changed or committed. This was not a full song/game playback.
- Baseline CI run `36693964242` failed in the dungeon-reward fixture because
  explicit `-isystem /usr/include` broke libstdc++'s `#include_next <stdlib.h>`.
  This candidate omits the redundant system-root argument, retaining custom
  include support. All 32 native pool configurations pass, as do the shared
  fill, delivery and plando fixtures. Dungeon-reward gameplay code is unchanged.

## Runtime check and recovery

Use the same build configuration, packs, Alt setting and shop that exhibited
the problem. Listen while idle, browse and buy an item, hear the fanfare, exit
and re-enter, pause/unpause, and switch OoT -> MM -> OoT -> MM. Check steady
music tempo, ordinary SFX and weather/Midna audio outside shops. The latest
supplied log does not establish the current shop's frame/audio timing or exact
pack state; audible resolution in that configuration remains unverified.

Keep the parent candidate/build as rollback. Only this branch receives the
audio changes. The accepted integration branch remains unchanged.
