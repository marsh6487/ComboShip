# WIP Combo Vanilla paired-save foundation

Complete upstream parent for this feature checkpoint:
`3796288605989833335193fbb2a85d1603656692` (the cumulative icon/bugfix candidate).
The feature is incomplete: creation UI, MM opening/first-cycle entry, restricted
NEI sharing, the fixed reward manifest/adapters and What's New remain pending.
Do not treat this checkpoint as a playable or accepted Combo Vanilla build.

## Implemented source milestone

The container records explicit `combo.mode = "vanilla-nei"` and placement
version 1. Legacy seeded containers remain Randomizer; missing seeds, mixed
metadata and unsupported versions do not become Vanilla. Native load paths query
the selected file before dispatching load hooks. Read-only mode peeks are
separate from activation so a save worker does not switch the foreground mode.

OoT's new-file writer stages its native section in memory. A separate MM export
calls native `Sram_InitNewSave`, transfers the font-code name and stages a native
Vanilla section, preserving the opening's first-cycle, form, magic and quest
state. The previous disk container is retained until a complete pair validates
and a flushed temporary file replaces it atomically. Failure restores OoT's
native state/metadata and keeps both PAL/NES name-entry paths out of their
success animation. Native MM is explicitly invalidated before aborting staging,
so later dormant writers see the ordinary no-save sentinel.

Wire identities were checked against `soh/soh/util.h`'s `FileType` enum and
`mm/include/z64save.h`'s `SaveType`: both native Vanilla values are zero.

## Source/native evidence

Fresh ASan/UBSan fixtures passed for:

- Real neutral mode/staging policy and actual filesystem replacement, including
  malformed metadata, incomplete native sections, bad slot IDs, failed writes,
  missing halves, abort and commit-once behavior.
- Extracted actual launcher read/write/create callbacks together with the real
  MM creator wrapper: name transfer, unmodified native opening state, no old-MM
  leakage into a new pair, failed initializer, initializer exception and failed
  final replacement. Every rejected path retains disk/cache contents and marks
  native MM as no-save.
- Actual OoT SRAM creation and PAL/NES post-creation branches: Vanilla is
  independent of a loaded seed, failures restore state and do not announce a
  created file, while seeded and solo controls retain their prior paths.
- Actual OoT threaded serializer with a fresh document and a prior seeded
  document: combined Vanilla removes obsolete randomizer/speedrun sections,
  retaining unrelated mod sections and existing seeded/solo serialization.

ROM initialization, game SaveContext serialization, native section handlers and
GUI services are explicit fixture boundaries. These tests do not compile/link
the whole game or prove configured runtime. Full game builds are pending for
this feature branch. clang-format 14.0.6 and whitespace checks cover changed
production sources. New CI gates append to the inherited registrations.

## Review and recovery

Read-only review identified two important stale-MM-write paths after failed
creation/rejected identity, and a minor old-randomizer-section warning on Vanilla
creation. The follow-up changes use the existing native no-save sentinel and
strip incompatible serialized sections, with reproducing tests. Review of
those corrections is recorded separately in the execution ledger.

Local source-import commits are recovery-only. The remote tree overlays real
feature/test/document deltas on the complete parent; it must not contain this
inspected subset's root history. Accepted scene archives and all earlier
crash/cursor/hint/icon changes remain preserved.

## Recovered publication and CI follow-up

The foundation is published at `517d408f5b7850f6ab0c7f72e28ea6539335a778`
in draft PR #79. Its first CI run exposed stale extracted fixtures rather than
a new native save failure: the solo OoT creation fixture lacked the new mode
callback declarations and named save/metadata types. The updated fixture uses
the actual mode header and retains all 30 existing solo/combined checks. The
cumulative soul receipt expectation correction and quest-song investigation
will be integrated from PR #78 while retaining both CI manifest additions.

The corrected creation fixture passes all 30 existing checks locally. Independent
review found no weakened checks or consequential issues. The broader local solo
runner is blocked by missing SDL_net headers; complete CI remains enabled. The
source merge retains the paired-save gate and the new quest-song gate.
