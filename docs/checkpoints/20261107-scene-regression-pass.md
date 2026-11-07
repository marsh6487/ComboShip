# Scene regression candidate — 2026-11-07

Baseline: captured build `93309d745b260fa1eba1a55d658db717ee350d0f`, stacked above `candidate/sword-hints-milk-20261105`. Accepted scene archives and input saves are unchanged. The candidate has not been accepted in game or promoted to master.

## Performance evidence and candidate

The same Young Din mesh costs approximately 14.65–15.11 ms of renderer CPU in the supplied Ranch, Temple, Link's House and Kokiri captures. It does not explain the Ranch-specific increase by itself. Ranch additionally renders Malon at approximately 7.78 ms and has a distinct game-side `play_draw/end_hook` median of 33.82 ms, versus 0.387 ms in the captured Kokiri view. These are separate timing boundaries, not additive FPS estimates. Profiling overhead is included. No Kakariko capture or equivalent Kokiri NPC cohort was supplied.

A source defect repeatedly allocated and scanned supported seasonal textures when no pixel changed. The candidate caches those completed unchanged evaluations using the existing source/tint/region/mask key, retaining source identity until normal retirement. Unsupported inputs remain retryable; accepted tint math and pixels are unchanged. Capture-only `season_foliage/update` timing and scan/cache counters permit a decisive comparison. The existing capture does not identify the owner of the entire expensive hook.

## Repairs

- Seasonal rain lifetime follows PlayState/scene, not each room. Native/authored rain owners remain. Tests compare target density, actual emitted PCM and source intent through repeated room changes, density/thunder owner handoffs, hidden-cycle resumption, native nature-channel handoff, pause and exits. Separate reported lasting silence remains unproven in game.
- Potion receipts retain proper native content names and the complete native tutorial without an extra catalog-name-only page. Item identity, grant/latch state, localization, controls and the existing icon policy remain intact.
- Wolf runtime-generated vertex tokens now use 16-byte exported offsets, avoiding double scaling under the combined 24-byte ABI. Both standard and HD are tested. HD blobs are removed from generated built-in assets; standard blobs are byte-preserved. Both menus disable HD when the selected resource owner has no HD resource, and saved HD selection safely falls back to standard.
- Byrna collision uses a normalized cane frame and geometry-sized padding rather than scaled native sword bounds. Axial reach is the visible 6677.55 limb units plus 200 units. The asymmetric crown envelope is [-900,+1900] transverse and +/-400 depth. Native swept collision/reset and combo-counter semantics remain; other weapons and animation selection are unchanged. Fixture scale cases are not real skeleton playback.
- Saria's stat-upgrade hint selects the earliest oracle-reachable magic sphere with stable OoT/check-name ties, independently of victory. Native pending-item capture survives winning spheres and pruning; context/import/reset boundaries invalidate cached rankings. Child hint-only conversation may precede Letter when enabled. Existing hint preferences and saves are not automatically enabled or rewritten; song/reward progression retains its original gate.

## HD Wolf installation

Use matching rebuilt `soh.o2r` and `2ship.o2r`; both contain standard Wolf only. Put the same optional `wolf-link-hd.o2r` under `mods/soh/` and `mods/2ship/`, then restart. An old application archive that still contains HD is not a valid absence test.

External archive SHA-256: `85ddcd803149e86982496945b1ebd681462d3b701ceef202b5fcaddbd325dcb7`.
HD resource SHA-256: `7705dc56447b571a0d13b019fb4461ac5147dcaaf441db16d24dfe9a990f451c`.
Standard resource SHA-256: `5047bd1f0a639db9ad50f87063cbd52303d8fe8c0056896d34ec1e6cfd5d99f2`.

## Verification to date

