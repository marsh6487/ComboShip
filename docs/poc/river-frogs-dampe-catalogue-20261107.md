# Independent river frogs and ghost Dampé, 2026-11-07

| Field | Record |
|---|---|
| Source baseline | `51ce4e17cb4d397a8a4efae4c4fd17407406e066`; preceding cumulative night audio, Black Market, MM hint and Saria corrections retained. |
| Candidate | `candidate/frogs-dampe-catalogue-20261107`; exact final commit/tree and file hashes are in the cumulative recovery manifest. |
| Scope | Five colored Zora's River frog identities 29–33, params `0x7D04`–`0x7D08`, and ghost Dampé identity 34, params `0x7D09`, through the existing OoT En_Viewer catalogue (`0x002A`). |
| Preservation | Existing identities/poses, normal native frog/race actors, progression, camera, authored position/yaw and normal/Combo guards. No scene placement, new external asset pack or native-MM presenter. |
| Result | Source, native CPU fixtures and full viewer object compilation verified in both guards. Full executable and configured appearance remain untested. |

The six entries are decorative. They request their existing `OBJECT_FR`
(`0x00E6`) or `OBJECT_TK` (`0x0089`) bank and wait for it to load, initialize
the public skeleton/animation resource path with dynamically owned joint/morph
tables, and release that state on destruction. They do not call En_Fr or
En_Po_Relay init/update: those actors contain shared minigame/singleton state,
visibility and positioning rules, race timers, rewards and dialogue actions.
These new entries have no talk, targeting, tracking or player OC collision.

Frogs retain the five native environment colors, two eye textures on segments
08/09 and billboarded eye limbs 7/8. Ghost Dampé retains his float animation,
three eye states, native opaque skeleton stream, halo at limb 8 and lantern at
limb 14 with environment alpha 128. His body starts with neutral environment
color to prevent inheriting a preceding frog's tint. The lantern uses a fixed
native-range warm color rather than the race actor's draw-time RNG/light
updates. Multiple copies do not share native actor state.

Verification performed:

- The first catalogue fixture failed before the additions because `0x7D04`
  did not resolve to type 29. A separate neutral-ghost draw assertion failed
  before its entry-state color reset. Both pass with the final implementation.
- `python3 -B tests/river_frogs_dampe/run_tests.py` passes with ASan/UBSan in
  solo SoH and Combo: eight simultaneous instances (five colors, a ghost and
  duplicate yellow/ghost), pending-to-ready object loading, native and
  replacement resource ownership, idle/blink, exact eye/color/accessory command
  boundaries, retained transforms, no interaction/OC callbacks and paired
  native skeleton registrations/frees. It also compiles the complete production
  `z_en_viewer.c` into real-header objects in both guards.
- Existing `tests/custom_messages/run_cutscene_actor_tests.py`,
  `tests/regional_actors/run_tests.py` and
  `scripts/diagnostics/run_static_story_culling.py` pass. Existing core actor
  and Zelda registry fixtures pass under ASan/UBSan. The regional fixture adds
  fail-if-called boundaries for the new OoT draw functions.
- The existing native message-opening fixture exposed a missing declaration of
  `CVarGetInteger` in its generated C unit. Including the real console-variable
  bridge repairs the fixture; production message code is unchanged. Native
  opening/decoding/reopening passes in both guards and all four test languages.
- The six JSON presets reference existing native resource paths, IDs are unique,
  all 25 CI policy tests pass, changed/new C files pass clang-format 14.0.6 and
  `git diff --check` passes. The focused runner is registered in the render CI suite.
- An independent read-only review found no blockers and reran the focused
  sanitized presenter and full viewer object compilations in both guards.

Host verification uses GCC14 legacy resource-pointer compatibility flags for
existing OTR declarations; the existing C++ regional fixture additionally uses
the available nlohmann/spdlog include roots. Test renderer/resource services are
explicit boundaries: no complete native archive geometry traversal, live pack
loading, Alt switching or game execution is claimed.

Placement names/codes and model details are documented in
`docs/static_story_actor_poc6_params.md` and
`combo/actors/RiverFrogsDampeActorPresets.json`. These follow the established
manual Prelude Params workflow; an external editor dropdown is not changed.
The decisive configured test is to place all six with a rebuilt OoT module,
verify their size/idle/eyes and Dampé's accessories with native and compatible
replacement assets, then re-enter and pause/unpause without movement, camera,
dialogue or race/minigame side effects.

The cumulative source checkpoint is independently restored and hash-checked
against its exact public baseline. The prior source candidate was authorized
for publication; the user requested a combined authorization including these
additions before the expanded candidate is uploaded. No master promotion.
