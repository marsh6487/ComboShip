# R20 actual native generation and replay — November 9, 2026

## Exact production runtime

Published source: `1d5188e2f47ffb38d29102b4219d50b457b76867`.
Equivalent local source: `ef9e2973d95a5ed8dd1bf0c6fa0654e917366a7d`.
Tree: `918b3b99d7785837f7db262d3d046d6ab924c2e7`.
CI merge: `f524d613f0c5b5d521e3fc1995aa3d7f54c70568`, same tree.
Workflow run `42673118642`, attempt 2; Linux job `129372410527`.

All Linux native targets linked, both resource archives generated, ordinary and
door-only entrance smoke tests passed. Native runtime artifact `13703053403`
ZIP SHA256 `d19f24290499836802ecbdd4a78b1519593c5a9f4c4504c656e1bc7d1e53c890`.
No private patches, replacement libraries, preload interposition or changed
native generation budgets were used. Final local proof used five independent
streams, each with sequential native children and fresh output directories.

## Complete matrix

The required GitHub native parity stage and independently repeated local matrix
both pass the same eight cases, seed `20261109`, with identical module hashes
and case manifests. Every case performs real OoT and MM standalone fills and
independent native replay, then real combined generation and original-driver
replay. These are complete native generation runs, not reachability fixtures.

| Profile | OoT / MM native solo fill and replay | Combined fill and replay | Foreign placements |
| --- | --- | --- | --- |
| own | PASS / PASS | PASS | 216 |
| own-excluded | PASS / PASS | PASS | 212 |
| stone-normal | PASS / PASS | PASS | 218 |
| stone-inverted | PASS / PASS | PASS | 194 |
| rusty-keys | PASS / PASS | PASS | 208 |
| mm-parity | PASS / PASS | PASS | 230 |
| all-off | PASS / PASS | PASS | 216 |
| all-parity | PASS / PASS | PASS | 584 |

Both games' boss keys are Own Dungeon simultaneously: all six OoT and four MM
physical owners are checked in every profile. Normal and inverted Stone Tower
ownership, exclusions, rings and actual cross-game placements pass. Additional
read-only candidate-set verification confirms MM confined placements belong to
the native producer's allowed-check sets. The OWN circular negative preserves
the item multiset and foreign records, proves native missing/owned-key gates,
and requires actual replay rejection exit 1; setup exit 3 cannot pass. Native
overfull-budget rejection also passes.

The all-parity profile includes 66 shared-soul components, all 211 combined MM
environmental checks, 27 OoT / 39 MM Rusty keys, counted rings, three lottery
nights, capacities, song events and OoT Clock. Native wallet source counts
OoT2/MM3 deduplicate to OoT2/MM1, canonical total3; profiles without Child Wallet
retain OoT2/MM0, total2. These are verified against real source pools and native
shared-pair exports. Standalone MM correctly has181 environmental checks
without starting OoT Iron Boots;30 underwater checks are enabled in combined.

## Native first-drop stage

Vanilla Off, On and On-excluded each pass actual native graph, native solo
fill/replay and combined fill/replay using the same exact modules. Off contains
zero first-drop checks; enabled cases contain43 checks and all43 source-soul
negative controls. All4096 Vanilla/MQ layout masks also pass native component coverage.

The nine additional genuine configured MQ/mixed cases now pass with the same
exact R20 driver/module identities. Both original and MQ archives were privately
recovered and validated: original native OOT_NTSC_US_12 (`0x693BA2AE`), MQ native
OOT_PAL_MQ (`0x1D4136F3`), complete ZIP CRC checks. The private runtime overlay
leaves the canonical runtime untouched and uses no availability placeholders.

| Layout | Off / On / On-excluded first-drop checks | Graph / native solo fill and replay / combined fill and replay |
| --- | --- | --- |
| Vanilla (CI) | 0 / 43 / 43 | PASS in all3 cases |
| All MQ | 0 / 40 / 40 | PASS in all3 cases |
| Mixed-even | 0 / 41 / 41 | PASS in all3 cases |
| Mixed-odd | 0 / 41 / 41 | PASS in all3 cases |

