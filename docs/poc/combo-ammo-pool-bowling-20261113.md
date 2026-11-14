# Ammo, pool policy, and Bowling Alley repair

## Authorized scope and baseline

cor requested traces of Bombchu/stick/nut ammo loss across hosts, gray arrow
counts, unwanted native SoH Feather, and native magic in Magic RPG seeds. The
same uploads contain a Bombchu Bowling Alley crash. After receiving the trace,
cor clarified that gray arrows were on a C button and probably also in the
pause menu, authorized implementing and publishing the fixes to
`marsh6487/ComboShip`, and explicitly requested no further authorization prompts.

Preserve baseline `b8068f8c24246c99e6d490f08b33b7e7196ec5d9` (tree
`a35749d80f3be7c3f3a7ab2d4b6e2642bfe08be7`). Windows merge build
`45dbd6901c6d59342ff28384cf4e8123112d8d2e` has that identical tree. The original
checkout remains unchanged. Work occurs in isolated branches and is published
only to `candidate/seed-fill-regressions-20261110`; PR #90 stays open and draft.

## Design decisions

Repair the native ownership/capacity/actor dependencies identified by executing
current source, rather than overriding resulting values at a late drawing or
sync boundary. Preserve ordinary item restrictions, consumption, ordinary magic,
solo modes, and the progressive NEI Roc chain.

- **Bowling:** bullseye and wall actors wait for their Bowling Lady dependency
  before entering states which dereference it. An absent soul-gated Lady remains
  absent. No dependency safety fix grants her soul or bypasses the spawn rule.
- **Ammo:** independently owned Bombchu capacity governs use/refill/import.
  Each host authors and imports stick/nut counts only with its usable local bag;
  MM's separate shuffled base-bag flags remain required. A missing local bag
  cannot export its forced zero over the peer's usable ammo. Consumable counts
  are exact, including spent zero, and are never merged by maximum.
- **Feather:** SoH owns native Feather supply in modern combined generation.
  Both MM fallback additions must be excluded there. Solo-MM fallback and NEI's
  separate progressive Feather-to-Cape items remain intact.
- **Magic:** the frozen RPG magic rule must replace native supply from both
  worlds and grant equivalent RPG progression to MM's generation oracle. Legacy
  native capacity remains earned; if a saved native floor masks RPG growth,
  credit the minimum equivalent RPG progression before a subsequent real pickup,
  without reducing capacity or treating sync retries as pickups. Ordinary magic
  behavior remains independent. Exact compatibility details require a failing
  production-behavior test before implementation.
- **Arrow text:** MM pause digits must consult the same effective restriction
  hook as the item icon. Preserve intentional disabled-button alpha in the HUD.
  Source probes did not reproduce gray nonzero enabled-bow HUD digits, so this
  pass does not claim that symptom is fixed or infer cor's unrecorded form/game.

## Evidence before changes

The crash dump and matching DLL agree on the null `chuGirl` read in
`EnWallTubo_DetectChu`; native layout and complete actor reproduction confirm it.
The existing MM ammo/transfer fixtures reproduce 20-to-zero loss and safe
ownership controls. Complete MM pool execution reproduces both Feather sources
and native magic supply. Current capacity helpers reproduce the native floor;
complete combined-fill fixtures retain MM native copies. Both HUDs' C-button
callers reset white and use button alpha; MM unrestricted form controls pass in
the HUD but fail the pause-digit restriction override.

These are bounded source/native tests and dump forensics. No configuration-
specific gameplay acceptance exists for a patch from this pass.

## Implemented behavior

The two hosts persist independent Bombchu capacity and infinity in previously
unused FC cells 95 and 96. Native OoT progression publishes the earned 20/30/50
ceiling; MM retains its ordinary Bomb Bag fallback when no independent bag
exists. Both hosts import earned capacity before current ammo, including a first
foreign receipt and a saved legacy receipt that predates the new capacity cell.

Ammo-only deltas can update an owned item. Explicitly unowned sources and
missing local bags cannot overwrite its consumable count. Deferred Bombchu
receipt materialization preserves already imported ammo; stick/nut capacity
receipts acknowledge the native tier already carried by the snapshot rather
than granting a second tier and refill. The legacy network path sends capacity
and receipt arrays before scalar ammo, and retains omitted ammo only in its
canonical cache so an unavailable bag does not cause futile hash resyncs.

MM use, refills, obtainability, shops, full-count colors, infinite-consumable
repair, developer refills and editor operations use the independent capacity.
Native Bomb Bag upgrades preserve independently owned Bombchu ammo. The native
refill obtainability path also avoids indexing the base item-slot array with
extended refill IDs before their normal remapping.

Combined free-pool generation excludes MM native magic when the frozen OoT RPG
rule is enabled. Configured starts and fixed native rewards remain supported.
The combined fill, playthrough, requiredness, magic-hint traversal and headless
replay pass exact RPG capacity to MM's oracle. A real subsequent RPG pickup
credits the minimum equivalent saved native capacity before advancing one level;
snapshot merges do not perform this credit. Existing seed placements are not
regenerated by these pool-policy corrections.

