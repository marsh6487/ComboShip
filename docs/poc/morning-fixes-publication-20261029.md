# October 29 morning fixes publication

## Authorization and preservation

cor authorized combining the October 29 actor/NPC, music removal startup crash, and OoT/MM chest-classification sources into one candidate; resolving integration conflicts; running regressions; pushing a candidate branch; opening/updating its PR; running CI; and providing a downloadable Windows candidate. Accepted masters remain unchanged. Protected-branch merges and public releases are excluded. Build success does not establish runtime acceptance.

The accepted repaired Unbound parent is `candidate/cel-unbound-smoke-repair-20261029` at `3cbb22d1f09e85deb206d2b085fc62105bc09fad`, tree `711c3cdf8bde43bb283345ac4ff20e1b3749f045`. Preserve its collision/capacity expansion, shading, prior integrations, messages and archive handling.

## Exact source references

| Group | Source |
| --- | --- |
| Native MM graph, Zelda skeleton counts and authored Saria/Twins/Fado tracking | `candidate/native-cutscene-vertices-20261029` at `4ca64d89b6a1fce834f8b234518ab227c5f92aa7` (PR71; three commits above the repaired parent) |
| Missing custom sample data, OoT/MM solo and Combo | `candidate/missing-music-sample-guard-20261029` at `7edef8a9d35b1fcc996bafb1ec87402ad7307799`; original `ComboShip_Missing_Music_Data_Source_Fix_20261029.zip` |
| MM masks and OoT/MM stray fairies | `e2a999b59f911e489818a30b38f1120fe668e775`, then `ccc0d4621a1b77f6bd30f71aafea76cb00db4ff8` on `candidate/mm-mask-chest-routing-20261029` |
| Necessary test-only graph resolver ABI update | Held combined local snapshot `8cffd2ad09fa737522687f508c77e24384d82f01`; original bounded update in `cd3a12a1a0ec062f3ac9dd565d2ef9540eb215e1` |

Candidate branch: `candidate/morning-fixes-20261029`. PR base: repaired parent branch above. Original donor workspaces and their untracked work are left unchanged. Source and assets outside these bounded changes retain the parent bytes.

### Durable donor identities

The authenticated publication preserves the exact tested donor trees as candidate-parent commits. Commit metadata differs from the original local donor metadata; source bytes do not.

| Donor | Published source commit | Exact tested tree |
| --- | --- | --- |
| Actor | `4ca64d89b6a1fce834f8b234518ab227c5f92aa7` | `22a91bd99c3c29fd73b0a6619d2c4a99ec493b99` |
| Music | `727f1d98117d17933aaed81108b5065056cdaf02` | `85421880c8f67e9b4e4cf970eec5a7f2cc130a8c` |
| Chests | `f2488479f31238ada6170bf49e88d1f46553e53b` | `b5f148d7d2e92f37713945bd05bf79f4b3f47b2e` |

The combined pre-publication source tree is `84b736496a13419dbc3d1c2815780b69be877ff5`, locally recorded by commit `9677001086ea8273db662c8a60d62b79d38015b9`. Applying its full binary diff to an independent Git index initialized at the repaired parent reconstructed that exact tree. Patch SHA-256: `8e4a53bc9d6ec30c3cc9f1336192bc225d9a6db5be2b553640fa5200a816f426`. The published tree adds this identity record; all tested production and test bytes remain identical.

## Verification ledger

