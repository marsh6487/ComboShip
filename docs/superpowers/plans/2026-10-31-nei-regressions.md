# NEI item regression implementation plan and recovery record

Goal: correct item presentation/selection failures and support independent NEI D-pad assignment and equipment toggling in both games.

Spec: cor's October 31 request and screenshots image(20261031-121845).png / image(20261031-121900).png. The first shows an Ice Cavern Map entrance hint overflowing its box; the second labels MM Cane of Pacci (Lift) while drawing a blue rupee.

Baseline: marsh6487/ComboShip commit e1b34337c9d2075dd2663b5983f928439942add2, tree 1fa2abfa867b5b3745d6faf2de2ab7bdb0b66aae. This includes the separate spawn-recovery candidate and is not a runtime-accepted master. The user's running executable commit is unknown. Candidate branch fix/nei-item-regressions-20261031.

Architecture: retain each game's inventory/save and renderer ownership. Fix where full item identities, ownership or presentation metadata are lost. Use existing cross-game draw descriptors and native input dispatch, with per-game regression seams.

Tech stack: C/C++ game modules, Python native compilation harnesses, existing JSON/resource pipeline.

Preservation: accepted archives, progression/seed routing, native item swaps, transformation behavior, other UI/equipment features, original/combined-game guards and prior spawn changes. No accepted-master promotion or release publication. Runtime and Alt Assets verification remain separate from compilation.

Review focus:
- NEI D-pad writes preserve the entire C bank and its full-ID sidecar.
- Equipment IDs must not alias trade items, icon selectors or arrow animation sentinels.
- Shared-cell/empty/unowned selection publishes the currently owned identity only.
- Native/foreign GI paths avoid recursive exporter fallback and generic Mirror Shield shadowing.
- Receipt layout retains every word/color and fits the native pen, including long entrance labels.

## Workstreams

- [ ] Receipt typography (parent): execute exact Ice Cavern Map / Fire Temple Boss Door layout with real font widths; trace builder -> staging -> layout; fix confirmed failure only.
- [ ] GI routing (gi_routing): repair concrete MM canes and confirmed sibling omissions; isolate Ikana GI resources; test native/foreign and optional-asset controls.
- [ ] OoT controls (oot_controls): independent D-pad assignment, full IDs, all equipment toggles, held-item D-pad dispatch, empty selection and Pictograph names.
- [ ] MM controls (mm_controls): same contract with native MM slot/form semantics and full-ID/icon controls.
- [ ] Integration: review diffs; run focused/relevant existing suites, formatting and platform CI; preserve verified milestones and final independently restorable checkpoint.

## Verified milestone 1: MM binding and pause selection

Files: mm/mods/ext_buttons/ext_buttons.cpp; mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_equipment.c; mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_item.c; tests/nei_controls_mm/run_tests.py.

Production-body regression fails on baseline and passes after repair: independent NEI D-pad writes preserve C slots/full IDs, including Roc's Feather/Cape IDs overlapping arrow aliases; equipment accepts C/D directions including boots; Pictograph wheel updates selection/name immediately. Existing unowned equipment titles remain empty.

Owning-worker verification: tests/nei_controls_mm/run_tests.py --sanitize (ASan/UBSan, leak scanning disabled for container compatibility), four modes. Full icon/runtime audit and legacy Mirror test expectation update remain in progress. This is WIP, not in-game acceptance.

## WIP milestone 2: broader implementation preserved

Both control workers have passing production-boundary tests for independent D-pad assignment, all 12 equipment bindings/toggles, full-ID icon identity, stale/empty selection and native/shared-cell controls. Equipment bindings use per-game full IDs 0x0240..0x024B to distinguish trade/icon aliases without changing save format. Runtime all-family audit continues: spiritual stones, mask D offsets, duplicate update/cached-mask handling and alias-aware equipment status need final verification. Independent review confirmed the OoT status-updater omission; correction is in progress.

GI work has passing focused reproductions for missing MM cane export, sibling legacy fallbacks and dedicated Ikana roots plus a stock-only nested resource owner. The original procedural fallback cube is preserved for Dominion Rod/base Elemental Wand; an authored model still takes priority. Final GI matrix/asset parity verification continues.

Exact Ice Cavern Map / Fire Temple Boss Door text fits current native OoT/MM layout with every word retained. The screenshot overflow is not reproduced against this source. One-time receipt staging/reflow traces now report message ID, presentation mode, byte lengths, buffer position and scale; their no-per-frame-repeat regression passes. Full receipt suite first encountered a missing explicit OoT include path in the existing runner, now corrected; rerun remains pending.

