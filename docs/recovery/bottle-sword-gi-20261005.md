# Bottle and sword GI candidate, investigation checkpoint

Base: `9eff802cbf678047f72bfaa28ea00f999780857b`, isolated branch `poc/bottle-sword-gi-20261005`.
Public PR34 accepted baseline reported by parent: `1147a1a8`; base includes parent Grace/hourglass and Slate work.

Scope: fairy contents and TP shell selection, upright sword GI fitting, designated sword particles and intrinsic Din effects under selected mesh routes. Preserve custom mesh priority, bottle motion and shimmer, native fallbacks, held/player geometry, songs and unrelated inventory/text work.

Direct evidence: inspected three supplied clip frames for fairy, Master Sword and Razor Sword. Empty bright bottle; sword meshes upright and near upper screen edge; designated blue shimmer visible. Mod archive inspection reports no dedicated GI replacements and +X equipment blades spanning up to 6172 units.

Confirmed code boundary: `CwAltSwordGi` returns fixed `.04` hand-equipment scale before authored fitting. Native selected sword draws use the same fixed scale. `NeiGi_DrawEffects` and external presentation suppress intrinsic sword effects whenever geometry is selected from mods. MM native legacy-mesh fallback attaches only shimmer.

Further investigation: fairy draw uses active-host skeleton/animation, explicit standard limb count checks and small `.004` scale. Shell selector favors fairy-specific override before selected BlueFire shell. Exact TP archive/config not available yet; tracing selected resource behavior before choosing fix.

Status: investigation only, no implementation or build-accepted claim. This checkpoint records recovery state and will be integrated/published by the parent; this agent is not authorized to publish.

Next: failing execution-level regressions for selected mod route fitting/effects and bottle composition, then narrow fixes and native/foreign syntax checks. Game runtime remains unavailable.

## Fairy candidate checkpoint

Failing behavior regression reproduced: an actor fairy drawn after a scaled MM native contents matrix shrinks again by the `.004` actor scale. The controlled matrix includes `.01` sprite scaling; that is a regression fixture, not a measurement of an unsupplied native archive. Native MM and its foreign OoT consumer now keep only the native matrix translation for actor VFX and retain the complete matrix for original contents fallback.

Second failing behavior regression reproduced: a TP XML BlueFire shell in a built-in/companion O2R was rejected solely by non-mod archive provenance. New owner-aware query recognizes XML replacement geometry as well as selected mod archives. Vanilla binary chamberstick remains ineligible. Fairy-specific and generic mod priority remains intact.

`python3 -B scripts/diagnostics/run_fairy_bottle_tests.py`: passes both host variants and both real C++ helper header gates with provided CPATH. Native/foreign transform and fallback checks passed. `run_mm_item_visuals_tests.py` is running; exact result will follow. No game runtime validation or exact TP stack proof.

## Selected sword fitting and particles WIP

New graph reader obtains bounds from the currently selected Fast display list and vertex resources, recursively following XML filepath and binary hash calls, then transforms +X equipment bounds by the retained Z=1.8 upright pose. Full-spin radius and vertical bounds feed the existing `FrameFit` envelope. Owner-aware lookup and live Alt selection remain; no player/held vertices change. Static unsupported/dynamic matrix paths leave their geometry draw intact.

Focused red tests: `run_sword_pose_tests.py` failed on selected Din equipment top >48 GI units; `run_nei_identity_tests.py` failed when native MM selected sword fallback emitted shimmer without intrinsic particles. Both now pass their controlled execution checks. `run_sword_mod_gi_tests.py --archives /workspace/scratch/b1753232b6d5/upload` is executing actual supplied resource graphs and model routes. Native/foreign syntax/integration gates and intrinsic Din fire-layer restoration remain unfinished. This is a WIP checkpoint, not a build-accepted candidate.

Fairy gate update: `run_mm_item_visuals_tests.py` completed successfully (native MM command checks, real z_draw C syntax, real item-enum dispatcher). Existing controller macro redefinition warnings remain unchanged.

## Sword execution integration checkpoint

