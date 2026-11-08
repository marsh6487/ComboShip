# Saria stat-upgrade guidance for existing seeds, 2026-11-07

| Field | Record |
|---|---|
| Baseline | Actor/settings behavior from `e374318d`; current candidate parent `799f09e4` on `candidate/hint-colors-mask-menu-20261107`. |
| Report | Saria absent in Sacred Forest Meadow despite Shuffle Zelda's Letter, Skip Child Stealth and Skip Waking Talon being off. Earlier saved-seed evidence had Magic Stat Upgrades on, classic Saria Hint off and the Met Zelda event unset; the current private save was not supplied. |
| Cause reproduced | The previous fix stopped clearing the classic hint preference for new settings, but preserved the old saved zero. Actor eligibility, text selection, text hooks and hint generation still required that zero-valued preference. Magic stat target selection alone could not make Saria spawn or speak it. |
| Scope | Effective stat guidance when either classic Saria Hint or Magic Stat Upgrades is enabled; targeted construction of one missing legacy hint message. |
| Preservation | Stored options, Zelda/story/song/reward flags, item placements/ownership, actor transforms, adult/vanilla rules, existing enabled hint text, runtime RNG and hint/accessibility metadata. No asset change, save migration, seed reroll, publication or master promotion. |
| Verdict | Source/native CPU integration and named real-header object compilations verified; the user's loaded build, age and winning pack stack remain runtime acceptance questions. |

The three reported options do not select Saria's actor mode. Native song
eligibility still requires `EVENTCHKINF_OBTAINED_ZELDAS_LETTER`, the event set
when meeting Zelda (or by the existing unshuffled Starting Letter skip). It is
distinct from simply owning a shuffled Letter. The new stat guidance enables
only child randomizer talk mode before that event; it does not grant the song,
trigger its reward or set that event. Classic hint off plus stat upgrades off
retains the original story gate.

The effective policy is shared by native actor eligibility/text selection,
static message registration, native hint generation and cross-game composition.
Both generation entry points retain full stat reachability capture even when
the classic-meter hint preference is off. Existing saved preferences remain
unchanged.

For an old saved slot without an enabled `RH_SARIA_HINT`, the first actual Saria
textbox resolves only that row. It scans the saved item table across `RC_MAX`,
because cold loads do not restore the generation-only `allLocations` pool or
graph areas. It prefers a valid matching cached full-sphere result when one
exists; otherwise it chooses a stable native check and its static home area.
If the upgrade is across games, it reads the current slot's baked
`hints.ootItemAreas["Magic Stat Upgrade"]`, rather than relying on a warmed
foreign-area cache which is absent on cold load. A legacy target without full
sphere metadata is an existing placement, not proof of earliest reachability.

This path constructs clear localized native talk/song MESSAGE templates,
encoding their color spans into durable percent escapes before storage.
Raw MESSAGE save/reload therefore retains the green/red spans; Saria's song
textbox restores native blue styling in both build modes. It neither consumes
RNG nor marks locations as hinted/accessible. Existing enabled messages are
preserved. A matching complete search proving no upgrade reachable prevents
the placement fallback. Missing or malformed target metadata produces a
formatted, terminated, localized in-world response without reporting a hint
as revealed; the song variant retains its event/blue-textbox protocol.

The full-chain fixture uses the exact normal-child actor row independently
decoded from retained `Meadow_POC3.o2r`: actor `0x146`, position
`(118,489,-2941)`, yaw `-3277`, params `0`. Archive SHA256 is
`daf804a5c326bd585ad75e7d621208d4628107ed78042ec32f37583a6162c81c`.
Its child setup 0 contains that actor and `OBJECT_SA` (`0xBC`); missing child
night setup 1 falls back to setup 0. Adult setup 2 has no Saria and missing
adult night setup 3 falls back to it. This disproves a normal-child omission
in that retained candidate, and does not identify the user's active winner.

Verification:

- `tests/saria_magic/run_spawn_interaction_tests.py`: address/undefined sanitizers; actual scene-entry, actor initialization, update, native talk offer/accept/close, real text-hook conditions, message callback, lazy resolver, native template insertion/color encoding/autoformat and native MESSAGE constructor. Allocation/resources/font storage and player button acceptance remain explicit boundaries.
- Replaying actor source from `e374318d` with `--actor-source` fails at the old saved-hint-zero/stat-on actor-survival assertion; the current source passes. Removing lazy runtime repair separately fails the expected native-area message assertion.
- Controls cover before/after Zelda, classic off/stat off, adult/vanilla, completed song, native/cross old saved placements without a generated row, existing enabled messages, absent/malformed metadata, matching verified-none and stale caches. Raw save-style reconstruction retains colors, song styling, control codes and native message termination. Settings, item identities, runtime hint/accessibility state and RNG stay unchanged.
- `tests/saria_magic/run_tests.py`: actual cross composer, full native sphere capture, repeated seeds/import invalidation, stat guidance with classic preference off, native progression and original progressive-meter controls pass.
- `tests/cross_hints/run_gossip_routing_tests.py --sanitize`: full native generation, OoT/MM routing, translations, ownership/reload and category controls pass.
- Full production units `z_en_sa.c`, `Messages/StaticHints.cpp`, `3drando/hints.cpp` and `hint.cpp` compile directly with real engine/libultraship headers and the available dependency headers. Commands/logs are under `/tmp/saria-production-objects/`. C compilation uses GCC14 compatibility flags for the engine's existing resource-pointer declarations. No test SDL declarations are used. This is object compilation, not a complete executable build.
- Changed production files use clang-format 14.0.6; `git diff --check` passes.
- The existing Saria CI entry now also runs the sanitized spawn-to-message integration fixture. All 25 CI policy tests pass.

Next configured check: load the existing child save with stat upgrades enabled
and its classic Saria Hint value still zero, enter the retained Meadow candidate,
talk and close the magic hint, save/reload and repeat. Confirm the song reward
still waits for its original Zelda event. If the actor remains absent, the
existing progression/actor probes distinguish a missing winning actor entry,
failed resource/actor spawn and an initialization eligibility kill. No new
scene export or user save rewrite is required by this source correction.
