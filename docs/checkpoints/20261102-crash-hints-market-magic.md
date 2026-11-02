# November 2 source checkpoint

Base: `377e8191fb389ad8aec28f89879f0dd148925673`, branch `candidate/oct31-shop-startup-chamber-20261031`.

This candidate preserves that complete source baseline and adds the current four-part pass:

- Shadow Crystal/Wolf entry and removal use the existing instant-flash path. Wolf has a separate SSBB animator; the generic MM transition could call an uninitialized animation callback. Removal captures its form before the transition so later ticks retain the correct path after reload.
- Map/compass receipt wrapping accepts OoT's 8192-byte destination and uses adequately sized scratch buffers. The old 1280-byte destination check skipped wrapping in Unbound. MM retains its actual smaller buffer boundary.
- The inspected R19 Market suite adds a foliage-only seasonal eligibility path for eleven exterior day/night/ruins variants and 302 reviewed foliage identities. Weather eligibility remains unchanged. Atlas masks protect adjacent posters and masonry; whole prerendered background photographs are excluded.
- Fractional magic loading gets bounded OoT tracing. The reported persistent refill sound with one of fifteen configured magic upgrades remains unresolved. The trace records restored magic, expected capacity, ownership and refill targets, followed by completion or a still-active record after 120 interface magic updates. It does not change magic or stop audio. MM has isolated meter checks, but no added production trace in this checkpoint.

## Verification

Fresh focused checks passed before publication:

- 1305 OoT and 1305 MM fractional/native-floor load cases, including required=15 and empty/partial/full saved amounts. These compile extracted production branches with option, hook and audio service boundaries. Sound assertions measure requests, not audible output.
- A deliberately reset current amount verifies bounded stalled-fill tracing without changing the injected failure. It does not reproduce the user's unidentified production trigger.
- Eight Wolf transition cases, including reload, buttons, repeated transitions and loader failure.
- OoT/MM receipt typography, wrapping, pagination, boss/reward sprites and message reset checks.
- Eleven Market exterior eligibility cases, 906 native/Alt cache bindings, and 56 actual archive texture fixtures covering source/alpha/petal preservation and Summer restoration.
- C11 compatibility of the public RPG header, Python syntax, CI manifest parsing and whitespace checks.

Focused native fixtures use ASan/UBSan with LeakSanitizer disabled on this host. Independent source reviews found no introduced Critical/Important issue. Full engine linking, configured game runtime, audible behavior and user acceptance remain pending. The candidate is not promoted to an accepted master.

## Asset identity and recovery

The source tint catalog was inspected against `Market_Suite_R19_Bombchu_Routes_And_Day_Alley.unbound.o2r`, SHA256 `e62cfb6309e159695611dcd861c83fffbe4cb2afe6883333b1a6dbb7039583b6`. That archive is unchanged and is not included as a replacement in this commit.

The first published checkpoint, `33359ffce2a79e80d7f490d64c1462bfcaad6b14`, is a direct child of the pinned complete-source base above. Subsequent candidate commits preserve that history. Check out the candidate branch for the cumulative additions, the first checkpoint for the initial four-part pass, or the pinned base for the preceding shop/startup/Chamber source. The first checkpoint's equivalent locally tested subset was `87e13883b99ea17f727e2f619d0075953443906a`; that subset SHA is a preparation identifier, not an upstream GitHub commit.

For the magic investigation, the next decisive evidence is the `[MagicLoad]` begin and settled/still-active records from a load where the sound persists. A settled meter with matching current/target values directs attention to audio; a still-active meter exposes values for tracing the writer or target that prevents completion. A save-file upload is not a prerequisite for gathering this trace.

## Cursor-chat integration and latest reports

The subsequent checkpoint adds the reviewed OoT empty-cell pause-cursor repair
and the Market night room-background camera guard from the November 1
"Fix Cursor Snapping" work. The cursor repair stabilizes intentionally selected
empty cells, permits vertical movement through them, and defines or hides all
four cursor corners. The room guard preserves the background-load hook before
restricting prerender metadata access to the fixed prerender camera, and handles
a null background result. Both donor gates were appended to the existing CI
manifest; none of the earlier registrations was removed.

Fresh native ASan/UBSan checks passed for six cursor cases in both standalone
and combined modes, existing pagination/selection guards in both modes, and
ten room-background cases per mode. Source and fixture bytes match the reviewed
donor manifests, apart from the deliberately merged CI manifest and updated
publication notes. The complete executable and configured gameplay remain
untested. Enabled-policy side-arrow reentry remains a runtime check.

The separate Graveyard camera snap remains unresolved. Its investigation is
retained in `docs/diagnostics/graveyard-camera-status-20261102.md`; this commit
does not enable its diagnostic camera patch or claim a Graveyard repair.

Latest user reports: a new save load played the magic-fill sound normally;
the intermittent cause is still unconfirmed and the existing bounded trace is
retained. The separate overworld POC's DMT vertex spill was absent in the night
scene, while daytime had not been checked. That is an asset-POC observation,
not runtime proof for this source checkpoint.

Combo Vanilla was proposed during this integration. The current launcher has
Combo/OoT/MM session modes, while combined save creation requires a consolidated
randomizer seed. A native combined vanilla flow therefore needs paired native
save creation and an explicit per-save identity; it cannot be implemented by
adding only a menu label. No Combo Vanilla product behavior is added here.

## Formatting repair after the failed workflow

The Fast source checks job `125174010587` for remote head
`46d57fd52152454388dea6d36c86d26c734d1615` failed at the clang-format diff
gate, before the game-build matrix. The repair applies the exact CI-produced
formatter output to four files: SeasonFoliageTint.h, SeasonWeather.cpp,
randostatupgrade.cpp and season_foliage_market_test.cpp. Each resulting blob
matches the formatter's recorded new blob ID, and a lexical comparison confirms
no C/C++ token changes. No checks were disabled.

Fresh native ASan/UBSan runs passed the magic-load probe (1305 cases per game
and bounded OoT trace checks) and Market probe (11 eligibility guards, 906
native/Alt bindings and 56 actual archive textures). Full workflow/build results
remain pending for the replacement checkpoint; these probes are not an engine
build or runtime acceptance.
