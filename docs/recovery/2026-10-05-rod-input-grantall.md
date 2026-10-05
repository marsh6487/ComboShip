# MM Rod input and bulk season ownership candidate

Baseline: local integration `7c17a37ec58a8a39a5905f6621686b55e33fb364`.
The supplied `MM_NEI_failure_trace(1).zip` traced published PR #34 head
`1147a1a893ad2e2c0425049cc9b2ed618d944656`; its Rod input ordering remains
present in this local baseline. The accepted master is unchanged.

The bounded Rod fix uses MM's selected effective `Input` after native
suppression, overrides, and `OnPassPlayerInputs` hooks. Consuming a Rod edge
also consumes that same edge in native item dispatch, which previously read
the copied `ITEM_EXT_BUTTON` marker and stowed the freshly drawn Rod in the
same frame. Raw input consumption remains for other custom-item listeners.

The expanded `tests/seasons/run_rod_lifecycle_tests.py` executes the real
native input-selection block, C/D item accessors, button dispatcher, Rod
tick, and use/finish/init lifecycle against actual MM structs. Animation
scheduling, audio, and unrelated actor systems remain fixture boundaries.
Its first complete C-button frame failed on the baseline's post-dispatch
drawn-state assertion before the fix. Afterward, C-left and D-up retain the
Rod on the first press and open its HUD on the next edge. Unrelated native
item replacement, hook suppression, movement suppression, override input,
disabled C/D buttons, zero-health suppression, delayed draws through full
native dispatch, hand capture, and cancellation checks pass. This is a native
code fixture, not a game runtime proof.

The separate editor fix preserves a valid selected season across Grant All.
With no owned valid selection it chooses the newly owned Spring power.
Individual season rewards still select their granted season. No seasonal
weather or particle design is changed: Spring's existing native snow actor
is retained, and a deliberate Winter selection remains Winter. The new
editor assertion failed before this fix because a fresh Grant All selected
Winter. The actual editor grant, ownership, pool, and GI-binding suites pass
under AddressSanitizer/UBSan/bounds checks; native SaveEditor/GiveItem syntax
also passes.

Verified commands so far:

- `python3 -B tests/seasons/run_rod_lifecycle_tests.py`: PASS for complete
  C/D dispatch and native use/finish/init lifecycle.
- `python3 -B tests/seasons/run_rod_lifecycle_tests.py --sanitize`: same
  complete C/D frames PASS under AddressSanitizer/UBSan/bounds checks.
- `python3 -B tests/seasons/run_tests.py`: all 16 season cases PASS.
- `python3 -B tests/mm_editor/run_tests.py`: actual grant/pool/binding suites
  PASS with sanitizers; SaveEditor/GiveItem native syntax PASS.
- `python3 -B tests/mm_wolf/run_syntax_tests.py`: real native player unity
  translation unit and the three existing Wolf translation units PASS.
- `git diff --check`: PASS.

Recovery: the Rod integration changes only `Seasons_TickInput` and its MM
call site plus the corresponding native fixtures. The Grant All change is
isolated to the editor ownership helper and editor fixtures. Full game
runtime and the user's active custom asset configuration remain untested.