## Verification record

The newly registered CI entries pass together through `run_regressions.py`:

- Native Bowling actors: absent/late Lady, both wall targets, allocation retry,
  hit, fall, reset and next-round behavior in original and combined builds.
- MM pause draw commands: native restrictions, effective overrides, zero and
  full counts, original and combined builds.
- Independent Bombchu capacity: 48 capacity/infinity combinations per mode,
  native use/refills, shops, obtainability, grant/shared-tier replay and editors.
- Both native save layouts: missing bags, ammo-only deltas, 20/30/50 Bombchu
  spending and return, first and legacy receipts, spent zero, both stick/nut
  receipt orders and repeated synchronization.
- Both legacy network senders/rescans: dependency packet order, canonical cache,
  active/inactive authority, zero and packet-size controls in both build modes.
- Frozen RPG supply, fractional MM costs, complete bounded combined fill,
  replay, requiredness, fixed supply and pool balance.

Independent adjacent checks also passed: 384 native Feather pool configurations,
8,192 cane grants, 6,144 market inventories, 2,048 replacement rolls, 720 seeded
action-pool fills, the existing RPG and infinite-capacity suites, 35 CI handling
tests and eight native-ABI adapter tests. Changed production sources pass
clang-format 14 and whitespace checks.

The complete adjacent capacity suite also passes in original and combined builds
with ASan and UBSan. Its initial sparse-checkout compilation boundary was resolved
by supplying six additional exact, Git-blob-verified asset headers outside the
checkout; no production change was needed. This adds native bag gates, imports,
refills, starting items, saved ownership, wallet progression and foreign grants.
The separate cross-dungeon structural validator retains its existing all-items
inventory policy and is outside the new fractional-oracle fixture. Review found
no demonstrated seed failure from that separate path. Hosted builds and actual
configured gameplay are distinct from the native fixtures above.

### Hosted diagnostic follow-up

The first hosted run exposed missing dependencies in older extracted-source
fixtures: shared-item grants lacked the new capacity helpers, trade/cane receipt
selectors matched an earlier JSON condition, and the affordability canary lacked
its captured frozen magic rules. The follow-up changes only tests and this record.
The fixtures retain the actual production bodies, select shared fields inside
their owning functions, and supply the real helper definitions and frozen-rule
assignment. No existing behavior assertions or hosted checks were removed.

The complete six-command shared-item integration group passes. Moon's Tear passes
its complete normal and ASan/UBSan runs, including four 4,203-scenario paired
sync/materializer/save/reload roundtrips; cane passes normally and with ASan/UBSan.
Chateau now has a fresh-receipt control that failed before its extraction repair
and passes on both the preserved baseline and current source. The affordability
canary retains all original wallet/sphere controls under five frozen-rule cases;
its adjacent RPG magic suite also passes. Independent reviews and focused reruns
found no weakened assertions or remaining repair blocker.

The strict environmental catalog audit also exposed eight stale icicle
expectations for the former boolean magic rule. Only those expected conditions
were aligned with the actual two-unit Goron startup gate; identities, regions,
donor metadata and the strict audit are unchanged. Five environmental gate
commands pass locally. A bounded native RPG probe passes 208 comparisons across
the eight actual CHECK bodies at capacities zero, one and two, with native magic,
Cape, spent-magic and missing-mask controls. The full environmental native
sanitizer gate requires real SDL2 headers absent locally and remains a hosted
check; unrelated SDL stubs were not used.

One optional unchanged receipt-catalog probe reached a local OoT/MM enum include
conflict after thin asset headers were supplied. It has no baseline reproduction
or production-defect conclusion; its source is unchanged and the hosted full
checkout remains the authoritative check. Leak detection was disabled for the
local trade/cane sanitizer runs because this environment uses ptrace; address
and undefined-behavior checks remained enabled.

## Verification contract

Each correction starts with a failing behavior regression using actual native
code or helpers, then the smallest source repair and positive/negative controls.
Check local source formatting, register the focused regressions in the existing
CI manifest, and keep all prior checks enabled. Review the combined diff and
preserve an independently checked recovery checkpoint before publication.

Publication is authorized. It is not a merge, master promotion, release, or
claim of in-game acceptance. Inspect the exact published head, PR draft state,
and hosted checks; repair scoped build/test failures without new approval.

## Runtime acceptance still required

Enter Bowling with the Lady absent and present; acquire the soul and re-enter.
Use/refill/save/reload/switch both ways for each ammo type with and without its
bag. Generate native-Feather-off and RPG-only seeds, including required MM magic
checks. Test a legacy native-magic save followed by another RPG pickup. Compare
pause and C-button arrow text under the actual game/form that showed gray text.
