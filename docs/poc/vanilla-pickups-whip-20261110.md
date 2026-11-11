# Vanilla pickups and Whip checkpoint

Baseline: `a5b1abd697d284ecbca70c13091562821dfc3feb`, tree `da99bc7dd7191661e4dae8f829b2a98d2f22602b`.

Candidate: `candidate/vanilla-pickups-whip-20261110`.

Status: the pickup/Whip source checkpoint and the later MM forest companion scheduling repair are implemented and independently reviewed. Seven initial focused CI groups and two additional forest groups pass. The MM randomizer trace now also exercises its authentic starting-check identities and initialization; its updated routed CI group passes. No configured gameplay fix is yet claimed.

Evidence: user reports successful MM Hourglass and Whip receipts, first OoT Rod interaction displays Pocket Egg tutorial, later Yes does not collect. cor explicitly confirmed that the Egg symptom was tutorial dialogue only: no Egg entered inventory. Submitted log confirms first `0x8EE0` followed by native item-icon ID `0x2D`; that log line loads an icon. Running build identifies `e8d4bd8`.

Findings: OoT `Message_StartTextbox` invokes text hooks before assigning `msgCtx.talkActor`; the reward hook currently depends on that stale pointer. MM Whip uses native bow aim while the generic reticle renderer requires a legacy aim timer that this path never starts. Extension lifetime, not the unused range macro alone, determines reach.

Decisions: preserve receipt/save contracts and all placements except the later-authorized Rod relocation to Lost Woods room 0's center. Double Whip reach to 1040 units while retaining 136-unit swing length and current grapple eligibility. Independent source ownership permits reward and Whip work in parallel. No source publication or master promotion is part of this checkpoint.

Validation: nine distinct focused routed CI groups passed across the initial and forest runs; independent review has no unresolved material findings for the pickup/Whip and NPC repairs. Source checkpoints: Whip `330a7df66`, OoT `aa304e76e`, initial MM diagnostic tests `51d91057b`, forest companions `0d844d718`, authentic-start diagnostics `419659c48`. The recovery manifest records the final commit/tree and includes the later NPC and authentic-start diagnostics.

## Reproduction and interpretation

The four added OoT native simulations fail against the unmodified adapter for four separate reasons: the first open selects stale-owner / Pocket Egg text; accepted NPC talk is discarded while native textbox state is still NONE; an unowned reserved custom text ID falls into the stock table; and Yes starts no receipt. These are assertion failures after successful native fixture compilation, not dependency failures.

Native ordering explains both reported symptoms. `Message_StartTextbox` calls `Message_OpenText` and its hook before assigning `msgCtx.talkActor`. `Player_StartTalking` records the player talk owner and accepted-talk flag before the put-away action eventually opens the textbox. The reward actor must retain that pending conversation and handle choices only for its own current offer.

The Whip simulations also fail against baseline for the absent native-aim reticle, an eligible target beyond 520 units, and the old fallback rope-rendering density cap. Candidate tests exercise the real native aiming/equip, reticle dispatch/renderer, tip extension, surface classifier, combat helpers, and fallback snake-body renderer in MM and OoT, with Combo and standalone guards. Services such as collision queries and drawing are controlled fixture boundaries.

The marker and lash now have the same 1040-unit distance budget, but retain different native origins: the marker starts at player focus and the tip starts at the hand. They are not proof of identical endpoints, nor is the red marker an attachability indicator. Z-targeting retains native homing behavior. The exact Woodfall platform and poison-water crossing still require a configured gameplay check.

Strict C translation-unit checks with the combined graphics ABI pass for both changed `object_whip.c` files in Combo and standalone modes. Both baseline and candidate `camera_helper.c` have the same pre-existing strict-GCC errors (implicit `Player_ZeroSpeedXZ`; MM reticle vertex pointer conversion; OoT `sqrtf` declaration). MM Whip fixture uses the existing test-only `linearVelocity` to `speedXZ` compatibility alias. The fixture result is not a full MM Whip translation-unit or application build result.

