# MM foreign OoT GI follow-up candidate

Baseline: PR33 `69957d96eca1ddf1880c0f7df3c21acac2bc75e8`, tree
`07da4fa13dd39661e5d2d70bc69f26371f40bdd7`. The reconstructed local parent
`fb8a166` has that exact tree. This candidate is isolated from that baseline.

Cor confirmed blue potion GI/message icon, blue-fire bottle GI/message icon,
and MM flame with the custom Volvagia head. Those are runtime evidence for the
baseline. New candidate tests are command tests, not new game acceptance.

| Request | Candidate behavior |
| --- | --- |
| Masks and transformation colors | Same procedural NEI `SampleShimmer` geometry through shared renderer in both hosts; existing transformation/remains palette retained |
| Progressive swords | Resolve actual concrete award first, select standalone OoT owner Alt equipment/Din blade when available, preserve shimmer on native fallback |
| Custom equipment | Typed concrete MM draw callbacks for inline equipment; authored paths for static/split models, owner-aware routes and validated operation metadata |
| Seasons | Four season color/flame recipes plus Rod of Seasons |
| Slate and runes | Authored slate mesh and native rune flame colors |
| Scepters, hourglass, shadow crystal | Authored static/split recipes rather than unsupported-callback sentinel |
| Actions | Existing crawl/climb/open-chest/grab GI recipes retained; no standalone `RG_SWIM` exists, swimming uses scale ownership |
| Morpha foreign OoT GI | Actual membrane/nucleus, native rotation/scroll/color, MM native flame; selected owner Alt paths |
| MM souls rendered in OoT | Goht, Gyorg, Odolwa actual skeleton/clip exports; native MM behavior retained |
| Bean pack icon | Explicit magic-bean texture binding, avoiding randomizer/native itemId collision |
| RPG magic jar | OoT cosmetic color carried with custom selected-asset classification; MM submits native-equivalent grayscale scope, refreshes after acquisition latch |

See `boss-soul-model-routes-20261003.md` for all nine OoT/four MM boss routes
and animation clips. Native SoH already advances those skeletal clips. Morpha
uses procedural rotations/scrolling, not a skeletal clip or native bobbing.

## Verification

Production-body tests cover the draw descriptor, actual award resolution,
owner Alt selection, routing validation, custom submission, inline axe draw,
mask/remains dispatch, exact shimmer mesh, C/C++ linkage, Morpha/MM soul recipes,
native MM flame, icon staging, and magic cosmetic/latch behavior. Focused suites
pass normally and under ASan/UBSan with leak detection disabled because the
container cannot inspect `/proc` for LeakSanitizer.

Both-host native color/syntax tests, foreign skeleton/Alt/fallback, spin,
NEI GI/held renderer and MM NEI suites pass. Independent review found and resolved
magic latch freezing, an early flame declaration, sword shimmer overwrite,
a C return-type mismatch, and metadata-only Alt magic aliases. No grant/action gameplay code changed.

## Remaining evidence and source limits

- Full application build and in-game appearance remain unverified locally.
- TP boss archive retrieval returned HTTP 502 twice; exact installed custom
  model/skeleton mappings remain unverified. Owner canonical Alt routes are
  preserved rather than inventing resource names.
- Barinade foreign animated geometry requires per-limb scale/rotation and
  post-XLU handling beyond the current descriptor. Its skull fallback remains.
- Twinmold foreign animation needs its separate 23-matrix segment-13 binding;
  its existing remains alias remains. Native MM Twinmold behavior is unchanged.
- Native MM imported `RI_SOUL_OOT_BOSS_*` rows retain their existing generic
  skull path; this change targets the foreign OoT GI route used for custom
  boss/equipment checks.
- Frozen sword award recipes retain the selected Alt appearance until cache
  reset. Live Alt toggles during a latched progressive award are not proven.
- The requested death-skull texture swap is not implemented. The supplied
  image is a cropped save-panel reference with surrounding background, while
  the native save-panel art is embedded in IA16 file-info textures. A clean
  source texture is needed for an exact standalone RGBA icon replacement.
- The full local combo regression script cannot read its historical
  `c77c185...` Git object from the recovered snapshot. Downstream suites were
  run separately; this is not a full-suite pass claim.

Runtime acceptance should compare Alt off/on with the same pack/load order:
Morpha vs accepted Volvagia; each progressive sword family; Seasons/runes;
scepters/hourglass/crystal/equipment; mask transformation hex and shimmer;
bean icon; RPG magic color; blue potion and blue fire regression checks.
No master promotion is part of this candidate.
