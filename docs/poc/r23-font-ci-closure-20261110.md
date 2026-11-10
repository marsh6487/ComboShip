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

Three earlier production compilation corrections are retained. The
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

This report records source-verification evidence available before the follow-up
publication. PR #89 and its exact-head CI results record the subsequent full
platform outcome; the earlier published run identifiers below remain historical.

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
and control remains unchanged. At that earlier stage the other eight row
commands had not run locally. The later Equipment continuation passed the
complete ten-command Rito row in 32.130 seconds on `a227a506`. A fresh full
MSVC build and final-head CI remain required.

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

The earlier published font head `7cab35bd` (tree `bfd0999b`) was checked by
Build Artifacts run `42740519091`:
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
The later `49437303` / run `42746690900` result is recorded below: both native
Solo MM profiles passed. The next follow-up still requires its own fresh CI.

An earlier local application build was interrupted before completion. It is
not counted as a successful full link. The first published font-head
regression groups stopped on fixture omissions addressed here; final-head CI
must independently confirm the complete registered groups.

## Further fixture closure from the published candidate CI

Run `42746690900` checked published `4943730393f1179a51af2115436fe73c1c394a35`
at tree `53ddbe5763f92b72927549995e8088240ba8a96d`. Its fast source gate and
separate Windows compile preflight passed. All six regression groups then
stopped on isolated-fixture omissions. The follow-up restores the actual
dependencies in the affected tests and diagnostics. These fixture repairs
do not change production behavior, native assets or build configuration:

| Fixture | Repair and exact passing evidence |
| --- | --- |
| Receipt information | One actual Rito snapshot header. The registered command's CI prefix passed before the missing-header compile; the unchanged information/tracker generation, strict compiler flags and execution tail then passed locally. This is composed coverage, not an uninterrupted local rerun. The earlier layout-only/sanitizer invocation is separate. |
| Silver rupee cosmetics | Place the unchanged cache after the existing owner boundary and include the actual soul preference helper. Compatibility commands 5–8 passed locally, including the normal/sanitized silver cases; passing CI commands 1–4 are retained. |
| Mirror travel inventory | Extract each host's actual Rito ownership helper and provide the real combined-mode storage declaration. Unrelated Rito grant/removal boundaries fail if called. The complete registered row passes for both MM and OoT hosts under its existing `COMBO_BUILD` flags. |
| Foreign key cosmetics | Restore the equivalent recipe-slice boundary and actual cache preference/owner dependencies in the runner. Plain and sanitizer commands 1–2 of 4 pass. The shared C++ prefix used by the silver fixture is byte-identical; the two heart commands were not repeated locally. |
| Sword asset toggles | Include the actual preference helper and connect owner Alt state to the fixture's existing toggle. The complete sanitizer row passes, retaining all seven identities/progressive tiers, independent host/donor transitions, fallback, missing-resource retry and cache resets. |
| Shared and Mirror saves | Extract the three actual OoT Rito ownership/note-cell/migration helpers before the current save-load implementation. Both complete registered rows pass for MM and OoT hosts under their existing Combo flags. |
| Mirror editor | Include actual per-host Rito ownership and native mode storage, with fail-fast boundaries for unrelated grant/removal. Both registered editor/camera commands pass; the camera code is unchanged. |
| Foreign soul skeleton syntax | Update one extraction condition to retain the complete actual `freezeLastFrame && anim` block. Local commands 5–9 pass; unchanged CI commands 1–4 are retained. This is composed coverage of the nine-command row. |
| OoT editor | Two added runner lines extract the actual native Rito ownership helper. The complete registered row passes, retaining all nine wand/season combinations, repeated-grant controls and preserved selections. |
| Shared inventory UI | Extract each host's actual Rito ownership helper and fail immediately if these unrelated wheels grant or remove Rito. The complete MM/OoT row passes with every A-button, direction, full-width C/D-pad, Slate-title and wand-rule assertion and flag unchanged. |
| Mask transformation routing | Add the actual Rito ownership definition before the already extracted form-enabled function. The complete registered routing row passes; the paired C++ fixture and all previous assertions, modes and compiler flags are unchanged. |
| Time pedestal | Include the three actual native Rito ownership/note-cell/migration definitions before the extracted save/load functions. Both exact row commands pass, retaining age, progression, collision, camera, arrival/handoff, loadout and Pak-layer controls. |
| Native door keys and legacy layouts | Include the actual Solo MM traversal headers and Combo-only codec source. Move the existing game-mode include ahead of extracted legacy snapshots and include the actual Rito snapshot header. CI command 1, local command 2 in original/Combo modes with the existing sanitizers, and repaired commands 3–4 provide complete composed row evidence. Legacy 24/new 27 identities, Castle and shuffled returns, and both MM JSON save variants remain checked. |

