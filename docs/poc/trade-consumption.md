# Trade hand-in cleanup — source candidate

## Requested behavior

This adds cor's requested trade-item cleanup to the existing Blast Mask and
Goron/Zora combat checkout. Once an NPC accepts a submitted trade item, its
current copy leaves Kaleido and its equipped C-button/D-pad bindings. Merely
showing an item, declining a trade, or waiting for an unaccepted offer does not
remove it. A genuine later acquisition can make the item available again.

The source is pinned to ComboShip upstream commit
267fc64d7ac6b0f0d6c14a3f108b1a2e7299a02e, branch
candidate/seed-fill-regressions-20261110, draft PR 90. Its head was rechecked
and still matched during final verification. The isolated local branch is
poc/blast-mask-form-combat-20261112. Recovery metadata identifies the exact
final local commit/tree and expanded original-source baseline.

## Diagnosis

The unified trade wheel reads a separate saved ownership bitmask. Native
deletion/replacement removed a selected inventory entry without retiring that
bit. Pause normalization and cross-game ownership merging could then expose
the item again. MM randomizer also vetoed script and toilet-hand deletion,
while its scrub callbacks could replace the normal reward path altogether.

A local bit clear was insufficient: the shared earned-item registries and
ownership merge are monotone. They could restore a consumed item on game
switch. Some migration routines also called the real grant API when they
observed historical flags, incorrectly turning history into another pickup.

## Implementation

Each of the 23 stable wheel indices now has an appended 32-bit availability
revision in both NEI save structures. An even value means the earned current
copy is available; an odd value means it was consumed. Zero retains legacy
availability for an owned entry. A real consume or reacquisition advances the
revision only when its availability changes. Repeated callbacks for the same
state are idempotent. UINT32_MAX is terminal consumed and never wraps.

The original ownership bitmask and randomizer acquisition/check history remain
earned history. Effective wheel ownership also checks the revision. Historical
folding and migrations do not advance it. Both save codecs persist all 23
values and default missing legacy values to zero. FleetSync merges revisions
by maximum, reconciles obsolete native entries, and acknowledges historical
FC grant deficits without replaying a consumed item. The new wire helper is
shared by both engines. Both games must use the updated source to carry this
availability state across a switch.

Consumption removes exact identities from the OoT child/adult trade cells and
all seven C/D bindings in live, child, and adult equipment banks. MM removes
exact identities from all three native trade homes and its seven shared C/D
bindings. Another item in the same home is preserved. Foreign MM wheel
placeholders are retired against their old cursor before selecting a new one.
Pause normalization also scrubs stale cells before same-frame equip lookup.

### Accepted hand-ins

| Game | Accepted transaction |
| --- | --- |
| OoT child | Castle Talon receives the Chicken; the mountain guard finishes accepting Zelda's Letter |
| OoT adult | Pocket Cucco, Cojiro, Odd Mushroom, Odd Potion, Saw, Broken Sword, Prescription, Frog, Eyedrops, and Claim Check exchanges |
| MM | Moon's Tear and all four deed exchanges, Letter to Kafei posting, Express Mail delivery, Pendant delivery, and accepted toilet-hand paper |

Native adult OoT receipts retire their input at successful NPC transitions.
Randomizer consumption occurs before the completion flag queues the shuffled
reward, so a reward of the same item can remain a new acquisition. King Zora's
Frog-vs-Tunic identity is retained across the get-item handshake. MM scrubs
retire the submitted item before both native and shuffled reward handling.
Script/toilet deletion honors the final hook decision.

### Preserved semantics

- Adult Talon waking up leaves Pocket Cucco available for Cucco Lady.
- King Zora's Tunic reward does not consume Prescription.
- Room Key access/display does not consume the key.
- Keep Express Mail retains its existing first-delivery exception.
- Pendant's permanent combat perk and effect toggle survive delivery. Legacy
  boots migration records history/perk ownership without reviving a consumed
  Pendant. Cojiro's separate companion ownership is unchanged.
