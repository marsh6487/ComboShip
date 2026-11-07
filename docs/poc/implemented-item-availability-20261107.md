# Implemented item availability, 2026-11-07

## Scope and authorization

cor authorized this addition during publication of cumulative candidate
`432c3f85d8780783bb7cd22b07ca1e8017aea4c6`: exclude unfinished items from new
randomizer pools, retaining Fugue of Home only if it has an implemented action.
The following audit and correction descend from the separately preserved
prepublication fixture checkpoint `f10b2b1d0b03c68cf3e9306de03f8d6d5829f01e`.
This authorizes a source candidate and draft PR, not accepted-master promotion.

## Gameplay audit

| Item | Traced behavior | New availability |
|---|---|---|
| Fugue of Home | Grants a quest bit and recognizes/plays the song. OoT finishes with the generic song effect; MM's `OceffNei` effect counts down and expires. Neither dispatches a return-home action. | Excluded in every mode. |
| Ballad of the Hero | Grants playable song knowledge and a visual effect; its proposed gameplay action has no implementation. | Excluded in every mode. |
| Trirod | OoT has `Trirod_FireSummon`. MM maps the skill to selector 6, beyond the implemented cane cases 0–5, and performs no action. | Retained when OoT is playable; excluded in MM-only pools and new Black Market stock. |
| Open Chests | OoT consumes the ability. MM's foreign grant is explicitly storeless and no native MM chest gate consumes it. | Retained when OoT is playable; excluded in MM-only pools. |

The audit covered the explicit MM NEI, cane, wand, season, crossover and OoT gear
admission blocks; the corresponding OoT NEI, equipment, mask and RPG additions;
and every Black Market catalogue row. No further wholly unfinished admission was
identified. Functional Song of Command, Slate runes, intentional passive rewards
and the existing solo-OoT playable MM-song knowledge layer remain available.
An old comment describing Slate runes as unimplemented does not match their
current cast handlers and was not used to remove working items.

Fugue's missing authored GI is not repaired here: the conditional GI request
depends on an implemented gameplay action, which the audit did not find.

## Preservation contract

- Keep item enum values, catalogue IDs, save bits, legacy lookups and direct
  legacy item grants. Do not reroll or rewrite existing seed placements.
- Remove both unfinished songs at both MM supply paths: the legacy paired song
  substitutions and optional foreign quest additions. Preserve native Epona and
  Storms songs where those substitutions previously replaced them.
- Treat OoT as playable in combined mode or the legacy unified-pool backend.
  A compiled-in OoT host alone does not make MM-only items functional.
- Skip the unsupported Trirod skill in pure-MM progressive base-cane grants.
  Apply the same rule to `ResolveCanePresentation`: the check queue, starting
  items and oracle replay can convert a generic cane to an explicit skill before
  granting it. Retain the remaining skill order, paired-game order, existing
  ownership and explicit legacy Trirod grants. Both paths must agree.
- Apply the MM Black Market restriction to new stock and replacement generation.
  Continue validating and loading known frozen legacy base/replacement stock,
  including a previously saved Trirod. Preserve slots, tiers, prices and purchase
  identity.

## Evidence and remaining limits

The failing-first fixtures reproduced both unfinished-song suppliers, MM-only
Trirod/Open Chests admission and the generic-cane bypass before correction.
Sanitized native checks then passed:

- The complete pool generator in combined and standalone builds, 384 total
  game/option configurations, including starting inventory, density, native
  songs, functional runes and unchanged RNG-call counts.
- Actual cane conversion/resolution and grant bodies, 8,192 direct/converted
  grants over all 64 ownership masks, modern/legacy pairing and vanilla intent.
- 6,144 deterministic Black Market inventories, 2,048 replacement rolls,
  explicit/default filters, exhaustion and exact frozen Trirod state validation.
- Existing 4,096-inventory market coverage and both dungeon-reward pool suites
  (32 configurations per build).
- Existing native cane GI/presentation checks, paired/MM-only/unified resolver
  controls and exact market cane/sync checks. The editor's complete sanitized
  grant executable passes with its real mode ABI; its later GUI compilation
  remains blocked by the missing actual SDL2 SDK.

An independent source review found the conversion bypass and verified the
corrected production callers, public declarations and build inclusion. It also
independently passed the new sanitized availability suite. A source/CPU fixture
result does not prove a configured game run.

No full platform executable or in-game receipt has been validated in this
environment. The separately reported severe Black Market slowdown remains
unresolved; these availability changes do not claim a performance fix.