`run_nei_gi_tests.py --combo` now passes the actual native renderer and owner descriptor, native MM selector/fallback, both foreign dispatchers, common/acquisition/shop routes, mandatory sword intrinsic particles plus exactly one designated shimmer, base/Alt model priority and arena guards. Fixtures now distinguish sword particles from authored non-sword energy; custom sword meshes always keep their awarded identity. Actual supplied O2R graph test passed all 14 selected meshes across 360-frame pickup/shop rotations. `run_sword_fallback_tests.py` passes the actual OoT foreign custom sword stream and matrix execution fixture. Existing C qualifier/engine macro warnings remain.

This checkpoint confirms host execution/build gates, not the user's runtime configuration. Intrinsic Din core/flame layers still need a GI hook, and native MM's selected equipment route still needs explicit handling. Both remain active work; no runtime acceptance is claimed.

## Native MM selected-sword checkpoint

Added a failing actual-MM selector test: seven concrete awards discarded the selected standalone owner sword when its authored GI descriptor declined. Native MM now resolves the owner's existing selected-sword producer, carries its upright Z-only pose, and retains the native award's intrinsic particle/shimmer identity. Shared external sword drawing now applies selected graph fit before both mesh and particle passes; optional True Master flame is preserved. Live owner Alt/resource selection remains in the existing producer.

Fresh `run_nei_gi_tests.py --combo` and `run_nei_identity_tests.py` pass after this change, including native MM's seven selected-sword awards and existing custom/legacy priority fixtures. Resource-loader matrix fitting and exact user runtime still need final verification; Din physical layers remain active work.

## Din layer integration checkpoint

A failing actual foreign draw test reproduced the selected Din blade missing its CoreDL/FlameDL layers. The GI hook now queues those existing resources in the selected blade's fitted/upright matrix, with owner scopes around both geometry/hash references and textures. The existing hot core/flame combine modes, I8 tiles, scroll and alpha animation are retained. Complete private dependencies and Din feature/owner Alt eligibility are required; missing layers leave the selected blade and designated particles/shimmer intact. Selected fitting now also reads real core/flame bounds when active.

Fresh `run_sword_fallback_tests.py` passes the actual OoT custom foreign stream and model/core/flame pose check. Fresh `run_nei_gi_tests.py --combo` passes native OoT and MM complete/missing Din dependencies plus all previous selected/mod/particle cases. The real resource-helper header syntax gate passes. Expanded actual-archive layer bounds and foreign MM execution checks remain to be completed; no full game runtime is available.

## Shared fitted pose and actual Din graph checkpoint

A new failing foreign-stream test showed sword particles still using the old unfitted pose. Native OoT and both foreign custom sword dispatchers now apply the fit outside model-only transforms, so model, intrinsic Din layers, designated sword particles and shimmer share the correction; Z-only upright orientation remains inside the model transform. Shared external/native MM already uses this same composition.

Fresh `run_sword_fallback_tests.py` now executes both actual foreign custom handlers/dispatchers and passes model/core/flame matrices plus intrinsic/shimmer fit. The fixture uses real OoT GBI layouts for both command streams and controlled API aliases for MM; separate actual-MM-header C++ syntax passed `NeiGiPresentation.cpp`. Actual supplied archive graph test now includes real CoreDL/FlameDL geometry in the existing pickup/shop full-spin envelope for all 14 meshes with Din off/on. Shared production fit helper is executed rather than a duplicated bounds formula. Unsupported depth-dependent branches are rejected from static fit instead of reading branch payload as commands.

Fresh broad native GI execution and C syntax gates pass after the shared-pose change. Fairy regression fixtures are being refreshed for the updated foreign dispatcher signature. Remaining work is final sanitizer/real-header refresh and recovery handoff; runtime acceptance remains pending the user's exact configuration.

## Native MM API and custom GI priority checkpoint

