# ComboShip evening publication

The user authorizes consolidating the October 24 evening source checkouts and
publishing required compressed asset parts to `marsh6487/ComboShip`, updating
candidate/cumulative branches and draft PRs, and producing a Windows candidate.
Keep `develop` and `master` unmerged.

Baseline: public `602e430613ce3045f782f1ec9b76bd5878879784`, tree
`292685496dd311b2b1b97c0847eea537988b6c5a`, identical to local `bb6f927e`.
Retain the earlier audited cumulative source and all repairs in this baseline.

| Input | Exact local head | Scope |
|---|---|---|
| Shop | `66fedceece85fbc02eb6bf216fa2f44380c1fa0d` | Close OoT camera, browsing/return, counter barrier and optional original room materials |
| Bridges | `48cb1f7d149bd2aaea129da7ce874c714af91920` | Owned texture sampling, vertical outward water spill, OoT Fountain resource ownership and readiness |
| Solo OoT | `32a20c26af19ff40c03fd1bef299c053b1e4a2c5` | Mirror receipt close, raw segment restore on market entry, native A/B glyph colors |
| What's New | `20a01120e616e2413f5262e2acd65737d76044b7` | Color-only highlights, missing settings, OoT Rod registration order; includes Solo OoT head |

Preserve accepted scene geometry, native collision/waterboxes, alternate setups,
child/adult access, standalone/combined guards, stock and purchase semantics,
manual Prelude actor ownership, original archives and independent input recovery
packages. The vertical spill supersedes earlier inward/shallow water revisions.

Use the proven lossless compressed pools: each decoded part is at most 96 KiB,
each encoded text part at most 128 KiB. Reuse existing blobs; upload changed asset
parts individually with SHA/size receipts. Verify the assembled remote tree
against the complete local tree before moving any candidate ref. Exclude private
saves, logs, dumps, original game archives and intermediate build products.

Implementation/native tests, full builds, configured gameplay and user acceptance
are separate evidence levels. This publication is a test candidate, not runtime
acceptance or master promotion. Required gameplay checks include shop camera and
counter interaction, Mirror pickup/reset/shop re-entry, button glyphs, color-only
menus and Rod placement, Lake flow/materials, and both-age Fountain access with
vanilla/Djipi packs. Record exact public commit/tree, CI checkout and artifact
digests in the final publication receipt.

## Integrated verification

All four exact source heads are retained in local merge ancestry. Every input
changed blob matches its input except deliberate CI composition and the later
formatting/fixture repairs. Every baseline/input CI command is retained.

Fifteen focused commands passed: actual MM shop/camera, native counter/talk,
room graph/factory/ownership/draw exits and asset identity, both-host Mirror,
Solo OoT receipt/glyph/raw-segment lifetime, real ImGui menu/shared-seed, and
regional sanitizer/presentation translation-unit checks. Direct native OoT Rod
registration and MM Item Pool probes passed. Market preparation reconstructs
200 OoT and 212 MM resources. Asset collision checking passes with no new
collisions. The complete project clang-format 14.0.6 pass changes only four
product files' formatting; tokens and concatenated strings are preserved.

Independent source review found no Critical issue, one Important missing-CI
coverage issue and no Minor issue. The new CI coverage guard first failed on
all five missing regressions and the parser dependency, then passed after the
required world group gained the existing room-material and raw-segment tests.
The full CI policy suite passes 22 tests; the registry has 226 entries.
`python3-pycparser` is installed for the actual native draw-exit flow runner.
The Generate menu fixture also reproduced its missing room-material declaration;
it now includes the actual production header and checks the new widget while
retaining previous color, identity, input and complete-rectangle assertions.
The complete real ImGui suite passes after that correction.

Publication uses an exact tree snapshot parented on public `602e4306`; the
independently restorable recovery bundle retains the original local ancestry.
Existing compressed texture/geometry pools are reused unchanged. No new encoded
texture part or original game archive is introduced. These source/native checks
do not establish full-build success or configured gameplay acceptance.
