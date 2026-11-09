# Native souls production continuation — 2026-11-09

Reconstructed on the restored M1 tree `e360328d03d6d30e38151035a2d5a8d92994d975`. English item identities, native option keys, append-only IDs, save flag allocation, the 154-entry soul catalog and 68 shared counterpart edges remain stable. Shared controls in ComboMenu and publication are controller-owned.

## Implemented behavior

- Explicit scene/parameter NPC roles: 130 OoT and 213 MM rules, including schedules, ranch families, Moon child variants, native witch/fisherman counterparts, Princess bottle roles and Mayor meeting participants. Unknown/custom roles retain their native behavior.
- Explicit requirements for 391 OoT and 200 MM checks. Native quest sources are gated separately from starting/settings shortcuts; 23 OoT and 37 MM independent expected quest-event cases exercise those adapters. Physical, shop, photo, minigame, conversation, refill and automatic reward paths are included. King Zora's retained physical ice can melt with Blue Fire independently of his soul; native conversation rewards remain soul-gated. Kakariko's guard letter exchange requires the Guard soul while starting/open-gate shortcuts remain independent.
- Native Saria bridge, Requiem, Nocturne, Light Arrow and drawbridge transitions defer before consuming their event flags. Independent time-saver paths use the same availability check. Eligibility overrides cannot bypass source ownership.
- Enemy immunity covers native collision/damage, direct damage helpers, hammer quake input and both Dodongo bomb-swallow paths. Presence/update continue for ordinary immune enemies. Removing a missing-soul enemy cannot fabricate a room clear; transient room state resets on native room/scene initialization.
- MM Princess bottle release preserves the bottle when either required soul is missing or actor allocation fails. Consumption follows a successful native spawn.
- MM timed Trading Post requirements match the two native shop workers, including their shared 21:00–22:00 solver slice. Moon trial entry/return interactions, Sirloin's independent fire-arrow alternative and Postman/Kafei schedule sources retain their original non-soul requirements.
- OoT's two native option groups render Enemy/NPC/Animal/Misc options with names, descriptions and Off defaults. MM's native Souls card renders NPC/Animal/Misc counts through its existing accent wrapper. Help explains the physical blocker exception.

## Monotone passage policy

| Native blocker | Missing soul | Existing opening behavior |
| --- | --- | --- |
| King Zora, Zora's Domain | Native actor/OC remain; conversation/action is unavailable | Native moved position and fountain settings remain |
| Mido, Kokiri Forest / Lost Woods | Native OC and path tracking remain; conversation/ocarina is unavailable | Open-forest shortcut, already-moved flags and independent route tricks remain |
| Bomber hideout guard, East Clock Town | Native actor/OC remain; dialogue/action is unavailable | Previously stepped-aside position and return entrance remain |
| Biggoron, Snowhead | Native wind update continues; song/talk progression is unavailable | Open-dungeon scene flag and previously opened/cleared initialization remain |

Receiving a soul therefore cannot recreate a previously absent barrier. Other NPC roles retain optional existence. No new visual assets or save fields are introduced.

## Native translations

All 147 native/imported soul item rows and their dedicated hint registrations carry meaningful French and German labels. Authored native vocabulary is reused where available, including Oktorok, Araknon/Arachno, Killerana, Salia, Igor/Boris and Danse-Flamme/Flammenderwisch; proper creature/person names remain stable where no native counterpart is supplied. Existing five imported boss translations remain intact. Imported enemy labels identify MM to avoid localized-name alias collisions with OoT items.

All 43 first-enemy source types have named EN/DE/FR fragments. Native Text's EN/FR/DE order and CustomMessage's EN/DE/FR order are preserved explicitly. Canonical English check/item names are unchanged. First-drop templates, source descriptions, item hints and the actual encoder compose the selected language with independent source/reward accents and white resets.

## Verification boundaries

- All 21 `tests/shuffle_parity/souls/run_*tests.py` suites execute actual native bodies/adapters with real headers and enum identities. Capability, renderer, context and allocation callbacks are bounded fixture interfaces; these are not full game execution.
- Nine runtime/world/route suites passed ASan + UBSan in original and Combo modes. The native 147-item hint/export/composed-clue suite independently runs ASan + UBSan.
- Complete production translation units pass original and Combo syntax compilation with real native headers, actual CMake CVar/PCH/DLL definitions and existing native compiler allowances. Final logs enumerate each unit; compiler checks do not establish executable linkage.
- Native lifecycle fixtures execute actual actor removal/update bodies. MM fixtures additionally execute actual Giant/Bomber initialization and native week-flag setters/clearers, including saved flag restoration, opened/cleared Snowhead and the guard's moved position. OoT moved/starting initialization is preserved by source inspection and full translation-unit compilation; its fixture verifies update/OC/interaction and position preservation, not a real scene reload.
- First-drop pool/hint tests exercise all 4096 layout masks and 43 actual multilingual composed clues in both modes. Both MF_ENCODE output and native ReplaceColors bytes check red source, green reward and white resets. All 147 soul rewards are also composed into the native multilingual location template and checked at both encoder stages.
- Save-array tests cover shorter legacy/reused/empty tails and future longer arrays. Shared tests cover semantic matching, effective options, native receipt adapters and fill/replay boundaries.

Full native world seed generation, executable linkage, configured gameplay/scene proof and user acceptance are separate controller evidence gates. This report does not infer those from compiler or bounded fixture results.

## Recovery and reproduction

Each bounded milestone has a cumulative binary source patch ZIP independently applied to the exact public M1 base and verified against its target Git tree. Packages include the patch, changed source files, manifest and restore instructions. The final evidence package contains the exact syntax helper, compiler/suite/sanitizer logs and a manifest.

Run native-body suites with an installed C++20 toolchain and pinned native header dependencies:

```sh
for suite in tests/shuffle_parity/souls/run_*tests.py; do
    python3 "$suite" || exit 1
done
python3 tests/shuffle_parity/enemy_drops/run_pool_tests.py
```

The controller's committed `scripts/diagnostics/run_native_syntax.py` replays per-host native target flags; `run_native_compile.py` alternatively uses a configured compilation database. No ROMs, archives, game assets, private credentials or generated native binaries belong in recovery packages.
