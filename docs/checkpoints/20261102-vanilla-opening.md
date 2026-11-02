# Combo Vanilla creation and native MM opening checkpoint

Baseline: draft PR #79, `a00d35fa1aaa78e4a3fd3c4f746d8795408b5ba8`, complete
upstream tree `e0443e6a91bea1d3e8b50b7558ba9e5e58259aaf`. This follows the paired-save
foundation and retains the inherited icon and quest-song work. Accepted archives
are unchanged. Local recovery-import commits must not be published as source history.

## Source behavior

The creation-only choice is available as `Vanilla + NEI` or `Randomizer` in native
file select and the Combo settings panel. Vanilla starts without generating or
loading a seed; Randomizer still requires one. A selection affects new files only.
Existing files read their persisted mode; previewing another slot does not change
the active mode. Both name-entry failure paths retain their prior rollback behavior.

Cold and warm MM entries both use Setup_InitImpl. A Vanilla file before its first
native reset follows the native intro, Deku-cycle and owl-save rules instead of
being sent directly to a Combo portal. The forest fall keeps the native curse and
tutorial. The MM loader accepts a Vanilla native half only for an explicit Vanilla
container and rejects mismatched halves. An unusable Vanilla half is neither rebuilt
from a stale seed nor allowed to enter gameplay on empty state.

Direct incoming shared grants and pending snapshot pulls wait until
`threeDayResetCount > 0`. The pending pull survives the waiting frames. This is
first-cycle protection, not completion of the NEI-only sharing policy: the Task 3
allowlist, reward transactions and Task 4 fixed placements remain unfinished.
The legacy two-process warp pipeline remains unchanged; COMBO_BUILD disables it.

## Evidence and limits

Five local ASan/UBSan suites cover the existing mode/atomic staging, host/native
creation, native OoT SRAM and both name-entry paths, serialization, plus the new
opening/creation behavior. The new fixture executes actual menu input and combined
MM setup/load functions across intro/Deku/owl/reset/resume/seeded controls, actual
normalization and quarantine handlers, and both DLL preview/activation shims.
Graphics, native flash I/O, DLL resolution, actual event timing and full ROM game
initializers are explicit fixture boundaries. Leak detection is disabled for this
host; address and undefined-behavior sanitizers remain enabled.

Independent review found the inherited rando-only load check and unconditional
curse/tutorial skip; each was reproduced before repair. A redundant legacy path
experiment was removed after review caught a standalone declaration problem.
The reviewer reran the opening fixture and reported no remaining Critical or
Important finding within Task 2. clang-format 14.0.6 and whitespace checks pass.

The prior a00d35fa CI completed Windows/Linux engine builds successfully. Its render
and entrance suites failed on two extracted fixtures missing the new mode-query
boundary. This continuation supplies explicit seeded-mode callbacks without
changing their assertions; opening-route fixtures include the new native helper.
The new complete-tree CI results must be recorded separately.

## Status and next work

Implemented/native-verified Task 2 candidate, not a complete playable Combo Vanilla
release, runtime acceptance or master promotion. Tasks 3–5 remain pending:
NEI-only sharing and foreign aliases, permanent receipt/check transactions, fixed
reward adapters/placements, final What's New and configured runtime proof.
The next decisive runtime check is fresh paired creation followed by first MM
entry, native intro/curse/Deku cycle, first Song of Time reset and later travel.
