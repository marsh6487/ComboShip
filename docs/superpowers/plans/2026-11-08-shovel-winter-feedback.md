# Shovel and winter surface feedback implementation plan

Goal: One shovel press completes its dig; winter ground sounds snowy outdoors; frozen water sounds icy and produces no player foot ripples, including temple pools.

Baseline: hidden UI/night music checkpoint c5fd7b234509460c5a851f6d1c3eb113d86a1a04, matched against all 25 packaged source hashes. Work is an isolated copy on candidate/shovel-winter-20261108. Public reference 008534e3b1abcdfa8805a555de8d4639991b740f; bounded local history is not public ancestry.

Preserve: UI input/navigation and Debug Warp night music byte-for-byte; shovel impact timing, loot and progression; native grotto destinations and scene-exit types; collision geometry/waterboxes, swimming below water, existing boot/form overrides; other seasons and weather/audio ownership.

1. Shovel
- Reproduce first-use failure with production start/upper-action bodies and an equip animation replacing the pending upper animation.
- Delay OoT dig animation installation until Player_UpperAction_Shovel owns the upper skeleton, as MM already does.
- Use MM DoorAna type macros and bombCollider teardown; visible scene exits must not be rewritten by digging.
- Verify first use and subsequent use each impact exactly once, cancellation/missing animation, hidden grotto reveal idempotence and destination preservation.

2. Winter feedback
- Select snow step material only for dry, grounded outdoor scene terrain (ground, sand, concrete, dirt, grass), using each engine's existing eligible presentation season.
- Preserve footwear and transformation overrides. OoT uses the existing MM audio bridge for snow; if that optional audio is absent, use native sand as a non-silent fallback. Ice uses native SFX.
- During scene collision set the native no-entry-ripple flag for a winter surface walker before Actor_UpdateBgCheckInfo. Continue evaluating water collision.
- Resolve ice floor SFX after the current surface pin, even with no floor polygon. MM clears WATER/WATER_TOUCH on the frozen pin; guard its later wading branch too.
- Mirror OoT's saved owned winter gameplay gate in MM so temple freezing does not depend on outdoor weather presentation; preserve human/non-swimming/descent requirements.
- Verify snow/ice priority, null bottom polygon, landing and shoreline changes, water boot behavior outside winter, underwater/swimming, interiors and season changes.

3. Integration and recovery
- Run focused native fixtures with sanitizers and original/Combo guards, existing season/MM action and hidden UI checks.
- Compile changed full player translation units with native project guards where available; record actual blockers.
- Review complete diff independently, retain source recovery package with combined UI patch, checksums, installer and independent restore proof.
- Deliver source candidate, not a claimed rebuilt/runtime-accepted game. No remote publication or master promotion requested.