Existing assertions, test controls, compiler/execution flags and extracted
production cache bytes are preserved. The receipt repair reproduces the
entire previous fixture when its one added include is removed. These local
results do not turn the failed published CI groups green; the follow-up must
receive its own complete registered-group and platform verification. PR #89
records the observed build results and latest published identity.

## Native Solo MM entrance CI evidence

On published `49437303` / exact tree `53ddbe57`, Linux run `42746690900`,
job `129601148198`, passed the actual native entrance generation/replay step
and its evidence upload. The `off` and `mixed-decoupled` profiles both exit
zero at input seed `20261110`, reach the native goal and visit all 284 shuffled
checks. Their replay reaches 2,823 checks and ignores the nine deliberately
stale combined-mode portals. The mixed case persists 124 entrance mappings
and rejects duplicate destinations, duplicate return destinations and a
foreign endpoint.

Evidence artifact `native-solo-mm-entrances-linux` (`13732847274`) is 159,411
bytes, SHA-256
`b11e988c34baa0323cb5a9a99c29e1d73e1f5419ccfe672001df35c26c80240d`.
Its downloaded bytes match the GitHub digest, every ZIP entry passes CRC and
readback, and its five text files contain no copied runtime directory. The
manifest records the staged native module hashes. Those hashes were produced
by the actual runner; the large runtime artifact was not independently
redownloaded and rehashed. A second deterministic fill was not requested.
This is native generation/save/replay proof for those exact two cases, separate
from configured player traversal, graphics and audio acceptance.

The same Linux job has passed all six native build targets, both resource
archives, the seed and door smoke tests, all three native first-drop Vanilla
cases, and all eight native parity profiles. The first-drop cases cover off,
on and on with exclusions; both on cases retain 43 source-soul negatives.
The parity matrix covers own/own-excluded, Stone Tower normal/inverted, rusty
keys, MM parity, all-off and all-parity, with actual solo and combined fills
and replays. The circular negative intentionally returns exit one, and the
overfull-budget rejection passes. All four recorded module hashes match
across the three native evidence artifacts; the first-drop and parity driver
hashes also match. The entire Linux job, including packaging, runtime upload and AppImage
upload, completed successfully. Windows passed the full SoH build but failed in 2Ship at
`SoloEntranceMmRuntime.inc.h(60,29)`: MSVC C2375,
`MM_SetSoloEntranceRuntimeSeed` redefinition with different linkage. The exact
diagnostic comes from the authenticated job annotations; decoded full-log
retrieval failed. Downstream Windows targets and packaging were skipped. All
six regression CI conclusions and the aggregate required gate in run
`42746690900` remain failed. The follow-up requires a fresh complete run. Local repaired rows and continuation results are recorded separately.

## Solo MM Windows export declaration