This checkpoint is explicitly unfinished. GitHub publication was blocked by automatic approval review (new-pass source disclosure authorization not recognized); local commits and this recoverable archive are preserved. Full platform builds and actual game/Alt Assets acceptance remain unproven.

## Final implementation and verification verdict

All GI and input workstreams are implemented and reviewed. Source remains an unaccepted candidate; no executable, configured gameplay or Alt Assets acceptance is claimed. The first two local commits intentionally preserve earlier unfinished states; the final checkpoint restores temporary test-fixture edits after the MM compatibility headers were hydrated.

Native results:
- Complete OoT group: six runners, ASan/UBSan, solo/Combo boundaries; 74 binding identities; all 12 equipment pieces across seven inputs; native player toggle/retention and status updater; empty cells and correct Pictograph naming; all 24 custom-item handlers once per frame; stones, medallions, remains and Pikachu C/D input.
- Complete MM group: seven runners in Combo and Solo with ASan/UBSan; independent binding/full IDs, all equipment icons/toggles, owned/title/empty-cell rules, shared-cell names, claimed-input/status policies, progressive/virtual slot separation, duplicate cache union and Net deduplication.
- Review corrections include OoT alias status exemptions, MM native D Ultrahand guard, boundary cursor refresh and low-ID trade assignments. Cryonis owns only its up/down distance controls; C/left/right remain usable. Disabled duplicate bindings do not regain input through union masks.
- GI: actual MM six-skill cane exporter authored present/absent and stock/local/donor roots; canonical imported catalog fallback without sentinel recursion; 18 omitted OoT static recipes; eight native-equipment callback transports; independent Ikana roots/default stock graph and teardown; exact native placeholder geometry for Dominion/base Elemental Wand.
- Real resource manager/cache/loader under ASan/UBSan: root/vertex/texture/hash pinning, hot caches, independent Alt switches, metadata aliases, in-flight lifetime and stock-owner lifecycle pass.
- Receipt layout and traces pass with native fonts, including exact Fire Temple Boss Door screenshot text and no repeated frame logging. Full native/foreign receipt catalog passes. The full receipt umbrella later stops in unchanged run_seed_settings_tests.py because external imgui.h is absent; this is a dependency block, not a passing full-suite result.

The screenshot overflow itself remains unreproduced in this source, so it is traced rather than declared repaired. MM Pictograph wheel identity is repaired; OoT's pre-existing correct resolver/selector invalidation is verified without a speculative rewrite. Native placeholder cubes remain placeholders when authored assets are missing. The private Fados archive case is skipped because that input was not supplied.

CI metadata runs both complete control groups and the GI fallbacks/resource-owner checks using the existing pinned NEI dependency include path. Public GitHub upload remains blocked by automatic review pending explicit authorization for this new pass; full platform CI has not run for this candidate. No accepted source master or input archive was changed.

## Authorized publication and indexed-rendering follow-up

cor explicitly authorized publication of this source checkpoint, tracked GI assets, tests and CI changes on October 31, then requested indexed rendering be disabled by default with an opt-in checkbox. The NEI checkpoint was published as c043a85b26b8ce150c075bfb565119861fac7bc3 on fix/nei-item-regressions-20261031; its tree a40f9df5fafc79e87e6d9f284484b17d9772ca66 is byte-identical to local checkpoint 2d27f023. That resolves the earlier upload authorization block. No protected-branch merge or release is authorized by implication.

The renderer's gEnhancements.IndexedTriangles fallback is now 0, matching its false initial field. Both native Settings > Graphics > Advanced Graphics Options menus expose Indexed Triangle Rendering (Experimental), default false. Combo inherits OoT's shared Graphics model and writes the same runtime CVar store. Absent settings in fresh and existing configurations stay off; explicit saved 1 remains an opt-in. No preference clearing or migration is performed.

Production-function tests cover native/shared checkbox writes through the actual CVar bridge, both interpreter instances, default off, explicit on/off, unsupported backend and safe batch flush transitions. DX11/OpenGL native indexed/legacy submission and caustic binding regression checks pass with sanitization. Full application/GPU execution remains unverified.

cor reports roughly 10 FPS in Kokiri, and separately reports poor performance persisting after removing two twin placements in the Link's House hub. Indexed submission did not improve that observed workload. Source inspection confirms CPU vertex transforms and lighting precede indexed submission; the dominant cost in the current reported scene is still unproven. This setting change is not represented as an NPC performance repair.

## CI correction and boss-soul receipt checkpoint

The October 31 public run 41206154247 passed fast source checks, entrance regressions and Windows compile preflight but failed the five remaining regression groups and both full MM builds. Linux identified a missing declaration before the MM cane consumer: ResourceMgr_IsModAssetForGame is now declared with its actual plain extern-C linkage. The cane fixture now preserves that declaration-before-definition boundary and removes complete borrowed stubs independent of formatting.

