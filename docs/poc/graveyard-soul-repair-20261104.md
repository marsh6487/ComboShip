# Graveyard and soul repair candidate — 2026-11-04

Baseline: 2e28b239cb4aa0f7f31da683cd15b1bcc8874ac1, matching the supplied crash build. Candidate branch: candidate/graveyard-soul-repair-20261104. Original archives/master preserved; runtime acceptance not claimed.

## Implemented changes

GraveHoleJumps now pins only the active matching collision resource, restores its table/polygon assignments before teardown or settings changes, and sizes extra-surface storage dynamically. Mod collision resources are excluded from vanilla polygon-number patches, including custom31/32-surface meshes retaining material numbers. The user's39-surface Alt Graveyard is unchanged. OTR/Unbound exit commands retain their actual list length; player and Hylia's Grace reject null, negative, high-bit and out-of-range exits. Count-less raw N64 commands retain a1..31 conservative fallback, not an exact legacy array bound. The PlayState field is appended.

Majora's cross-game GI describes its actual nonflex floating mask, normal eyes seg8, billboard, orange flame and post-skeleton tentacle material; its remains alias is removed. Native/fleet identities were already correct; no Gyorg alias was established. The animation ABI appends one field: rebuild both game DLLs together.

OoT souls now use a dedicated native flame root/quad, retaining native material, tint, scrolling and shared Alt texture IDs. This avoids the TP miscellaneous pack's deliberately invisible bottle-flame replacement. Bottle resources are untouched. Rebuild and ship matching soh.o2r with the DLLs.

The separate POC5 icon archive repairs the192x192 POC4 resource's version, RAW flag,6x6 scales and declared pixel count (old header4096 bytes versus147456 physical bytes). All artwork bytes are preserved. tools/boss_soul_icons/repair_pack.py reproduces the deterministic candidate from original POC4 input SHA256 ead9c8dedfbedc1468534aed2f4c12fee8dfcb44c300794d2a683fdc60967e9a.

## Evidence and verification

- Production Graveyard ASan/UBSan: baseline39-surface overflow reproduced; candidate stock31/32, custom39/custom32, mod32 unchanged material numbers, toggles and teardown/reentry pass in solo/combined. Independent review found the latter mod32 preservation gap; new regression failed before the explicit mod exclusion, then passed.
- Shared exit-guard tests pass for valid endpoints/null/zero/negative/high-bit/excess indices; both production callers checked.
- Majora actual-engine-header routing and recipe checks pass. Production foreign-skeleton and boss-model tests pass with sanitizers in both hosts, preserving Barinade/Twinmold, resource ownership, segment cleanup and material order.
- Dedicated flame verifier passes with a TP zero-alpha negative control. Existing POC5 texture hashes and agent-run llvmpipe checks pass over1024 OoT/256 MM phases for all3 filters. This is not in-game pickup proof.
- Native/imported soul receipt icon production tests pass. Candidate image bytes equal original POC4; deterministic ZIP verified.
- Player/scene/play/Hylia C translation units pass real-header syntax with the soh/mods include root. Existing unrelated warnings remain.
- Broader custom_unbound_collision behavior checks pass shared/SM64/water/grapple/actors/bridges/motion/TimeCtl/Granny, then run_adapter_abi_tests.py blocks on missing nlohmann/json.hpp. Complete C++ translation-unit/application builds also lack external nlohmann/json.hpp and spdlog/spdlog.h. No full platform build claimed.
- Independent review reported no other important defect. git diff --check passes.

## Install and runtime verification

Replace POC4 with BossSoul_HD_Tracker_POC5_Metadata_Fix.o2r and select it as the winning gBossSoul resource under base/Alt. This asset fixes the dialogue/tracker icon only; other repairs require the rebuilt source candidate, both DLLs and matching soh.o2r.

Test Graveyard torch-room descent/reentry and native grave entries with the option toggled; Majora pickup/preview with Alt off/on; Gohma flame with Alt off/on; TP blue-fire bottle casing; soul dialogue/tracker icon. Original archives remain rollback inputs. No commit was pushed or master promoted by this handoff.