Windows job `129601147418` on published `49437303` passed SoH, then reported
C2375 for `MM_SetSoloEntranceRuntimeSeed` in 2Ship. Its implementation is
already explicitly exported; its shared header lacked the matching owner
export decoration. The header now includes the existing `ComboGameExport.h`
and applies `COMBO_MM_EXPORT` only to that declaration. This existing macro
exports only the Combo MM DLL owner; original builds and shared consumers
retain their prior declaration policy. The entire runtime implementation and
all other declarations are byte-identical. Reversing the one declaration
substitution and removing the one include reproduces the prior header exactly.
[Microsoft's DLL declaration guidance](https://learn.microsoft.com/en-us/cpp/cpp/definitions-and-declarations-cpp?view=msvc-170)
requires DLL-interface declarations to carry their matching decoration.

The existing native ABI guard now extracts this actual production signature
and includes the actual shared header. Its signature extractor accepts the
function-try-block and omits only the trailing `try` while generating its
isolated body. All 11 previous extracted signatures, assertions, owner/consumer
probes and compiler flags are unchanged. With the old header, the declaration
export guard fails specifically because this symbol is absent from its MM ELF
export set; the repaired header passes both owner probes and the consumer
probe. This control concerns the isolated declaration guard, not the previously
successful Linux runtime's explicitly exported definition.

Local C11 and C++20 native-MM checks with `MM_BUILD_DLL` and without
`COMBO_BUILD` pass with strict warnings. Preprocessing confirms the plain
original declaration and no imported implementation export macro. The header
passes clang-format 14. The existing Windows-before-configure ABI command will
exercise the new case with MSVC. The actual full Windows failure is not called
closed until fresh candidate CI passes.

## Completed local continuations

The complete World registry now has composed passing evidence for all 58 rows:
retained CI rows 0–17, complete repaired rows 18/21/50, and unchanged local
continuations 19–20, 22–49 and 51–57. The final seven rows passed in 802.549
seconds on local `59f227cc`, tree `4a70ba58`, including all 20 soul commands,
all 90 production syntax cases and the native collision-capacity/arena row.
The source/index and all 15 prepared untracked baseline files remained unchanged.

All 28 remaining Render rows, 57–84, passed on that same prepared source in
476.522 seconds. This includes the font, seasonal floor, soul/foreign-item
rendering, Sages packaging, authored key/Clock routes and spring texture/fade
coverage. Earlier failed rows retain their separately recorded repairs.
Entrances likewise has complete composed coverage through its final row.
These results preserve the actual CI prefixes and exact local suffixes; they
do not claim an uninterrupted fresh whole-suite run or configured rendering.

All 11 remaining Systems rows, 36–46, passed on prepared `59f227cc` in
829.777 seconds. The final row retained all four native controls, Vanilla
generation/import and tracker commands in original-MM and Combo modes. The
root source and all 15 prepared untracked baseline files remained unchanged.

## Final Items and Equipment fixture dependencies

The final continuation exposed nine more isolated fixture failures.
Eight repairs supply omitted native ownership/migration definitions, a declared
dependency include path and, where needed, the native game-mode declaration and
assertion-failing boundaries for unrelated Rito grant/removal. Their exact inverse patches reproduce every
previous runner byte. They retain the production helpers, all existing
assertions and native modes, and every compiler/execution flag already present.

| Fixture | Verified scope |
| --- | --- |
| Elemental sword save | Seven added runner lines bring in the actual OoT save storage, native macros and the Rito ownership/note-cell/migration helpers. All 16 commands of Items row 40 have composed passing evidence: retained command 1, then repaired command 2 and unchanged commands 3–16 in 538.831 seconds. Both save hosts retain sanitizers. |
| OoT pause inventory | Two added lines extract actual Rito ownership. The complete sanitized Equipment row 45 passes in original OoT and Combo modes. |
| Mirror shuffle grant | Five added lines supply actual per-host ownership and MM-only unrelated-service boundaries. Retained commands 1–2 and green commands 3–5 provide complete Items row 45 coverage; the final three pass in 19.335 seconds. |
| OoT shortcut selection | Two added lines extract actual Rito ownership. Four retained passing sub-runners plus the repaired selection and unchanged status runners cover all six of Equipment row 55; the final pair passes in 1.045 seconds. |
| MM shortcut controls | Six added lines supply actual ownership and game-mode storage with unrelated grant/removal guards. All seven sub-runners in both Combo and solo modes pass: 14 invocations in 12.485 seconds, retaining the existing sanitizers. |
| OoT cursor navigation | Two added ownership-extraction lines. The full sanitized row passes all six scenarios in each of original OoT and Combo modes, retaining geometry, empty-page, disabled-cell and navigation assertions. |
| MM Vanilla reward bindings | Six added lines supply actual ownership and game-mode storage with unrelated grant/removal guards. Retained commands 1–2 and green commands 3–4 cover Items row 50, including all 30 native reward bindings, prerequisites, atomic failure and single-debit controls. The final pair passes in 4.858 seconds, including unchanged Bomb Arrows/Twilight checks. |
| Native item-table bounds | Two added lines append the existing pinned `IMGUI_DIR` to the runner's explicit include list. The complete original and Combo commands pass in 197.132 seconds under unchanged ASan/UBSan flags, covering real Item/Text initialization, repeated initialization, stable key-ring IDs and reserved slots. The generated native C++ is byte-identical. |
| Rito imported GI classification | The old generic-fallback classifier incorrectly included the newly authored native Rito GI. Its 52 existing legacy cases and foreign sentinel are retained byte-for-byte. A separate positive fixture executes the complete actual native descriptor and dispatcher for Rito in all four Alt/effects combinations. |

The item-table runner is a gate-origin fixture. CI already supplies the pinned
ImGui directory, while the regression driver deliberately applies its auxiliary
fixture include path only to NEI-origin rows. The runner now consumes its
existing declared dependency explicitly; ambient include variables remain
cleared in verification. No global include-path override or native header
substitution is used.

The Rito classification repair verifies the local MM authored GI URI, scale,
Gold effect, mandatory shimmer, draw/state flags and preserved award name.
Donor lookup, alternate resource routing and native table boundaries fail
immediately if called. The full native descriptor, dispatcher and legacy
fallback bodies are byte-identical to the approved `e0b3f945` source; the
production implementation is unchanged. The actual dispatcher reaches the
native Rito descriptor before its legacy fallback, which correctly declines
Rito. Retained Items row 46 command 1 and passing commands 2–6 (17.602 seconds)
provide complete composed row coverage. This checks the approved presentation
route without claiming configured visual acceptance.

All 62 Equipment rows now have composed passing evidence. Retained CI and
repaired rows 0–13 combine with the exact 48-row local suffix, 14–61. That
suffix includes the complete ten-command Rito row, both original and Combo
control modes where registered, song/quest ownership, Wolf vertex offsets and
Cane geometry. The original registry, source/index and all 15 prepared file
hashes remained unchanged during the fullroot continuations. Each isolated
repair has a source checkpoint, exact source hash proof and independent review.

## Complete composed regression coverage

All 67 Items rows now have passing composed evidence. Retained CI rows 0–8
combine with the repaired receipt row, unchanged continuations and the exact
repaired rows described above. The final native key-ring, capacity/wallet and
boss-key rows 61–63 passed in 764.232 seconds. Row 64 passed both complete
native commands in 197.132 seconds after its two-line include closure. The
final soul-hint and MM first-drop-hint rows 65–66 passed unchanged in 228.668
seconds. The primary continuation 51–60 passed in 448.380 seconds, including
24 action-pool cases and 720 seeded fills with zero chest-count contract failures.
Every fullroot continuation preserved `a227a506`, its index and all 15 prepared
file hashes. The explicit coverage map retains each CI prefix, repaired row,
continuation and source checkpoint, including their original failure records.

| Registered group | Rows with composed passing evidence |
| --- | ---: |
| Items | 67 |
| World | 58 |
| Equipment | 62 |
| Render | 85 |
| Systems | 47 |
| Entrances | 12 |
| **Total** | **331** |

The registry and its commands remain unchanged. Coverage combines retained CI
prefixes, complete repaired rows and unchanged local continuations. The final
published follow-up still requires fresh complete group and platform CI; the
old `42746690900` failure conclusions remain part of the historical record.

## Historical comparison prerequisites

The existing CI workflow fetches fixed public historical commits before its
regression commands. The local Equipment continuation initially lacked
`c0181856968dd6f3869be0d1aa240743091fd6a8` (Gerudo) and
`a82852a3a02c8cabc541cce7c124da71c5755efb` (Great Fairy). Their exact required
commit/tree/blob objects were recovered and their Git identities recomputed;
unchanged `git show` lookups then returned the expected historical bytes.
All 271 Gerudo path lookups and the Great Fairy actor source read back
successfully. The Gerudo and Great Fairy rows then passed. This was local CI
preparation and required no source, asset, reference or comparison-baseline
change. Recovery was limited to required historical paths; it did not claim
restoration of every unrelated historical object.

## Publication and remaining runtime checks

Publication uses text tree entries over the freshly reread candidate parent, reuses
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