## Later placement and latency request

cor explicitly authorized moving the Rod to the center of Lost Woods room 0 and reported delayed visual appearance, plus a similar first-check delay after the MM randomizer Clock Tower start. These observations extend the original scope; the earlier no-location-change constraint no longer applies to the Rod.

The exact named `Lost_Woods_New_Grass_POC6.o2r` in the user's mount log was inspected read-only. SHA-256: `631e2ee5a00ceb1b2e8577638263287da7050c67a70fec1ec9d364b96da674ac`. Its native and alternate `spot10` collision resources are identical (`020e061956afedbf0350b92072e5e423cc9513c86a6277a51f723c5d18087a4c`). Upward-facing floor polygons 194 and 195 span x/z ±200 at y 0; the center probe returns floor y 0. The source position can therefore prefer x/z 0 and its existing 22-unit floor hover, retaining runtime collision, water, exit and wall checks. The archive itself is not modified or included in source recovery.

OoT's spawn adapter has a proven modulo-16 poll after safety gates, adding 0–15 gameplay frames before a ready drop exists. New native-layout tests reproduce this avoidable delay and the old entry-relative Rod position. MM Vanilla has no such poll: its visual spawn shares strict interaction/timer gates. MM randomizer has a different serialized queue and an explicit native get-item animation counter hold. Those mechanisms must not be conflated with cold asset loading; the submitted log lacks eligibility/spawn/draw milestones to establish a cold-load cause.

## Implemented behavior and verification scope

The OoT text hook resolves the current live NPC from `Player::talkActor`, consumes its three reserved IDs safely even without a valid owner, and keeps an accepted conversation alive through native put-away. A choice is accepted only when the player owner, message owner and offer ID all agree. Receipt identities, atomic persistence and grant contracts are retained.

The native pickup fixture now has 11 cases. Three schedules traverse real talk startup, Yes, native talk closure, safe give-item offer, native A-button acceptance/get-item action/consumer, the actual randomizer fixed-receipt dispatch, `Seasons_GrantRod` and the extended owned-item slot, then the host receipt commit. Controls include No/reentry, replaced CHOICE ownership, cancellation, stale/repeated callbacks, begin failure, commit rollback and retry. Inventory, trade slots, quest flags, season ownership and unrelated NEI fields are preserved. The manifest sweep covers 38 optional OoT offers through first-open, No, reentry and exact Yes staging; the 39th OoT row retains Roc's Feather's native source. Only the Rod case executes the full native grant rather than manifest-specific grant service boundaries.

That sweep reproduced and repaired a missing Shadow Crystal presentation mapping. Its full-width item ID `0x0222` has native `RG_SHADOW_CRYSTAL` GI metadata but no player-action registry row. A direct mapping prevents reward initialization from rejecting it. The native inventory grant semantics are unchanged.

The Rod anchor now requires Lost Woods room 0, works independently of scene-entry route and prefers the measured center before nearby safe alternatives. The first safe spawn pass and each new room-arrival pass run without the modulo wait; later scans retain the 16-frame cadence. Native placement tests cover all 16 phase offsets, late floor readiness, room arrival, duplicate suppression and 16 activity/transition/receipt/ownership/collision controls, plus preservation of other reward anchors.

Independent review caught a zero-length wall ray in the first center-placement draft. The revised preferred-center path uses two nonzero 40-unit orthogonal spans at floor + 30, checking both polygon faces. The original nonpreferred approach checks remain intact. The regression links real `sys_math3d.c` and executes `CollisionPoly_LineVsPoly` and native vector/normal/vertex helpers for X/Z walls and both normal signs; it failed before the correction and passes after it. These are bounded wall spans, not a full sphere-volume clearance proof.