All12 unique cases pass, with660 actual own-soul negatives across both native
graph modes. This closes complete native first-drop generation/replay for the
configured Vanilla/MQ/mixed matrix. Original archive portVersion9.2.3 differs
from the Combo build: headless native graph/fill/replay success is separate
from graphical archive compatibility and live gameplay acceptance.

## Retained evidence

CI complete matrix artifact `13703404696`, SHA256
`080b7c6de76a984cbac87ab7fe1c3532506fc8094f7d6250e146c4b1b3c1c93a`.
CI first-drop artifact `13703523285`, SHA256
`8ac66384a9f9cb504f27a7cc0a392cb7ed36bd01353fc47bd8e89ce690159e02`.
The full12 quest proof is retained in
`Native_R20_FirstDrops_All12_Vanilla_MQ_Mixed_GREEN.zip`, SHA256
`dbd50dff9bdf9071194b6a30cea956d2b45a5fe5363c8461221e1c4731ecd7ce`.
All142 entries independently pass ZIP CRC, no assets/ROM suffixes, no runtime
directories and no symlinks. Only source/provenance/generated evidence is stored.

Each archive retains raw manifests, generation/replay outputs and logs. Logs,
spoilers, external game data and private inputs are not published in source.

Module SHA256 identities:

- `libultraship.so`: `0ecdc09f421ee19fe2fad4f0e00d0fe18439dbbe97832cab9d332c85932583ce`
- `lib2ship.so`: `6a9f811fb4da82cccdf0bbe1cc3bd68c2a681a6fad4d20fb82e548a0b6e5fe1e`
- `libsoh.so`: `50492f441ae8ec75b8c3ed74034fd28b32ad6ad17664bc75f9df2a09a3907968`
- `comboui.so`: `5d7a4ad7728d005bef398b213d446ff0cece04e0fd69483cf051e1179510652d`
- `comborando`: `123dc4a7fb872453b2a7028c224d3bb8e905f5bb2bf6c9dea6750492d6b17353`

## Remaining verification boundaries

Windows job129372410102 completed successfully: both game DLLs, LUS/UI, both
resources, launcher, ZIP packaging, staging, main upload and cache post-steps.
Artifact13705213365 SHA256
`7d866d0066839b038935851233691fde92eab7ba99c2736ee32908536d45ddbe`.
Its optional Windows CLI link was SKIPPED, not passed; integrated e2ec306f makes
that build/link unconditional and preserves optional upload. Full35 policy
controls pass; a fresh published Windows run must prove the actual CLI link. Four regression
groups exposed legacy fixture dependency failures. Minimal real-header/model/
logging-link repairs are integrated and their exact failed rows pass locally;
WORLD all5 remaining rows and90 complete TUs pass, RENDER all11 remaining rows
plus exact fairy ABI pass, SYSTEMS complete4-command UI row passes, and ITEMS
middle/rings/capacity closures and final tail pass: all67 ITEMS rows now have
passing CI-prefix/exact-row closure evidence. Across all6 groups this covers324
registered rows. A fresh full CI run on the cumulative published tree is still
required. Existing
assertions and compiler/diagnostic policies are preserved. Configured GPU,
private flame/form packs and live instrument audio acceptance remain separate.


## Actual MM authored-data verification

The recovered original MM archive independently matches all211 checks and279
aliases: exact actors/params/setup/room/list index, native object availability,
rock-ring slices, soil/bean switch/type/colocation/half-day rules and all30
conditional underwater witnesses. Actual native Identify/MatchesParams/
NormalizeSetup passes in both guards;51 scene/room and18 static collision
resources are decoded. No production mismatch was found. New source-only audit
and six synthetic decoder/full-policy negative-control groups are integrated,
with actual-data and fixture proof explicitly separate. See
`SHUFFLE_MM_ENVIRONMENT_RECONSTRUCTED_20261109.md` for static projection
exceptions and remaining player navigation/dynamic collision/GPU boundaries.
