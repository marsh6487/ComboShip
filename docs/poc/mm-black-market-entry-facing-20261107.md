# MM Black Market entry-facing correction, 2026-11-07

| Field | Record |
|---|---|
| Baseline | Candidate branch `candidate/night-music-dialogue-20261107`, parent `2054eef2`; cumulative Black Market shop and item-get camera fixes retained. |
| Scope | Normalize the legacy player yaw while generating MM's private shop scene, before assigning its independent player ITEM_GET cutscene. |
| Preservation | Player position/params/forms, four browsing camera records and chain, independent receipt camera record, room/geometry/collision/keeper, stock/purchases, OoT preparation and accepted input bundle remain unchanged. |
| Evidence | Failure first at the actual native spawn boundary, then sanitizer verification across five forms and repeated entry; serialized factory/generator tests and preparation byte audit. |
| Verdict | Source/preparation and native CPU behavior verified; full executable and configured game entry remain untested. No commit, publication, acceptance or master promotion in this workstream. |
| Recovery | Restore `tools/black_market/mm_native_shop.py` from `2054eef2` to remove the bounded production change. Retained bundle remains unchanged. |

The ITEM_GET-link generator added in `4cdd5f07` treated the retained scene's
raw OoT binary yaw `-32768` as an already packed MM angle. Setting its low
seven bits to cutscene 4 produced `-32764` (`0x8004`). That bypassed
`ComboBlackMarket_PrepareSpawn`, whose legacy compatibility check matches
exactly `-32768`. The actual MM `Actor_SpawnEntry` then read the upper nine
bits as 256 degrees, yielding native yaw `-18933`, instead of the intended
180-degree half-turn `-32768`. The previous generator test incorrectly required
preserving those raw upper bits.

The generator now converts that legacy half-turn to MM's packed `180 << 7`
before assigning cutscene 4. The resulting value is `23044` (`0x5A04`).
Already native packed headings retain their angle; applying preparation again
is idempotent. The previous malformed raw-angle/cutscene-4 output also upgrades.
The native startup compatibility path remains unchanged for legacy archives.

`run_scene_spawn_tests.py` now consumes the real host-specific prepared scene
alongside its existing retained-bundle fixture. It executes production
`OTRPlay_InitScene`, `Actor_SpawnEntry`, cutscene-manager initialization and
`Play_AssignPlayerCsIdsFromScene` with real engine types. Before the fix it
aborted with `prepared MM market spawn decoded yaw -18933, expected -32768`.
Afterward Combo and standalone fixtures pass all five forms over three entries,
preserving position, params, source cache, form and ITEM_GET cutscene 4. Legacy
entry, ordinary scenes, authored native chains and forest controls also pass.
Actor allocation is a controlled fixture boundary: the form loops preserve each
form setting but do not execute form-specific `Player_Init` or prove visible
in-game facing.

Verification:

- `python3 -B tests/black_market/run_scene_spawn_tests.py`: pass with address and undefined sanitizers.
- `python3 -B tests/black_market/test_mm_native_shop_scene.py`: all seven tests pass, including real serialized resource factories/player cutscene assignment, deterministic preparation, native headings, legacy upgrade and malformed chain rejection.
- `python3 -B tests/black_market/run_shop_camera_mm_tests.py` with the compiler environment below: pass. Actual native EnOssan/EnGirlA, cutscene manager and Data4 integration cover five forms/eight selections, item-get/receipt handoffs, exit/re-talk, camera ownership/restoration and full selection/return geometry clearance (7,577 room and 144 shelf triangles).
- Prepared MM comparison against the parent generator changes exactly one byte: scene offset 249, `0x80` to `0x5A`. The other 213 prepared resources are byte-identical, including room and all camera/cutscene records. Baseline scene SHA256 `e527d4a6058c875ecf25fc6d42f120df910d27bb346a04711939f4cec06fe2e2`; candidate `e44796d1f65a9c816ac530749b59d8356383e711d963000a358b97baa06ba34e`.

The optional scene-readiness runner was attempted but compilation is blocked
by missing system `SDL2/SDL.h` after supplying the available spdlog headers.
No readiness result is claimed from that run. The broader native shop-camera
runner uses temporary external compiler wrappers for this environment's GCC14:
CC adds `-Wno-error=incompatible-pointer-types -Wno-error=int-conversion`, CXX
adds `-Wno-narrowing`. These accommodate existing engine C/resource declarations;
no repository source or test assertions are relaxed.

Normal source preparation regenerates the corrected bundled MM scene. No
external scene replacement or new user-provided input is required. The next
configured test is normal Black Market entry/re-entry, followed by browsing,
purchase/item-get and return, using the rebuilt resources from this candidate.
