# Scene regression pass implementation plan

Goal: repair the independently traced regressions while preserving accepted scenes, actors, item grants and source baseline.

Baseline: captured build `93309d745b260fa1eba1a55d658db717ee350d0f`, candidate branch `poc/scene-regression-pass-20261107`. Touched original sources are preserved in the durable regression baseline artifact; exact HD Wolf bytes are separately preserved in `wolf-link-hd.o2r` before removal from built-in assets.

Execution: user explicitly requested parallel workflow. Six independent workers own disjoint files; the controller owns staging, commits, recovery packages, integration and final review. No accepted master promotion or runtime-pass claims based on host tests.

## Binding decisions

- Retain authored/native weather actors. Preserve seasonal cycle and audible continuity across compatible same-scene room transitions, tested with and without placed owners. Trace persistent silent rain without claiming an unobserved event fixed.
- Keep Saria's hint available with Magic Stat Upgrades. Prefer earliest full logic-reachable sphere with stable OoT-first/check-name ties, not a changing live-inventory target. Describe this as early reachable, not uniquely easiest. Preserve song reward progression; hint-only conversation can precede Letter in child randomizer with hint enabled. Do not silently modify existing saved hint settings.
- Native potion tutorial and native proper item name must appear without a redundant item-name-only page. Preserve localization, buttons, icons and grants.
- Standard Wolf remains built in. HD resource is external, byte-preserved; checkbox disabled when selected owner's HD resource is unavailable. Repair vertex offset contract for both models and test under combined s32 ABI.
- Cane damage bounds follow visible shaft plus modest game-space padding, independently of visual mesh scale. Preserve native other weapon collision and current cane behavior.
- Diagnose scene-dependent performance. Do not infer an accepted asset optimization is loaded from the NativeNpcV1 resource prefix. An unnamed end hook remains unnamed until measured. A repeated texture no-change scan can be fixed only with a test proving it and appropriate invalidation.

## Tasks and ownership

1. Performance: inspect actor/phase/renderer evidence; bound any foliage negative-cache change to identical presentation and correct cache invalidation. Test no-change repeat work and source/season changes.
2. Rain: actual production cycle, resolver and mixer tests; reproduce door discontinuity before repair, then Spring/Autumn persistent/intermittent with absent/native/authored owners, pause and exits.
3. Saria: inspect saved Skip Zelda/Letter configuration; support stat-upgrade generation hint selection and child hint-only actor mode without early song grant. Test stable selection, missing metadata and normal progression.
4. Potion: use realistic short native potion text in actual foreign receipt fixture; reproduce redundant page, remove only redundant policy and verify other bottle/native localization paths.
5. Wolf: preserve external archive, remove bundled HD resources/package references, gate UI availability per owner and fix vertex address stride; test standard fallback, archive presence/absence and s32 traversal.
6. Cane: separate cane geometry frame from oversized native sword padding; test transformed endpoints/quads and other-weapon invariants.

Each worker records fail-before/pass-after commands and limits. Controller inspects every diff, checkpoints bounded milestones durably, registers focused tests in the common test manifest if appropriate, runs combined checks and commissions a fresh review. Deliver source candidate, external HD archive and concise runtime test list. Any unavailable native/game runtime remains untested.

## Progress ledger

- Baseline sources verified against exact git blobs and saved before implementation. Original uploads untouched.
- HD Wolf external archive saved and verified before bundled deletion authorization.
- Workers dispatched: performance, rain, saria, potion, wolf, cane.
- Ruling: retain user-authorized parallel execution despite generic sequential implementation guidance; disjoint ownership and controller-only commits prevent index conflicts.

- Rain, performance, potion, Wolf, cane and Saria task reviews approved; Saria post-victory and same-seed-import findings corrected and scoped re-review approved.
- Four new gates pass via combined-ABI adapters; focused production suites and final source syntax/format checks pass. Whole-candidate review found no Important/Critical blockers.
- Source candidate only: configured gameplay and full platform CI remain separate. New OoT capture does not include MM Black Market; no MM stutter repair is claimed.

### User-requested followup: Ranch music

- Implement default-off `gAudioEditor.DisableRanchProximityMusic` in Audio Options with the existing What's New accent.
- Restrict native proximity suppression to Lon Lon Ranch; keep configured track at night via existing controller, preserving ambience and higher-priority native audio.
- Test option off/on, live toggles, both proximity entry points, selected/custom track ownership, night lifecycle, non-Ranch controls, preview/fanfare/combat/cutscene recovery, menu default and accent. Compile touched production units and review the bounded followup before checkpointing.

- Ranch followup implemented and scoped review approved after real queued custom-night OFF-handoff failure was repaired. Combined-ABI gate, native translation units, night/music menu/highlight suites pass. 295 checks registered. Publication remains blocked pending explicit authorization accepted by automatic review.