Rod lifecycle, seasons and MM use fixtures now include the real new input/equipment/save dependencies. The seasons assertion follows the requested independent NEI D-pad contract, preserves the entire C bank, and still checks native-item swap behavior and the displaced full Rod identity. All 23 related checks passed with actual pinned dependency headers. Native decoder/market fixtures provide the diagnostic logger boundary without replacing receipt/message behavior or weakening assertions.

The boss-soul trace found a confirmed exported-receipt format mismatch: MM's generic dungeon-map skull is RGBA16 16x16 in both supported XML variants, while MM_GetSeedItemIconInfo advertised RGBA32 32x32. Receipt selection now uses the established Captain skull RGBA32 32x32 substitute, including Majora, shared with the native receipt selector. Map/toast texture selection and GI artwork remain unchanged. Both engines' one-time ReceiptTrace stage record now includes dimensions and format. This proves and repairs a source corruption route; it does not claim reproduction of the user's unseen exact acquisition screenshot.

New ASan/UBSan regression checks all 14 boss souls through native and exported receipt rendering for both games, actual texture metadata, dimensions, formats and resource ownership. Existing layout, catalog and soul-color checks pass. Actual MM ItemReceiptText.cpp and Items.cpp pass real-header syntax checks (existing warnings retained). The new soul test is registered in the items CI group. Configuration-specific runtime acceptance and fresh full platform CI remain pending publication.

## Second CI failure pass: loader linkage and fixture dependencies

Public c3eb0fc3 / run41210857334 clears the earlier compile error and passes the complete world/dialogue and entrance groups, but is not build-verified. Linux compiles the modules then seed smoke testing fails while loading lib2ship.so: undefined Rando::FrameInterpolation_RecordCloseChild. Four regression groups reach later fixture failures: duplicate CVar/Dpad mask declarations in equipment preview, missing actual button accessor dependencies in wand modes, stale handwritten MM item IDs/cane/Ikana dependencies in dungeon-key cache extraction, and missing native gItemIcons storage in the key receipt full-TU link.

The loader defect is caused by the Ikana fallback's DISPS macros declaring interpolation functions at block scope within the qualified Rando::DrawResolvedItem body. This creates namespaced C++ references instead of the engine's global C symbols. A native regression using the actual MM DISPS helper macros and namespace-qualified caller reproduced both unresolved interpolation symbols before repair. The unchanged native drawing block now lives in a global static DrawNativeIkanaFallback helper; authored GI selection, transforms, both passes and exact Solo/Combo resource guards are preserved. Token comparison of the moved block passes. Native sanitized Ikana integration/link regression and actual full DrawItem.cpp real-header syntax pass; module load/seed generation still require fresh CI proof.

Fixture repairs preserve existing behavior assertions. Preview reuses inherited CVar/Dpad definitions and distinct shoulder masks; Wand extracts actual input/accessor/alias helpers. Dungeon keys use the production enum/helper and fail-loud boundaries for unrelated resource families. Key receipts generate the actual 131-entry native icon table and verify the aligned Captain texture pointer. Each previously failing fixture was reproduced before repair and passes native/sanitized tests afterward. Configured-cxx dungeon-key and downstream Gfx checks pass. Downstream controls, receipt catalog/souls, pause/tutorial and air-magic checks pass. Optional local quest-held geometry verification remains blocked by sparse asset-authoring source, rather than treated as a passing result.

This is a correction candidate pending fresh full CI; no source/master acceptance or in-game success is inferred from the native results. Indexed rendering remains default-off and the previous soul icon repair is retained.

## Remaining render-group failure: native age-button fixture

Public b83f1cd1 / run41213564230 passes the full Linux build/package/seed smoke checks and the equipment, systems, items, world and entrance groups. Windows remains in progress. The render group reaches the cross-game button age fixture, whose real OoT ext_buttons.cpp syntax check lacks the soh/mods include root for items/helpers/equip_helper.h. Its extracted accessor TU also needs the real claim/equipment-alias bodies and the new typed world-mode service boundaries.

The correction is confined to tests/combo_button_transfer/run_age_tests.py and age_shadow_test.cpp: add the native include root and helper headers, extract actual dependency bodies, supply inactive world-mode/ownership boundaries, and match the native u8 age-validator signature. All original age/save/HUD assertions remain. Both native syntax checks and all eight Solo/Combo age modes pass ASan/UBSan; the complete sanitized native/cross-game transfer runner passes. A CI-configured compiler verification and downstream rendering checks are being collected before publication. No renderer implementation, production controls, assets or indexed-default setting changes in this pass.
