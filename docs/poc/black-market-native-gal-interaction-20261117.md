# MM Black Market Treasure Gal interactions

Base: ComboShip `d3cf01741084c188eb0a01a263018fa3de23fbe1`.
Implemented source candidate; full executable/gameplay and visual/audio acceptance remain untested.

The private shop renders Treasure Gal on an EnOssan transaction controller.
Its adapter previously initialized only `ShopGalMMDLevelSwayAnim`; the native
EnTakaraya actions never ran, so her interaction choreography and message voices
were absent. Running EnTakaraya itself would also run its treasure minigame,
prices, wall spawning, timers and cycle flags, which do not belong in this shop.

The controller now changes only presentation using the native gal clips:
`00A280` enters the conversation, `001384` is the talking sway, `00AD98` raises
her from the lean, and `000968` is her asking gesture. Conversation/page/closing
and refusal events choose these clips; native animation update still runs exactly
once per keeper frame. The accepted custom level sway remains the resting pose.
There is no new animation data, skeleton edit, asset replacement or model pack.
The EnOssan controller continues to own item browsing, camera turns, payment,
receipt acknowledgement and customer placement.

Her native voices live in message `0x1E` SFX controls rather than actor-update
calls. The corresponding native MM messages supply the first cue for greetings,
keeper talk, refusal and unavailability; player-form message selection matches
EnTakaraya's tables. Cue parsing skips control arguments, checks message bounds,
and silently omits an unavailable cue. Existing EnOssan decide/cancel/error
sounds already match EnTakaraya and are retained without duplicate playback.
No native minigame BGM or timer cues are imported. This links original message
cues, but does not establish what a user's replacement audio pack sounds like.

Verification:

- `tests/black_market_keeper_interaction/run_tests.py`: ASan/UBSan pass.
  It compares the controller's gesture sequence against actual extracted native
  EnTakaraya Wait/Talk/SetupWait actions and original form/message tables. The
  C identifier `this` is renamed for C++ compilation; action logic is unchanged.
  A second executable compiles the actual production cue lookup against real MM
  message-table types and checks missing table/play/entry and malformed payloads.
- `tests/black_market/run_shop_camera_mm_tests.py`: ASan/UBSan pass for actual
  EnOssan/EnGirlA, CutsceneManager and ported OoT Data4 camera across all five
  forms/eight slots, purchase/receipt chains, cancellation, interruption and
  teardown. Animation resource/frame decoding is a declared service boundary.
- `git diff --check`: pass for owned paths.

The existing native C camera fixture needs GCC-compatible OTR-pointer warnings
relaxed (`-Wno-error=incompatible-pointer-types -Wno-error=int-conversion`) and
its extracted C++ camera data needs `-Wno-narrowing` on GCC 14. The run used
nlohmann/json v3.12.0 headers through `CPLUS_INCLUDE_PATH`.

Next gameplay check: greet/talk/help/page/leave, refuse an unaffordable item,
purchase and continue, then reenter with custom and native fallback keeper.
Inspect counter clearance, gesture timing and original voice cues. This source
checkpoint is not proof of configured rendering or audible runtime parity.