The MM randomizer diagnostic fixture executes actual queue/reset, cutscene selection, event processing, native blocking-cutscene predicate and custom-item spawn/init/update/touch bodies in standalone and Combo configurations. Once message/cutscene/transition blockers clear, the event spawns immediately. Controlled parent acceptance starts counter 0, and counter 20 invokes the grant and makes the presentation drawable. Two checks serialize; interrupted ungranted actors requeue, granted actors do not duplicate, and the existing overhead path grants on its first eligible update. The simulation's printed frame numbers are controlled coordinates, not measured startup latency; drawability means nonzero actor scale, not GPU rendering.

The existing MM **Skip Item Get Cutscene** setting offers Never (default), Junk Items Only, Everything But Major and Always. Its native selector chooses overhead presentation according to item type; Always bypasses the item-animation hold for all types. It does not bypass message, transition or blocking-cutscene gates. No unintended first-load delay was reproduced, so no MM randomizer production timing was changed.

## MM forest companion appearance

The fallback Saria and Epona controllers both formerly required transition completion and `Player_InCsMode` to clear. They were registered on `OnGameStateUpdate`, which native `GameState_Update` dispatches after `Play_Main`, its update/draw pair and draw completion. An incoming fade can therefore reveal the room while both visual actors remain uncreated; the eventual creation also misses that frame's scene draw. The baseline `ComboSaria.cpp` Git blob `b893869aaba4c619e874c3f4325c87305e2e71e9` matches the same file fetched from the user's logged build `e8d4bd8`.

The narrow repair registers exploration companions on `OnGameStateMainStart` and permits their creation during a known incoming transition or fully idle transition state. It retains the live running Play context, exploration eligibility, loaded room 0, pause/script/pending-cutscene/soaring guards, resource and floor checks, authored actor precedence and duplicate prevention. Explicit player existence, death and game-over guards remain. Outgoing and unknown transition triggers are rejected. Termina Field's song-giving Saria keeps the original strict control-ready checks and post-draw hook; native companion interaction and rendering code is unchanged.

Both native actor profiles use persistent `GAMEPLAY_KEEP`; the shared object loader's delayed readiness rule does not account for this demonstrated wait. Saria's resource bridge is synchronous, and the special forest horse initialization takes its direct stationary setup path. These source findings do not measure cold resource duration or actual GPU presentation.

The new timing regression executes real native hook registration/dispatch, `GameState_Update`, `Play_Main`, incoming fade logic and player cutscene predicates. Against the old source, a controlled fade reveals the scene on frame 2, spawns companions on frame 22 and first includes their actor entries at the draw boundary on frame 23. Holding native entrance input through frame 25 moves those last milestones to frames 26/27. The repair creates them before the first draw, on frame 1 in those schedules. These are fixture coordinates; actor allocation and drawing are controlled boundaries, so they are not measured visible frames from the game.

Four arrival cases cover combined-game and solo-MM runtime modes, with and without the entrance lock. The suite also covers idle creation, the room becoming ready during arrival, 25 rejected contexts in idle and incoming states, authored actors/reentry, resource/floor/allocation failures and unchanged Termina Field timing. Both runtime modes are compiled within `COMBO_BUILD`; this is not proof of a standalone non-Combo application build. ASan/UBSan through the combined graphics ABI, the existing native forest placement test and existing MM Saria resource/ABI tests pass. Independent review reports no material findings for production SHA-256 `a514fc6fb7ff7357d16c857fe0d3b0f23d312cddaaeefee2f61ca976e3d511ce`.

## Clock Town starting-check trace

The normal randomized-save producer marks `RC_STARTING_ITEM_DEKU_MASK` and `RC_STARTING_ITEM_SONG_OF_HEALING` eligible immediately after successful seed generation or spoiler application; Combo's headless save builder mirrors those assignments. The actual items awarded depend on the seed. Configured starting inventory uses a separate direct grant path. These two checks do not wait for a Clock Tower pickup actor to discover them.

`PrepareNewRandomizedSave` establishes Human Link at South Clock Town entrance 0 with cutscene index 0, reset count 1, Tatl's first-arrival bits and the Happy Mask Salesman tower switch. `CheckQueue` serializes eligible checks, and `ProcessEvents` waits for the incoming transition, native player blocking state, messages and any current event. The native arrival helpers can set `PLAYER_STATE1_20000000`; this independently blocks `Player_InBlockingCsMode` even after transition state clears. Mode D initializes a -20 counter, E uses -15 and F derives a value capped at -20. The native movement controller advances its negative counter before its completion/stop-cutscene path. These entrance counters are separate from the item's later twenty-update receipt hold.

