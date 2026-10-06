# Recovery — October 5 MM runtime review

This is a code checkpoint, not runtime acceptance. Keep PR 34 draft and do not
merge or promote master. Continue on top of this checkpoint after an interrupted
session; do not redo the completed implementation.

## References

- Repository: `marsh6487/ComboShip`.
- Existing draft PR: https://github.com/marsh6487/ComboShip/pull/34
- PR head branch: `poc/mm-gi-routing-followup-20261003`.
- Starting head: `c10fd9a5cfd679ddff0c90defb15a06fc88f651a`.
- Separate recovery branch: `checkpoint/mm-runtime-review-20261005`.
- Local isolated checkout: `/workspace/scratch/484d84d6ce17/comboship-runtime`.
- Local working branch: `fix/comboship-runtime-review-20261005`.

The user authorized pushing the verified candidate to the existing PR and has
now explicitly requested uploading this checkpoint/recovery. No merge is
authorized. Check the remote head before pushing; preserve concurrent changes.

## User requirements and completed code

See [mm-runtime-review-20261005.md](mm-runtime-review-20261005.md) for the diagnosis
of each reported GI, icon, effect and hint regression.

The final map wording must use the actual native SoH hint table verbatim:
green “It’s ordinary.” and red “It’s masterful!” with native locale/color controls.
The final entrance sentence is “It seems the entrance is at [highlighted source].”
Do not add an ordinary exclamation; the user withdrew that assumption.
Keep native 75% font, wrapping/pages and the same text/sprite baseline regardless
of hint quantity. MM END control 0xBF has zero visual width.

The compass failure was reproduced through actual native Entrance objects,
CreateEntranceOverrides, SOH_DumpEntranceOverrides, the OoT donor builder and MM
native/foreign receipt endpoints. Mixed-pool mappings can land at a dungeon
entry rather than a boss room. The fix follows forward boss-door mappings with
bounded cycle/dead-end handling and actual destination boss reward lookup.

Final native MM seasons design:

- Autumn: EnWood02 tree canopy/leaf palettes and bounded native falling leaves;
  preserve native rain, snow and blizzard fog. Selected Alt materials remain
  authored. Ambient leaves use presentation RNG; native roll/drop behavior stays.
- Spring: add rain on Days 1 and 3; Day 2 follows native weather.
- Summer: clear Day 2 rain, sky and native storm ambience.
- Winter: present native snow particles. Native snow CUR/MAX/fog continue evolving
  underneath; no stale snapshots are restored. Supplemental actors are room scoped
  and removed on clear/Off/Autumn.

Review found and the implementer corrected two weather defects: restoring
ambience from particle counts could resurrect audio after the native sunset stop
at rain MAX=8; restoring snow from overridden CUR/fog could resurrect stale snow
after a MAX-only weather-tag exit. The final implementation observes actual
native audio queue channel requests and composes snow presentation over live
native fields. The new native fixture covers both defects and Autumn preservation.

## Evidence and remaining work

Focused checks passed before checkpoint, including actual all-61 MM pickup
camera geometry, exact six-mesh Din archive, static producer/receiver routes,
OoT/MM sword fallback, receipt native/foreign endpoints and generated route tables,
song geometry, existing anklet/full Heart Piece routes, foliage, and crash reporter
ownership/capture. The independent review has checked these paths.

Remaining sequence:

1. Wire `tests/seasons/run_native_weather_tests.py --sanitize` into the combined
   runner. Run the full combined gate on the final formatted tree; at checkpoint
   this cumulative pass has not yet been completed.
2. Apply pinned clang-format 14 to remaining changed production files. Most GI,
   icon, foliage and crash files have already been formatted. Keep parser fixtures
   source compatible; do not format unrelated source changes.
3. Complete the independent review of the final weather revision. Resolve actual
   blockers and recheck only affected scopes, then run whitespace/asset collision
   checks and the production receipt/weather syntax checks.
4. Commit the verified final candidate, fast-forward PR 34's existing head under
   active authorization, update its title/body, and wait for exact-commit CI.
   Fix build/check failures and provide the matching Windows build artifact.
5. Live GPU/mod-stack appearance, native season transitions and Alt-Tab remain
   user runtime acceptance. Do not equate fixture/build success with that proof.

The Alt-Tab crash is **unresolved**: the supplied report/disassembly narrows it to
an invalid native MM `Actor.update` callback. The old shared reporter incorrectly
belonged to OoT. The patch rebinds it on both resume paths and captures copied
actor ID/parameters, scene/room and callback address before MM update dispatch.
It avoids walking damaged actor memory after an update fault. Do not claim that
this fixes the invalid pointer or proves focus loss caused it.

## Local verification setup

Dependency header paths currently available:

```sh
CPATH=/tmp/comboship-runtime-deps/json/single_include:/tmp/comboship-runtime-deps/spdlog/include:/tmp/comboship-runtime-deps/imgui:/tmp/comboship-runtime-deps/thread-pool/include:/tmp/comboship-runtime-deps/stb:/tmp/comboship-runtime-deps/platform-include
```

Header pins: nlohmann 3.11.3, spdlog 1.14.1, ImGui 1.91.9b-docking,
thread-pool 4.1.0, SDL 2.30.11, and stb
`0bc88af4de5fb022db643c2d8e549a0927749354`. SDL2 is a temporary include alias.
No repository dependency pin was changed. Native clang-format 14.0.6 binary:
`/tmp/comboship-runtime-deps/clang_format/clang_format/data/bin/clang-format`.

CI source fixtures must be available locally (already fetched here):

```sh
git fetch --no-tags --depth=1 https://github.com/marsh6487/Shipwright.git cec63fce86b1f582f6e61ad6a98eca6c3cca784b c77c18587a976f6d6cb5c8f91f27593286469218
git fetch --no-tags --depth=1 origin 122dd5f68cb37fcf515c3726f8a77043db5dcdf4
```

The authoritative test commands live in
`scripts/diagnostics/run_combo_nei_regressions.sh` and the CI workflow. Additional
focused native tests: `tests/seasons/run_native_weather_tests.py --sanitize`,
`scripts/diagnostics/run_mm_weather_tests.py`, and
`scripts/diagnostics/run_mm_audio_runtime_test.py /tmp/mm-seasons-audio-runtime`.
Address/undefined/bounds sanitizers stay enabled; LeakSanitizer is disabled for
this traced host's /proc restriction. System package installation was blocked;
use the recovered headers or CI, and do not request an escalation for it.

## Source evidence kept outside git

All thirteen screenshots and `Fleet of Harkinian(20261005-230104).log` are available
in the conversation files and currently under the local `upload/` directory.
They have been inspected; the old missing-image messages are stale. Do not ask
for them again unless they are actually unavailable after recovery.

The exact recovered authorized Din pack is
`active-packs/Din_Fire_Sword_Shield_Progressive_Combo_POC1.o2r` under the workspace
root. It is also available by that exact name in the user's files. Its six meshes
passed `run_sword_mod_gi_tests.py --archives <active-packs-directory> --sanitize`.
The three recovered `Extra Items(1).o2r` variants in `lantern-packs/` are byte
identical (SHA256 `2c0951581b350d5924e38f203549c8fec11642424f4d0a4b2621b85110415cdd`)
and only supply Lantern icons, not the reported Lantern GI geometry.

Offline exact production-triangle effect previews are at
`/tmp/comboship-song-effects-review.png` and `.gif`. Reproduction tools are
checked in under `tools/nei_gi/runtime_preview/`. These omit native note/shimmer
and are not gameplay/GPU proof.