The combined native MM DrawItem syntax gate exposed the Din helper's OoT-only `Matrix_NewMtx`. The MM branch now calls its actual `Matrix_Finalize` API. The earlier `NeiGiPresentation.cpp` syntax check used compatibility aliases, so it did not prove this include boundary. Fresh actual `mm/2s2h/Rando/DrawItem.cpp` C++20 syntax passes with the target's real precompiled-header includes and no matrix compatibility aliases. The focused sword pose script now preserves that full translation-unit gate, alongside both actual helper/header gates.

A new failing native MM selector test also reproduced a dedicated Razor/Gilded/Great Fairy GI replacement losing priority to selected OoT standalone equipment. Native MM's legacy GI selector now recognizes all four native sword families, including both split parts. Fresh `run_nei_gi_tests.py --combo` passes the red-to-green priority regression and previous native/foreign/arena gates. Fairy regression refresh passed both host variants and actual helper headers. Both actual foreign custom handlers pass address/undefined sanitizer execution. The actual archive graph sanitizer run is in progress; no final runtime acceptance is claimed.

## Selected True Master flame review checkpoint

Review reproduced the native MM selected True Master Sword's optional blue flame allocating a 192-byte texture scroll before any arena guard. The tracked `tests/nei_gi/run_flame_arena_tests.py` reproduced the real OPA head/tail crossing, then passed after the combined flame/model budget check. Invalid scale and non-finite pose inputs now skip before that native allocation. The MM selected branch applies fit outside both the native flame and shared presentation, with the inner renderer explicitly keeping that existing fit. Pickup/shop execution checks compare actual flame, blade, intrinsic particle and shimmer matrices, and both empty/short OPA, XLU and overlay arenas remain untouched.

Fresh broad `run_nei_gi_tests.py --combo --sanitize` passes the shared renderer/native/foreign selectors and real C boundaries. Fresh sword helper/actual MM DrawItem C++ syntax gates pass after the shared API change. The actual supplied archive graph sanitizer run completed all 14 meshes, 360-frame pickup/shop full spins and active Din layer unions. The fixture retains actual membership only for queried Din dependencies/pack markers; compiling unrelated player textures previously made sanitizer setup unnecessarily slow. No mod bytes are committed and no user-configuration runtime acceptance is claimed.

## Final focused verification checkpoint

The Din GI material hook now uses each host's existing native cosmetics route: OoT `.Value` color CVars and MM's changed-color/suppression-aware editor API. A failing execution test reproduced MM reading the OoT CVar path. Both actual foreign handler streams now pass sanitizer checks for native color queries and the resulting core/flame primitive/environment commands, alongside owner scopes and shared blade/layer/particle matrices. Both actual MM production translation units (`DrawItem.cpp`, `NeiGiPresentation.cpp`) pass the permanent C++ syntax gate using the target's real precompiled-header includes.

Fresh focused commands, all successful with the documented dependency CPATH:

- `python3 -B scripts/diagnostics/run_fairy_bottle_tests.py`
- `python3 -B scripts/diagnostics/run_mm_item_visuals_tests.py`
- `python3 -B scripts/diagnostics/run_nei_identity_tests.py`
- `python3 -B scripts/diagnostics/run_sword_pose_tests.py`
- `python3 -B scripts/diagnostics/run_sword_mod_gi_tests.py --archives /workspace/scratch/b1753232b6d5/upload --sanitize`
- `python3 -B scripts/diagnostics/run_sword_fallback_tests.py --sanitize`
- `python3 -B tests/nei_gi/run_flame_arena_tests.py` (always uses address/undefined sanitizers)
- `python3 -B scripts/diagnostics/run_nei_gi_tests.py --combo --sanitize`

These gates execute the native OoT renderer, native MM selector/draw body, both real foreign handlers/dispatchers, actual supplied mod resource graphs and real host headers. They establish implementation/command-stream behavior, not a full Windows build or the user's installed texture/Alt/load-order configuration. The exact TP archive/stack is still unavailable; shell recognition covers selected mod geometry and custom XML resources imported by companion/base archives. Static fitting intentionally declines display lists with unsupported embedded/dynamic matrices or depth-dependent branches, leaving selected geometry intact. No full game runtime or final user acceptance is claimed.