- Exact donor comparison: the 14 production files matched their designated actor/audio/chest donors before the necessary loader compile correction below. All music and chest source/test/document files match the tested donor bytes.
- All 259 prior regression groups and their prior commands are retained. The missing-sample addition brings the candidate to 260 groups. Actor and graph-fixture commands are additive within the existing render group.
- Actor donor baseline: shared graph ASan/UBSan checks, all seven skeleton-swap cases and all 1,920 tracking checks passed in this worktree.
- Missing sample data: all 108 cases passed normally and all 108 passed under ASan/UBSan, covering OoT/MM and solo/Combo. The local sanitizer run disables leak detection because this host denies LeakSanitizer access to `/proc/task`; CI keeps its usual defaults.
- Both complete chest suites passed: MM production draw/UBSan and full C-actor compilation; OoT real C/C++ fairy bridge, native/foreign rewards and complete actor compilation in both ordinary OoT and Combo modes. MM emits its pre-existing controller/const warnings.
- Existing adjacent audio checks passed: 134,833 MM and 132,529 OoT native SSE2 mixer cases with zero mismatches; six PCM comparisons at 32/44.1/48 kHz with zero differing samples; both MM worker controls and five OoT worker controls. The OoT sanitized worker needed the same local leak-detection accommodation after the default run hit the host's `/proc/task` restriction.
- Native MM wide writer regression passed 182 checks. The broader `tests/wide_vertex_hash/run_tests.py --sanitize` is locally blocked by missing `spdlog/spdlog.h`, rather than a source diagnostic; it remains required in CI with the normal dependencies installed.
- All 25 CI orchestration unit tests passed. Full platform builds, complete CI regression groups and configured game validation remain separate pending gates.
- Independent read-only review found no material issue in the donor combination, reward classification, skeleton/tracking guards, audio guard placement or manifest preservation. Its minor trailing-EOF whitespace observation was normalized for the required diff check.

### Build correction discovered during publication

The actor donor's prior CI run `40887503849` failed both full platform builds. The Linux compiler identified `mm_asset_loader.cpp:886`: `Fast::F3DVtx` is not declared in this loader's header context. The production `Fast::Vertex` resource actually stores `std::vector<Vtx>` and reports `sizeof(Vtx)` bytes per element.

Ruling: use the resource's actual `Vtx` element type, with its owning GBI header included directly. This preserves the requested runtime stride and avoids borrowing a renderer-private type. A new fixture extracts the production selector call, compiles it against the actual resource classes and patches a command to the real second stored vertex with both legacy and wide runtime ABIs. Before the correction it reproduced the same undefined-type compiler failure. This fixture is part of the existing required graph command; no CI group was removed or skipped.

The graph-entry fixture similarly failed first on the obsolete resolver callback signature, then passed after carrying the held candidate's test-only resource-view update. Its declared raw 16-byte fixture storage remains distinct from the game's decoded 24-byte storage.

After correction, the real-resource loader fixture passed for both 16-byte and 24-byte runtime storage with 16/22-byte archive records. The complete shared graph suite, all seven skeleton cases, all 1,920 tracking checks and the sanitized entry-fixture suite passed again. `git diff --check 3cbb22d1` passes.

## Configured runtime checks remaining

1. Native Anju crying/standing umbrella poses without a replacement pack; camera rotation, pause/unpause and exit/re-entry. Sample other native cutscene adapters.
2. Adult Zelda with Alt on/off and re-entry; authored Saria, Twins and Young Fado while looking around. Sample Young/Adult Malon, Skull Kid, Treasure Shop Gal, Mikau and Lulu routes.
3. Boot twice with music core metadata but a missing audio chunk; confirm a named missing-data diagnostic and continued startup. Restore all seven matching chunks, restart and check music/fanfare/SFX playback.
4. Masks show Major chest art. Fairies show Minor art and small chests with contents-based sizing enabled in both engines, including foreign rewards. Check independent style/size settings, authored large-chest opening alignment and unchanged reward delivery.

No archive update is required for these source fixes. The optional repaired Twins pack is separately identified in the actor donor notes; it is not bundled into the executable and is not runtime-accepted by this publication.

## Recovery and rollback

Keep the prior repaired executable and its matching runtime files. Do not overwrite the user's saved mods/configuration during extraction. Restore the full matching prior runtime package to discard this candidate; do not mix DLLs from different builds.

For source recovery, fetch the candidate branch and the exact repaired parent; the PR diff is the bounded integration above that parent. To discard the candidate, continue from `3cbb22d1f09e85deb206d2b085fc62105bc09fad` while retaining the original donor branches and archives. No save-format change is introduced.