- MM randomizer explicitly restores historically earned MM trade copies at
  the next three-day cycle, matching its existing repeat-cycle policy. It
  does not restore consumed OoT items. Native/Vanilla consumed copies wait
  for real reacquisition.
- Real native/randomizer hatches grant a newly available Cucco after an older
  bird was submitted; pause flags are not used as acquisition events.
- Mask ownership, ordinary wheel selection, and quest/progression flags keep
  their separate behavior.

## Verification

The final focused registry run passed all seven groups. Logs are included in
the recovery package. The fixtures compile extracted production functions
against native save/item/interface layouts where applicable; they substitute
specified engine services and do not run the configured game.

| Group | Evidence |
| --- | --- |
| Trade core | All 23 indices, native slot/binding cleanup, repeat callbacks, reacquisition, placeholders, permanent perks; OoT/MM standalone and Combo; ASan/UBSan |
| OoT hand-ins | Actual NPC and randomizer branches; accepted/refused offers, Tunic/Frog, adult Talon, same-item rewards, true grants and exact deletion |
| MM hand-ins | 124 checks per standalone/Combo variant; script vetoes, scrub redirects, exact paper deletion, cycles, legacy boots migration, and invalid-ID bounds; ASan/UBSan |
| Save codecs | Complete extracted OoT NEI save/load and MM NEI JSON conversions; all 23 values, legacy defaults, round trips, sparse/invalid MM arrays, permanent perks; ASan/UBSan |
| Trade sync | 23 current-copy round trips, stale/partial messages, reacquisition, and all 21 FC trade mappings in both port tables; ASan/UBSan |
| Existing unified wheel | Native age banks/hatches, mask separation, 23 IDs, same-frame C equip, page return and stale flags; normal and sanitizer runs |
| Prior combat | OoT Blast action/HUD/shield collision; MM mask actions 1373 checks per variant; form melee 15/15; existing Goron field 9/9 and Zora 10/10 |

The trade tests were demonstrated failing on old source before fixes. The
independent reviewer also reproduced the OoT flag migration and MM old-boots
migration revival paths, then verified their corrections under sanitizers.
No remaining actionable source finding was reported.

New tests are registered in scripts/ci/regressions.json. The portable package
runner supplies its pinned JSON/spdlog headers and runs only these seven
focused groups. The entire project-wide manifest is not a passing claim:
this is a partial source snapshot and unrelated test files are absent.

## Runtime boundary and acceptance

No full SoH/2Ship executable build, asset-backed animation playback, or
configured game/save/controller session was available. This is a recoverable
source candidate for the project's normal native build and gameplay checks.
No ROM, accepted archive, or configured asset set was changed or repackaged.

Use a copy of the actual save for the following checks after rebuilding both
game targets:

1. Complete one child and one adult OoT trade; check the grid and all bound
   buttons, reopen pause, change age, save/reload, and switch to MM and back.
2. Check King Zora Tunic vs Frog, adult Talon's Pocket Cucco, and completed
   Biggoron Claim Check delivery. Declined or unfinished offers retain items.
3. In MM, trade each deed and deliver both letters and Pendant; test the toilet
   hand and Keep Express Mail's configured exception. Room Key remains access
   proof; the permanent Pendant perk remains usable after delivery.
4. Reacquire an item and verify availability returns. In randomizer, use Song
   of Time and verify earned MM copies return while consumed OoT copies do not.
5. Repeat the combat acceptance matrix in the preceding Blast Mask/form report,
   especially adult Master Sword in MM, child Link, B-only activation, shield+B,
   and Goron/Zora attack reach against actual enemies.

Existing pre-patch historical bits are not bulk-erased from old completion
flags: those flags cannot distinguish an old consumed copy from legitimate
reacquisition. The new policy records completed hand-ins going forward. This
pass also does not change expiration of unconsumed MM trade items or bit-only
eggs, nor resolve the earlier form-combat wall-hit flag timing caveat.
