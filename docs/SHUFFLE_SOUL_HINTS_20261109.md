# Soul item hints — bounded native follow-up

## Actual gaps and failing-first evidence

The existing OoT `Hint::GetItemHintText` and combo item exporter already had a
localized display-name fallback for RHT_NONE. Those paths were not all blank.
However, all 147 `RG_SOUL_*` and `RG_MM_SOUL_*` Item rows still used RHT_NONE,
so native direct `Item::GetHint()` returned the literal "No Hint" and had no
individual authored identity. The new actual-row/native-accessor fixture first
failed its Stalfos assertion before changing production rows/registration.

MM native gossip stones already admit RITYPE_LESSER with weight 6 and resolve
actual native/foreign names and source locations. Desired Items already lists
all named items. Saria's menu and runtime required MAJOR/MASK/hallmark items,
which silently omitted the legacy MM enemy souls (LESSER). A new assertion on
the actual full menu function failed for `RI_SOUL_ENEMY_OCTOROK` before the fix
(native_test.cpp:123 at reproduction).

No global item type, pool, item ID, save flag or gameplay soul behavior was
reclassified. No existing display-name fallback was removed.

## Source and ownership

- `soh/soh/Enhancements/randomizer/SoulHintCatalog.inc`: 147 distinct item-key
  identities with exact native EN/FR/DE names: 43 OoT enemies, 46 NPCs, 4 animals,
  2 misc, plus 52 existing MM boss/enemy aliases. Frozen and handed to root as
  the enum checkpoint dependency; not changed after root requested the freeze.
- `randomizerEnums/RandomizerHintTextKey.h`: owned/edited by enemy_first_drops,
  which appends the catalogue include after its location keys. Every new key
  is `RHT_ITEM_SOUL_*` or `RHT_ITEM_MM_SOUL_*`, distinct from first-drop location
  keys and prior serialized values.
- `soh/soh/Enhancements/randomizer/item_list.cpp`: only the 147 soul rows changed
  from RHT_NONE to their individual keys. The CI worker separately owns the
  RG_NONE-adjacent reserved-hole Item row; do not attribute that edit here.
- `soh/soh/Enhancements/randomizer/SoulHintText.h`: native registration helper,
  preserving native translations and using the same individual name at every
  clarity without additional RNG draws.
- `3drando/hint_list/hint_list_item.cpp`: include + registration call inside
  the existing native item HintTable initialization. This runs before Item
  construction, so registration uses catalogue literals, not uninitialized Items.
- `mm/2s2h/Rando/MagicHints.h`: exact soul eligibility, current saved family and
  permanent ownership checks, genuine shared counterpart identity normalization.
- `mm/2s2h/Rando/MiscBehavior/SariasSongHint.cpp`: admits useful soul identities
  in candidate, local and cross-world priority/fallback paths.
- `soh/soh/OTRGlobals.cpp`: only the existing OOT_GetMMHintCandidates default
  branch extended for genuine shared native OoT soul counterparts.
- `tests/shuffle_parity/souls/run_hint_tests.py`, `hint_cases.inc`: actual native
  rows/keys/registration, native item resolver/accessors and real combined-seed
  clue generation.
- `tests/shuffle_parity/hints/native_test.cpp`, `provider_test.cpp`: extended
  existing native MM/OoT fixtures with all four soul categories and shared controls.

No hint.cpp, first-drop location text, enum-owner, CI registry, index or commit
edits were performed by this worker. Root owns publication/checkpoints.

## Runtime behavior and genuine shared identity

Native OoT item hints now retrieve individual authored soul names directly.
Full native Hint::GetItemHintText and combo export use the same per-item entry.
Native sentence templates/source-region composition remain responsible for
normal clue grammar; no generic replacement sentence or fake location is used.

MM Saria accepts actual enemy souls despite their LESSER type, along with the
already-major NPC/animal/misc souls. It rejects off-family, permanently owned,
obtained/cycle-obtained/skipped or converted-junk targets. The new catalogue's
NPC/animal/misc ConvertItem path does not itself have an ownership-specific arm,
so Saria explicitly consults the real saved catalogue flag as well. The native
full ConvertItem still determines the resolved award. Desired Items retains
OoT's explicit request for a genuinely outstanding raw copy, even if its soul
is already owned or the family no longer gates gameplay.

A foreign OOT name becomes a native MM soul selector only when foreign.shared
is true and the exact SharedSoulCatalog pair identifies its counterpart.
Unshared names never become native soul wishes by spelling alone. This also
handles merged identities such as Wallmaster/Floormaster through the existing
pair catalogue.

For an OoT-world source, the provider uses the consolidated seed's effective
`sharedItems` category mask (CrossWorldRando writes effectiveSharedMask), checks
the loaded OoT family's saved option, and emits the genuine MM counterpart.
MM separately checks its saved family option and actual ownership flag before
Saria can select it. Requested menu sharing alone cannot enable a target.
The existing slot guard, native saved-placement lookup, trap/exclusion/status
filtering and exception-safe traversal restoration remain intact.

Normal static gossip clues are generated from seed placements; they are not
newly converted into live inventory queries by this patch. Existing MM gossip
weighting/collection behavior was audited and left unchanged. Dynamic
Saria/Desired Items are covered separately as described above.

## Verification

Failing first:

- Actual native Saria menu excludes Octorok (native_test.cpp:123).
- Actual Stalfos Item::GetHint resolves No Hint rather than its name.

Both reproductions pass after production changes.

Fresh passing commands:

```
CPATH=../recovered-tooling/deps python3 -B tests/shuffle_parity/souls/run_hint_tests.py
CPATH=../recovered-tooling/deps:../firstdrops-tooling/imgui-1.91.9b-docking python3 -B tests/shuffle_parity/hints/run_tests.py
CPATH=../recovered-tooling/deps python3 -B tests/magic_hints/run_tests.py
```

Soul fixture: all 147 actual native item-list initializers; complete native enum
catalogues; individual noncolliding key assertions; native registration hook;
actual full Item accessors, Hint::GetItemHintText, text encoding and item export;
all three clarity tiers; preserved EN/FR/DE names; real combined Generate()
clues at the actual seed source; shared OOT items at an MM source check;
excluded and start-owned seed metadata controls. Constructor/storage/UI
formatting are explicit fixture boundaries, not an engine-layout claim.

MM/OoT hint fixture: both MM guards, full production ConvertItem/native magic
helpers and Desired Items preparation; Enemy/NPC/Animal/Misc native and shared
MM-check targets, exact native item/check identity, source-world cross targets,
permanent-owned/off-family controls, Desired raw-copy behavior, unrelated and
unshared-name controls. OoT provider uses real native Context/Logic/Region/
Entrance/ItemLocation storage and actual source names; shared category bits
without an enabled native family yield no target. Existing trap/exclusion,
obtained, slot and snapshot/exception restoration tests remain passing.

Full native HintTable_Init_Item and SariasSongHint.cpp translation units pass
C++20 syntax checks in original and Combo guards with official restored deps.
The full OTRGlobals.cpp TU still stops at the pre-existing missing zip.h before
these provider bodies; its complete provider functions compile/run against
native storage types in the fixture. Full-world traversal, configured cold
launch, full linked build and GPU/UI acceptance are not claimed.

Final source verification also passed complete `item_list.cpp` syntax under both
original and Combo guards. Owned-path `git diff --check` completed with exit 0.
No index or commits were touched.
