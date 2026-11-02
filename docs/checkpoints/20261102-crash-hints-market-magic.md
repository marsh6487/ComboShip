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

The new commit is a direct child of the pinned complete-source base above. Check out the candidate branch to recover all four additions; check out the pinned base to recover the preceding shop/startup/Chamber source. The equivalent locally tested subset checkpoint was `87e13883b99ea17f727e2f619d0075953443906a`; that subset SHA is a preparation identifier, not an upstream GitHub commit.

For the magic investigation, the next decisive evidence is the `[MagicLoad]` begin and settled/still-active records from a load where the sound persists. A settled meter with matching current/target values directs attention to audio; a still-active meter exposes values for tracing the writer or target that prevents completion. A save-file upload is not a prerequisite for gathering this trace.
