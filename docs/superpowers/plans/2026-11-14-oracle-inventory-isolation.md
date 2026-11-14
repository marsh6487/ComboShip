# OoT solver inventory isolation implementation plan

Spec: `docs/superpowers/specs/2026-11-14-oracle-inventory-isolation-design.md`

## Global constraints

Keep the ammo source and its fixture repairs. Preserve location predicates,
sphere indices, runtime inventory/save format, original/Combo build guards, and
accepted master branches. Do not mutate live NEI during simulation. Use real
production methods and consumers for behavioral evidence. Do not claim a full
game build, uploaded-seed validation, or user acceptance from isolated tests.

## Task 1: Native regression and negative control

Own `tests/oot_solver_inventory/`. Build a fixture using the real Logic declaration,
complete production ownership/reset/grant methods, and the actual GC maze-left
predicate. Exercise global contamination, empty/mask/empty, grants/removals,
live/copy views, and production combined fill/replay. Run against the baseline
and record the expected semantic failure before the implementation changes.

## Task 2: Isolate ownership and retain legitimate grants

Own `soh/soh/Enhancements/randomizer/logic.h` and `logic.cpp`. Add private extended
slots, a live-view flag, and enumerated custom grant ownership. Reset private state
with a new solver save. Scope all direct runtime ownership helpers in HasItem.
Preserve wand settings, Bomb Bag implicit arrows, Pendant aliases, shared cells,
and progressive Roc grants. Correct the Shovel/Demise bottle-side effect. Guard
slot bounds and retain 16-bit IDs. Make Task 1 pass without relaxing its contract.

## Task 3: Integration, review, and recovery

Register the fixture in `scripts/ci/regressions.json`. Run sanitizer and adjacent
ammo/progression gates; compile the affected native code as far as dependencies
allow. Review the complete diff independently, resolve material findings, record
remaining limitations, commit the bounded change, and save/verify a recoverable
source checkpoint before handoff or publication.