- Rain: fail-before at the first room transition; normal and ASan/UBSan/bounds production-controller/resolver/mixer tests pass across 124,800 trace frames. Existing seasonal, native weather actor and real rain-audio suites pass.
- Foliage: fail-before repeated unchanged builds; four normal/sanitized suites pass, including 51 updates/one scan/50 hits, source/season/mask changes, retirement and unsupported-data repair. Native probe linkage inspected; complete executable link remains separate.
- Potion: fail-before shared red-potion name page; entire existing receipt umbrella passes normally and with sanitizers, including 72 short-native-name/foreign-label/locale cases and cross-DLL integration.
- Wolf: actual production display-list builder and interpreter address resolver pass with ASan/UBSan under the combined s32 ABI for all 8112/8640 vertices in both hosts. Real loaders/actions/lifecycles pass UBSan and fast-math; actual Blob/owner/Alt selection, package checks and exact preserved rig/animation/audio/eye/chain bytes pass.
- Cane: fail-before measured 226.58 world-unit child-scale bounds; native helper/transform/sweep/reset fixtures, both host held-sword regressions, equipment and geometry suites pass. Native age/skeleton animation and actual tree interactions remain runtime checks.

LeakSanitizer is disabled in this managed ptrace environment; address, undefined-behavior and bounds instrumentation remains active where stated. A fixture pass does not establish complete platform compilation or configured gameplay acceptance.

## New MM capture limitation

The second supplied file records Temple of Time -> Link's House -> Kokiri, then reaches the 256 MiB cap. It does not record MM Black Market. This build's FrameTiming/RenderFlight recorder is wired to OoT, not MM; repeating an OoT-labelled capture while MM is active will not diagnose the shop. MM Black Market world/item hooks are measurement targets, not established causes. A useful future capture must identify MM/module/scene and cover exterior, entry and 10–20 seconds inside.

## Recovery and remaining checks

The original subsystem milestones were checkpointed before an environment reset. Recovery reconstructed the exact reviewed tree `1de266c9b80cc0ee0a4d8e3a0159e87afc665a37` as local commit `691c35fe`; no functional source was lost. The durable source checkpoint contains candidate overlays, exact base/head identities, deletion list and SHA-256 manifest; restore onto the exact baseline and verify all hashes. Original HD bytes are preserved separately before bundled removal. No private save or full gameplay log is tracked in the repository.

Final task and whole-candidate source reviews approved with no blocking findings. All four new gates pass through the combined-ABI compiler adapter after clang-format 14. Actual full translation units for native hints, fill, SeedContext and SeasonFoliage pass syntax checks with combined graphics definitions and the configured dependency headers. Source format and spawn/routing checks pass. Saria composer, post-victory target search, native winning-sphere capture, reset/import cache and existing native seed/settings/gossip routes pass.

Remaining: full platform compilation/CI and matched in-game checks for rain/audio transitions, foliage cost, native/Alt Wolf, cane swing/spin reach, potion text and Saria actor presence. No MM Black Market performance fix is claimed.

## Ranch audio followup

Add a default-off **Disable Ranch Proximity Music** checkbox in Audio Editor -> Audio Options, with the native What's New accent. Enabling it bypasses Malon's distance-based mix only in Lon Lon Ranch, restores an existing main-track fade, releases only Malon's native sub-track, and uses the existing selected-scene night-music lifecycle for the full Ranch track at night. Preserve ambience/SFX and native cutscene, fanfare, combat, ocarina and preview priority. Other scenes and song-learning behavior remain unchanged. The new Ranch gate passes through the combined-ABI runner with ASan/UBSan, including the actual native queue plus Malon and scene-night updater across both callback orders and global-night settings. A live-OFF custom-bank ownership bug failed before the handoff repair and passes afterward. Existing night and both real-ImGui menu suites pass; the actual highlight renderer passes sanitizers; all three touched production translation units compile. Scoped source re-review found no remaining important issue. Full executable linking and audible gameplay remain untested.

Publication was blocked by automatic approval review; no candidate GitHub commit, draft PR or platform build was created for this pass. The source and optional HD archive are saved separately. Existing saved Saria-hint preferences remain unchanged, including disabled hints in an older seed.
