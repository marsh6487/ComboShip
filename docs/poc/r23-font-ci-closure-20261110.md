# Font inclusion and cumulative CI repairs — November 10, 2026

This follow-up preserves the complete authorized cumulative candidate on draft
PR #89, including the authored assets, Solo MM entrances, Rito, Clock, spring
blossoms, shared Magic Mirror and earlier shuffle-parity repairs. The narrow
2Ship UI font draft is included. Winter water/ice behavior and snow walking
sounds on eligible outdoor floors were already implemented in the previous
public candidate and remain unchanged.

## Font source

The reviewed font donor is `961286e9c9046ecfec49922010963a0562479175`, with tree
`bfd0999b6aff67b8cb051aa08b775720787d59b5`. The published equivalent is
`7cab35bd6eab27c5d3b76e76037c3ff19ea2f73d`, independently read back with that
exact tree. It changes three MM menu selections to the existing standard
Montserrat font. The bundled font files, 24-pixel headings, scaling, icons,
font-stack guards and supported font-pack overrides are preserved. It does
not change in-game text.

Both affected native translation units, four font-override configurations,
clang-format 14 and independent review passed. See
[the font report](mm-ui-font-parity-20261109.md) for scope and the remaining
configured visual checks.

## Why the fixture repairs are required

The cumulative integration adds actual Solo MM serialization and Rito
preference dependencies. Several isolated regression fixtures extracted the
surrounding production functions without bringing those dependencies into
their compilation units. These repairs include the real headers and link the
real `SoloEntrances.cpp`, using typed native storage where a headless fixture
must provide application globals. They retain production serialization,
validation and routing code, existing assertions, seed matrices and solver
budgets. The shared-preference fixture also checks independent Rito and Mirror
snapshot restoration on both successful and failed saves.

The Goron and editor extractors retain explicit fail-fast boundaries for
unrelated live services. The editor grant fixture does not establish live Rito
form/button removal behavior. Existing route fixtures that use ownership
stubs do not establish that behavior either.

There are three narrowly scoped production compilation corrections. The
direct `libultraship/bridge/consolevariablebridge.h` include in
`mm/2s2h/Rando/DrawItem.cpp` declares `CVarGetInteger` for the authored Clock
draw function without relying on a Combo-only transitive include. Windows
MSVC also identified two rejected implicit conversions in the Rito code:
`custom_forms.cpp` now explicitly converts the unchanged bounded mask-wheel
arithmetic to `RandomizerInf`, and `rito_bow.inc.c` uses `const void*` for a
local animation pointer used only in identity comparisons, matching the
actual `SkelAnime::animation` storage. The calculations, comparisons and
control flow are unchanged. Reversing those exact substitutions and removing
the single include reproduce the three previous source files byte-for-byte.
Required native assets are unchanged.

## Local verification scope

The recovery receipt contains commands, source hashes, exit codes and retained
failure evidence. These results distinguish complete registered rows from
individual affected commands; partial results do not imply the complete
platform CI gate passed.

| Area | Verified scope |
| --- | --- |
| Solo games/settings | Complete registered row: three compiled probes and 20 production-boundary suites |
| Entrance and portal save | Complete seven-command row, with the existing applicable sanitizers |
| Shared items/preferences | Complete six-command row |
| Goron audio/lifecycle | All six row commands across preserved prefix/suffix runs; all 28 original cases retained |
| World messages | All 24 row commands across preserved prefix/suffix runs; final forest command has 44 passes and 15 existing optional skips |
| Key receipts | Complete sanitized row |
| Grace/Seasons/editor pools | All three complete rows, including editor sanitizers and native syntax checks |
| Grace settings | Complete registered row; optional historical-object mode was not available locally |
| Boss keys | All five row commands across preserved seed-matrix prefix and repaired oracle/syntax tail |
| Souls and door-key saves | Affected save commands only, in original and Combo modes |
| Grace and Seasons saves | Both complete registered rows, in Combo mode as configured by their runners |
| Magic Mirror save | Command 4 of 5 only, in Combo mode as configured by its runner |
| Shared native codec consumers | Complete lottery row; affected environment, vanilla-mode and two capacity commands, each in original and Combo modes |
| CI configuration | Existing 35 policy tests passed |