The extended diagnostic executes the real save initializer, the two production eligibility assignments, actual starting-check IDs, native mode-D helper and existing queue/event/item functions. Both checks are eligible while only the first is queued. Incoming transition and the arrival player flag independently prevent spawning; clearing them permits spawning on the first subsequent queue-processing call. Controlled awards are green rupees, not inferred items from the user's seed. Standalone and Combo sanitizer variants pass through the combined graphics ABI.

The fixture does not execute the complete arrival movement/camera controller, native parent acceptance, seed generation or the user's mounted South Clock Town player-spawn data. No retained scene archive/header established the actual start mode selected by this particular entrance/respawn. We can identify and reproduce concrete delivery gates and the separate receipt hold, but cannot attribute the full observed duration or claim a new first-load-only defect. MM randomizer production source remains unchanged.

## Evidence boundaries and remaining configured checks

- Core native simulation controls animation/draw completion, external hook side effects and persistence failures. It does not run the whole libsm64/Mario update loop. C-only `shift-base` sanitizer checking is disabled for existing signed `(1 << 31)` header flags; AddressSanitizer and remaining UndefinedBehaviorSanitizer checks are enabled. Leak checking is disabled for the executor's ptrace environment.
- MM timing controls native parent acceptance and message closure. Final inventory/save execution, full arrival movement and foreign delivery are outside that timing fixture; it records local grant callbacks and check consumption. Starting-check IDs and initialization now come from production. No frame-rate or cold resource-loading conclusion is made.
- Forest timing proves creation before the native draw boundary using controlled allocation and rendering. Native actor initialization, culling, skeleton drawing, archive precedence and actual first-visible pixels remain configured runtime checks.
- Full application linking and a new executable were not produced in this environment. Strict C baseline comparison and native fixture compilation are separate from application build proof.
- Remaining runtime acceptance: rebuild the exact candidate, confirm the centered Rod's first prompt, decline/retry, collect, scene reentry and save/reload; verify Whip reticle visibility and the specific Woodfall crossing with the configured assets; observe Saria/Epona on the first revealed Lost Woods frames; if MM rando startup still pauses with the chosen skip setting, capture transition/player state, queue eligibility/spawn, parent acceptance and receipt milestones to isolate the wait.

## Work ownership and preflight

| Task | Ownership / interface | Check |
| --- | --- | --- |
| OoT pickup | OoT reward adapter and native flow fixtures; consumes existing reward/receipt interfaces | Preserves stable IDs, save transaction, native trade data; native dialogue-order regression required |
| Whip | Whip state/render/reticle files and dedicated fixtures | 1040 reach, 136 fixed swing; native readiness plus unchanged legacy consumers |
| Integration | CI manifest, docs, checkpoint package | Consumes test runners only; no overlapping production files |
| Pickup / Whip | Separate native files and separate test directories | No shared mutable implementation interface |

Baseline test setup: initial OoT fixture attempt could not find nlohmann/json.hpp. Reusing the existing dependency header via CPLUS_INCLUDE_PATH resolved it; the unmodified OoT reward suite passes, demonstrating its prior coverage does not include the reported native conversation startup.

Build environment: this executor has gcc/g++14, but no cmake, ninja, or clang-format on PATH, no installed spdlog headers, and the sparse checkout excludes build dependency CMake modules. Full application linking and configured gameplay are not currently available. Native fixture compilation and affected C translation-unit checks remain available. The existing Saria resource test used a scratch-only no-op `SPDLOG_TRACE` header for the `Resource.cpp` destructor logging boundary; resource ownership and lifetime logic were real. Native forest fixture assets were restored from the current tracked tree without changing the shared sparse-checkout configuration.
