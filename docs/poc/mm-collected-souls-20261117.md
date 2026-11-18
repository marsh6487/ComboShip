# MM collected rewards and soul inventory

Baseline: `d3cf01741084c188eb0a01a263018fa3de23fbe1` on the isolated November 17
bugfix candidate. User reports an already collected Chuchu first-drop reward
still displaying a soul after returning through the time cycle.

## Source findings

`MiscBehavior/CheckQueue.cpp` sets both `obtained` and `cycleObtained` when a
check is received. `MiscBehavior/OnCycleSave.cpp` clears `cycleObtained` at Song
of Time so locations can be collected again, while preserving permanent
ownership. `ConvertItem` then determines whether a repeated reward is still
obtainable or becomes renewable junk/refills. Advancing to a later day without
Song of Time is not this cycle-reset boundary.

Two confirmed source omissions made the Chuchu presentation unreliable:

* `IsItemObtainable` had no case for the appended NPC, animal or miscellaneous
  souls. These have no vanilla get-item entry, so they fell through to `true`
  even after their permanent soul flags were set. Conversion now checks the
  same catalog flag that `GiveItem` sets, and the permanent check receipt.
* The Chuchu body preview passed no check ID to `ConvertItem`, although its
  actual drop passed the check ID. It now supplies `RC_ENEMY_DROP_CHUCHU`, so
  progressive/one-time rewards also display their repeated-cycle replacement.

No reward location is deleted, and legitimate renewable drops remain enabled.
No reset of soul ownership or erasure of the permanent collected ledger was
found in these paths. The user's exact seed/runtime has not been replayed.

## Soul inventory page

On the MM pause map, **L** opens a controller-operated collected-souls overlay.
Left/Right on the D-pad selects bosses, enemies, NPCs/shopkeepers, animals or
miscellaneous souls; Up/Down pages through the list. L or B returns to the map.
The view contains all 111 native MM soul identities and reads their actual
permanent ownership flags each draw. Families not shuffled in this save are
identified as inactive. It neither reveals soul locations nor treats a check
receipt as soul ownership.

The overlay is MM-only, explicitly titled accordingly. It uses the existing
GuiWindow/ShipInit registration pattern, checks the active MM game in Combo,
and consumes its own navigation before native map/Soaring/debug-L processing.
Normal map controls resume when it is closed; native item/equipment/quest L
subpages are unchanged.

## Verification and remaining work

`python3 -B tests/shuffle_parity/souls/run_collected_preview_tests.py` compiles
the changed native conversion prefix and Chuchu preview body with real native
IDs, plus the actual tracker model. It passes original and `COMBO_BUILD`
variants, including shared soul grants, retained permanent receipts across a
cycle boundary, first/repeated-cycle preview agreement, and every tracker
ownership identity and family filter.

The same test with `--baseline d3cf01741084c188eb0a01a263018fa3de23fbe1`
fails the owned-soul obtainability assertion. Existing
`tests/shuffle_parity/souls/run_catalog_tests.py` also passes.

The full new GUI/input translation unit passes standalone and `COMBO_BUILD`
`c++ -std=c++20 -fsyntax-only` against actual game/libultraship headers and the
project-pinned JSON 3.12.0, ImGui 1.91.9b-docking, spdlog 1.16.0, SDL 2.32.10
and thread-pool 4.1.0 dependencies. No GUI or game-header stubs are used for
that syntax check.

This is source/component evidence. A full game build, controller interaction,
rendered map overlay, exact seed replay and user runtime acceptance are pending.
Do not promote this candidate by treating these checks as in-game proof.