The song fixture passes MM original/Combo controls and OoT's existing 200 by
18 matrix in both modes. All 90 affected native syntax cases pass: 45 complete
production units in original and Combo configurations. Coverage combines
retained successful prefixes with the repaired Clock pair and remaining
cases; the repository runner and compiler flags are unchanged. Sparse-checkout
missing-header interruptions are retained as environment failures, with the
missing tracked headers restored before the remaining cases continued.

Both affected Rito production translation units pass native syntax checks in
original and Combo modes. The exact ownership and animation commands (2 and
3 of the ten-command Rito row) also pass. The ownership fixture now imports
the actual randomizer enum and retains its narrow Weird Egg through Mask of
Truth range guard. Its two flag-assignment indices use explicit integer
conversions required by the existing strict compiler flags; reversing only
those two conversions reproduces its previous `main()` bytes. Every assertion
and control remains unchanged. The other eight row commands and full MSVC
build still require final-head CI.

The targeted native save audit passes in original and Combo modes against the
preserved accepted report, whose SHA-256 is
`244ef3775f03b75afebc62ed7567e67ca13a035d7a423be277b996bf6cafd1fe`.
Its baseline is commit `e360328d03d6d30e38151035a2d5a8d92994d975`, retaining
94 option, 2,652 check and 151 flag entries. The diagnostic now links the
actual Solo codec when that source exists. Both generated audit `main()`
bodies and all comparisons are unchanged, and its two parser unit tests pass.
This is the targeted serializer proof, not a rerun of the entire historical
native-ID audit.

## Platform and native CI

The published font head's Build Artifacts run is
https://github.com/marsh6487/ComboShip/actions/runs/42740519091. Its Linux job
compiled and linked libultraship, both games, the randomizer CLI, shared UI and
launcher; generated both resource archives; passed seed-generation and
cross-game door smoke checks; uploaded its runtime artifact; passed the
actual native first-drop graph/fill/replay and both-solo/combined parity
checks; and built and uploaded the AppImage. The complete Linux job passed.
Windows
failed at the full SoH build with the two Rito conversion diagnostics described
above. Full raw-log retrieval failed, but the authenticated job annotations
provided both exact diagnostics, C2664 and C2440. The final repair-head results
belong in PR #89 and the publication receipt. The separate Windows compile
preflight run passed, but it does not substitute for the full platform build.

The Linux workflow now also runs the existing native Solo MM entrance
generation/replay checker against the staged linked runtime for the `off` and
`mixed-decoupled` profiles at seed `20261110`. It retains evidence even on
failure and excludes copied runtime binaries from the diagnostic artifact.
This new step's result must come from the final candidate CI run.

An earlier local application build was interrupted before completion. It is
not counted as a successful full link. The first published font-head
regression groups stopped on fixture omissions addressed here; final-head CI
must independently confirm the complete registered groups.

## Publication and remaining runtime checks

Publication uses text tree entries over the freshly reread font parent, reuses
all unchanged blobs, checks each simulated/resulting tree, and guards the
branch update with the expected parent and `force:false`. The publication
receipt records the actual remote commit and exact tree. Required native
animation and material resources remain present. GIFs, videos, animated
previews, private assets and history bundles remain excluded. Recovery
packages represent removed previews as deletions without embedding their old
bytes.

Configured gameplay, graphics and audio acceptance remain distinct from
compilation and headless verification. Remaining checks include font layout
and icons at larger scales, delegated MM menus and game transitions, authored
GI/material presentation, live Rito form/ownership transitions, seasonal
water/ice and outdoor snow sounds, and configured Solo MM entrance/save/replay
play. PR #89 remains a draft; no merge or master promotion is part of this work.
