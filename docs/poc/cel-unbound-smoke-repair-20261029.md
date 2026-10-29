# October 29 combined candidate smoke-test repairs

## Baseline and preservation

Repository: marsh6487/ComboShip. Tested integration branch:
`integration/cel-unbound-collision-20261028`, remote source
`9581b89ddf2a12c2dd1f05279dff2758e2ae6634`, local equivalent
`c36e6422759458ec1005cee70f9fde9029a946ae`, tree
`e4b832ac57b160f0133c59e2f61ba41bcc6c4f00`.
PR69's Windows merge build `ffb0ecee59f4c31f539a49ba10cea589782726c7`
has that same tree. Original Windows artifact: run 40816468437,
artifact 12874148556. No accepted master or asset archive is changed.

cor reports that modified Kakariko loaded with torches and rain; a round bomb
killed a Skulltula and broke pots; its token was collectible but the dialogue
was empty. The log records scene 82, overlap queue 57/100, item 0x71 and text
0xB4. These are configuration-specific successes. Grotto opening, performance,
all-scene compatibility and overall stability are not established by that test.

Preserve all collision/actor/object/cache expansions, cel rendering, host
ComboShip/MM/NEI behavior, saves and existing authored assets. Repairs touch
only OoT message publication, its mesh diagnostic logger and regression wiring.

## Input evidence

- Fleet of Harkinian(5).log SHA256:
  `daacfe26ed98deaa6e55c7408d76528d19279f4c12bdb371a9f7be48d35b952f`
- Fleet of Harkinian-crash(3).dmp SHA256:
  `3f064abc5cd40ed7732805485a71e52bf3c18177ad37c732738f7d453fb4bbf5`
- Dump: exception 0xC0000005, read address zero, soh.dll RVA 0xD0B273,
  matching the log's 00:29:02 Link's House transition fault.
- Startup log fault: soh.dll RVA 0xB64833, ResourceLoader worker call chain.
- The original artifact contains no PDB or MAP. Analysis uses its matching
  soh.dll instructions and embedded strings, correlated with source. Export
  names such as SOH_RunGameLoop plus large offsets are not precise symbols.

## Root causes and bounded fixes

1. `MMAssets::EnsureMmArchiveLoaded` archive reorder clears all five published
   message tables and `_message_0xFFFC_nes`, then calls `OTRMessage_Init`.
   Unbound's initialized guard returned without republishing its process-owned
   tables. The log records a null 0xFFFC after reorder. The exact Windows fault
   at RVA 0xD0B273 is the byte scan in `Font_LoadOrderedFont`'s strlen of that
   pointer, followed by the expected font message-buffer copy and glyph loop.
   The same null table publication explains native token text 0xB4 being empty.
   Repeated initialization now republishes existing ownership without rebuilding
   storage or appending terminators. True Combo resume still rebuilds via
   `OTRMessage_ResetForResume`.
2. `LogMeshAsXML` read `polygon1.multi.list` for format 1. That union contains
   `single.unk_0C`, not an image-list pointer. Windows RVA 0xB64833 reads
   image->unk_00 after creating the BgImage XML node; embedded strings and the
   surrounding format comparison identify this source path. A zero unk_0C
   produces the observed null read; a nonzero value would also be invalid.
   The logger now reads only the single member for format 1, emits one image,
   and uses zero for fields absent from that layout. Multi-background ids,
   counts and fields remain intact. This preserves the earlier correct loader
   fix that stopped overwriting single-image data with a list pointer.

The latest dump's font-loading fault can explain re-entry after reset. The
provided log ends after reset setup without a final logged traceback, so that
individual re-entry path still requires a configured runtime rerun.

## Verification checkpoint

- Both new native reproductions failed against the untouched baseline: lost
  published table assertion; UBSan null BgImage member access.
- Both pass after repair under ASan/UBSan. Message control executes the actual
  archive-reorder message block, actual initializer/publication/lookup/resume
  functions and ten repeated reorders; no owned-message reload occurs.
- Mesh control runs the actual logger with native union definitions and real
  tinyxml2. Tests single images with zero and nonzero unk_0C and multi images.
- All 17 existing Unbound native controls pass.
- Both modified production C++ units pass native syntax checks using the
  previous full build's exact compile flags, redirected to this worktree.
- Additional story decoder, state restoration, CI and Windows build results
  must be recorded as they finish. No configured game rerun has occurred.

## Restore and next runtime test

Checkout `candidate/cel-unbound-smoke-repair-20261029` from the repository;
this document, production changes, native reproductions and CI wiring are
committed together. Compare against the exact baseline above. Use the normal
repository-pinned Windows build, never mix a DLL with an older graphics ABI.

Run `python3 -B tests/unbound/run_smoke_repair_tests.py -v` (C++20 compiler,
tinyxml2 development library, ASan/UBSan), existing Unbound native/state tests,
and `tests/custom_messages/run_story_open_tests.py` through the combined ABI
adapter used by `scripts/ci/run_regressions.py`. CI includes the new controls.

For runtime: use the same packs/order and Alt setting as the successful test.
Test normal title startup, token text 0xB4, reset/reselect Kakariko, exit Link's
House into Kokiri Forest, and retained bomb/pot/Skulltula/token interactions.
Use resource logging enabled for the title control so the repaired logger runs.
Keep accepted masters unchanged until these tests and user acceptance.
