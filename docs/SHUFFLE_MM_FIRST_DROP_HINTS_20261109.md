# MM first-enemy-drop hint audit and bounded wording correction

## Existing support established by source audit

- MM has 56 native RC_ENEMY_DROP_* checks with RCTYPE_ENEMY_DROP. Region logic
  references those real check IDs, often from multiple scenes: the placement is
  the species' first shuffled drop, not a unique fixed scene/container.
- Native Saria searches reachable, outstanding checks regardless of location
  type. A progressive magic item at RC_ENEMY_DROP_OCTOROK selects that actual
  check and the current native upgrade. Exact hint text uses the existing
  CheckNames identity (`at Enemy Drop Octorok`).
- Desired Items searches the same native saved placements, rejects obtained/
  cycle-obtained/skipped checks, and renders its source via GetHintLocationText.
  Its explicit raw wish may still name an inaccessible outstanding placement,
  matching the existing OoT behavior.
- Native MM gossip selects non-junk shuffled items at enemy-drop locations too.
  EnGs.cpp composes `<item> is hidden <location>`; enemy-drop checks are not in
  rcToWeight's exact-only exception list, so ordinary hints use vague text.
- MM_DumpRandoStaticData exports GetLocationNameForHint(id,false) as its real
  locationHints entry. The combined generator consumes that metadata and strips
  a leading `in ` before inserting a source into its normal area template.

This is source/fixture evidence. It is not a complete first-drop actor,
combat/reachability, generation, game formatting or GPU acceptance claim.

## Concrete reproduced gap

The native vague source method returned the identical typo'd phrase
`within a fomidable foe` for every enemy species. Desired Items and native
vague gossip therefore lost the actual first-drop source identity. Source tracing
shows that exporting the same phrase to the combined generator would yield the form
`... can be found at within a fomidable foe`.

The new isolated fixture first compiled and executed the complete native
GetLocationNameForHint and observed `within a fomidable foe`, then failed its
species-specific Octorok assertion before any production edit.

## Approved smallest correction

Only `mm/2s2h/Rando/StaticData/Checks.cpp`, inside
GetLocationNameForHint's RCTYPE_ENEMY_DROP branch, changed. It now derives the
species using the existing native enum-name formatter and returns
`in the first <species> drop`.

For Octorok:

- Native gossip composition: `<item> is hidden in the first Octorok drop`.
- Desired Items: `The slate senses a Magic Upgrade in the first Octorok drop`.
- Combined normal clue: `They say that Progressive Magic can be found at the
  first Octorok drop.`
- Exact/Saria identity remains `at Enemy Drop Octorok`.

No check names/IDs, source scene metadata, enum, placement, item type, menu,
frozen soul production files or normal ordinary-check branch changed.
The descriptor names the global first species drop rather than inventing a
scene where that species must be encountered.

## Isolated evidence

New files:

- `tests/shuffle_parity/hints/run_mm_first_drop_tests.py`
- `tests/shuffle_parity/hints/mm_first_drop_cases.inc`

Command:

```
CPATH=../recovered-tooling/deps:../firstdrops-tooling/imgui-1.91.9b-docking python3 -B tests/shuffle_parity/hints/run_mm_first_drop_tests.py
```

The fixture compiles complete production native source functions with actual
native MM headers: all 56 first-drop static check initializer rows, the native
name formatter/location method, full Saria selection, Desired Items lookup and
rune preparation, item-name rendering and native progressive conversion. It
also executes the actual combined Generate() against the source text exported
by the native location method.

Assertions cover actual first-drop check identity/current magic tier, exact
and species-specific vague strings, all 56 native source descriptors, the full
Desired message, obtained/skipped controls, inaccessible Saria versus explicit
Desired behavior, ordinary exact/vague check behavior, unknown sentinel and
combined grammar. No frozen fixture file was modified: the new runner reuses
its existing engine-boundary declarations as source input.

Native reachability/scene rendering, obtainability and module lookup are
explicit fixture boundaries. The placement uses real saved item/check fields;
there are no synthetic enemy locations. The combined sentence template is a
controlled native-shape text boundary; the production composition algorithm is
executed unchanged. This proves string/source composition, not in-game textbox
layout or actor defeat delivery.

The complete Checks.cpp translation unit also passes C++20 syntax under original
and Combo guards with the official restored JSON/ImGui/SDL/spdlog dependencies.
No index or commits were touched.

## Remaining

No remaining source-identity or native/combined grammar gap was found in this
bounded path after the correction. Full-world native combat reachability,
configured seed validation and game/UI appearance still need their respective
runtime acceptance. Existing static gossip collection/repeat policy was not
changed; dynamic outstanding filtering is verified only for Saria/Desired.
