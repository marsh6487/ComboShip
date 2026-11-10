# Seven Snowhead icicles: source-only accessibility audit

Status: **native class behavior proved; exact placement modes/geometry not proved**. This audit does not change the frozen production slice or existing fixtures. It adds only `tests/shuffle_parity/environment/audit_icicle_access.py` and this report.

The official source pin remains OoTMM `0c0f8a8e1412f3bc979e77a2563293770d58a439`. Its `data/checks/mm/dungeons/snowhead_temple.xml:119–125` supplies these identities, which match the native seven-row catalogue exactly:

| Source location | Native scene | Room | Authored actor index | Native owning region |
|---|---|---:|---:|---|
| Snowhead Temple Icicle Entrance 1 | SCENE_HAKUGIN | 0 | 0 | ENTRANCE_BEFORE_BLOCK |
| Snowhead Temple Icicle Entrance 2 | SCENE_HAKUGIN | 0 | 1 | ENTRANCE_BEFORE_BLOCK |
| Snowhead Temple Icicle Entrance 3 | SCENE_HAKUGIN | 0 | 4 | ENTRANCE_BEFORE_BLOCK |
| Snowhead Temple Icicle Entrance 4 | SCENE_HAKUGIN | 0 | 5 | ENTRANCE_BEFORE_BLOCK |
| Snowhead Temple Icicle Entrance 5 | SCENE_HAKUGIN | 0 | 6 | ENTRANCE_BEFORE_BLOCK |
| Snowhead Temple Icicle Entrance After Block 1 | SCENE_HAKUGIN | 0 | 2 | ENTRANCE_AFTER_BLOCK |
| Snowhead Temple Icicle Entrance After Block 2 | SCENE_HAKUGIN | 0 | 3 | ENTRANCE_AFTER_BLOCK |

The region names in the final column abbreviate `RR_SNOWHEAD_TEMPLE_...`. Each exact native ID is `RC_` plus the corresponding upper-case underscore-separated source name.

## What the source does and does not establish

The pinned xflag XML contains type, location name, room, actor index and replacement/default item. It contains no raw actor `params` or position. The native N64_US/GC_US `mm/assets/xml/.../scenes/Z2_HAKUGIN/Z2_HAKUGIN.xml` files are resource extraction manifests (room segment/offset and display-list/texture declarations); they do not decode the room's actor entries. The generated room headers contain `__OTR__` resource paths, not actor entries. The available pinned generator scene patch sources do not provide this room's actor list.

Consequently the actual parameter word, low two-bit mode, coordinates, floor geometry and unobstructed attack lines remain **unknown for each of these exact seven placements** in this source-only audit. Do not infer stalactite/stalagmite mode from the xflag's `NOTHING`/`RANDOM` replacement-item label. No ROM or private game asset was read or uploaded.

The native mode interpretation itself is exact (`mm/src/overlays/actors/ovl_Bg_Icicle/z_bg_icicle.h:10–15`, `.c:76–87`):

| params & 3 | Native mode | Initial state | Noncombat behavior |
|---:|---|---|---|
| 0 | STALAGMITE_RANDOM_DROP | DoNothing | No automatic proximity break in this mode |
| 1 | STALACTITE | Wait | Proximity → Shiver → Fall → break and kill |
| 2 | STALACTITE_REGROW | Wait | Proximity → Shiver → Fall → break and regrow |
| 3 | STALAGMITE_FIXED_DROP | DoNothing | No automatic proximity break in this mode |

`BgIcicle_Wait` tests only `xzDistToPlayer < 60.0f`, not an item, form, soul, magic or height condition (`.c:129–134`). It sets a ten-frame timer. `BgIcicle_Shiver` changes to Fall when that timer reaches zero (`.c:136–163`). `BgIcicle_Fall` breaks on `AT_HIT` or `BGCHECKFLAG_GROUND` (`.c:165–189`). A weapon-free route therefore exists for a mode-1/2 placement **if its physical approach/fall/contact path is available**. Which of the seven qualifies cannot be concluded from the supplied source metadata. Setting their checks unconditionally true would be unsupported.

## Concrete missing native combat predicates

The native `sCylinderInit` has player AC reception, enabled bumper and damage mask `0xF7CFFFFF` (`z_bg_icicle.c:24–41`). `CollisionCheck_NoSharedFlags` rejects only attacks with no common accepted damage bits (`mm/src/code/z_collision_check.c:1459–1465`, subject to the existing GameInteractor collision veto). `BgIcicle_UpdateAttacked` breaks on a resulting `AC_HIT`; it performs no additional attack-item, form or damage-table filtering (`z_bg_icicle.c:199–230`).

Two native routes are demonstrably omitted by **all seven current CHECK combat expressions**:

| Native route | Native source evidence | Current seven CHECKs | Required next decision |
|---|---|---|---|
| Hookshot hit | `DMG_HOOKSHOT = 1<<7`, accepted by the icicle mask; `ovl_Arms_Hook/z_arms_hook.c:305` sets shot damage flags to `0x80`. Its collider is player AT with enabled attack element (`:149–159`). | No `HAS_ITEM(ITEM_HOOKSHOT)` term | Add a normal hookshot route only after confirming target reach/line at each relevant placement; no ammo requirement is introduced by the hookshot actor. |
| Deku Stick melee hit | `DMG_DEKU_STICK = 1<<1`, accepted by the mask; `ovl_player_actor/z_player.c:6266` assigns it to `PLAYER_MELEEWEAPON_DEKU_STICK`; `:6250–6254` writes that damage flag to the player's melee collider. | No `HAS_ITEM(ITEM_DEKU_STICK)` term | Add a reachable stick-melee route where geometry permits, retaining native stick-bag ownership via HAS_ITEM and resolving source availability. |

These are positive source-backed **attack acceptance** results, not a statement that all seven physical targets can be contacted by a short melee stick or hookshot from every region position. The same native mask accepts tested sword, nut, arrow, Deku spin, Zora punch/boomerang, Goron punch and explosives flags; form/item ownership terms alone do not establish line-of-sight, height, distance or ammunition replenishment.

## Reconciliation with the current region logic

1. The frozen seven combat predicates use `CAN_USE_SWORD || CAN_USE_EXPLOSIVE || CAN_BE_GORON || CAN_BE_ZORA || CAN_BE_DEKU || HAS_ITEM(ITEM_BOW) || HAS_ITEM(ITEM_DEKU_NUT)`. Native attack acceptance supports these classes after contact, but their local reach must still be checked. They also omit the native Hookshot/Stick routes above and conditional weapon-free mode-1/2 proximity routes. They must remain described as conservative WIP, not exact Glitchless parity.
2. Pinned OoTMM uses `break_icicle` for each of these seven source locations (`data/world/mm/dungeons/snowhead_temple.yml:16–20,37–38`). The macro (`data/macros/macros_mm.yml:296`) includes nuts/arrows/slingshot/Din/combat/Deku/explosives. `can_fight` expands to weapon/Zora/Goron (`:332`). This is a donor logic abstraction, not raw placement-mode evidence. It does not override additional attacks accepted by the native actor.
3. Pinned OoTMM entrance-to-main access is `has_mask_goron || has_mask_zora || (break_icicle && can_lift_silver)` (`snowhead_temple.yml:12`). The native current before-block → after-block connection remains only `CAN_BE_GORON` (`SnowheadTemple.cpp:268`). This is an additional conservative access mismatch: native current logic lacks the donor's Zora/silver-lift alternatives. Source alone here does not prove the exact native collision route or classify it as a trick, so do not copy those alternatives blindly. In particular, lifting shared silver blocks is donor-specific functionality, not automatic native equivalence.
4. Donor `has_arrows` and `has_nuts` include renewable/ammo-source requirements (`macros_mm.yml:351–356`). Current native `HAS_ITEM` checks ownership and bag policy; exact renewable ammo availability is part of the unresolved full graph proof. This audit does not create new source events or assume depleted ammo is usable.

The immediate justified finding is **missing native hookshot/stick acceptance terms plus unresolved per-placement proximity modes**, not a safe production predicate replacement. Production remains frozen as requested.

## Executed source-only proof

`CC=gcc-14 python3 tests/shuffle_parity/environment/audit_icicle_access.py --sanitize` PASS in original and Combo guards with ASan/UBSan and no recovery. Only leak detection is disabled for the host's process-inspection restriction. The script:

- validates all seven names/scene-room-index identities against the exact pinned source XML;
- verifies that inspected XML lacks raw params/positions, and verifies the cited native Hookshot/Stick attack definitions;
- compiles the **complete actual production C actor file directly**, without C++ parameter renaming, plus the actual `CollisionCheck_NoSharedFlags` function body and real native headers;
- executes native Init with each of the four possible raw modes, including high/mid parameter interpretation;
- executes actual Wait at the 60 boundary and below, all ten Shiver ticks, then actual Fall with an injected ground contact; modes 1/2 break with no inventory dependency;
- executes the actual damage-mask intersection predicate and actual UpdateAttacked for the listed native damage flags for each mode; normal shield is a rejected mask control.

Allocation, collision registration/contact, floor geometry, effects, rendering and engine state are explicit service boundaries. The test supplies `AC_HIT` only after native damage-mask acceptance, and supplies ground contact to test the actual transition. It does not simulate the seven source coordinates, room geometry, a player hitbox, or a complete gameplay scene. No reachability/full solver/seed/link/gameplay proof is claimed. No production or existing test file was edited by this audit.

## Input hashes

SHA-256 emitted by the source audit before root's subsequent clang-format14 pass. These identify the tested source snapshot; formatting may change native source hashes without changing behavior:

| Input | SHA-256 |
|---|---|
| pinned checks XML | `c849c348f74f12fadd4ddfcbdfa8e87c44672a5b3d358bf9adf756c5a81a4f55` |
| pinned Snowhead world YAML | `da38237916808c660bc399b7014d5b137bc1fb2c5ae076946f0ed13750dab2f6` |
| pinned MM macros | `8a86475b87d535bc1fd84a8e6e7c9af69ceb678700747f9534e1d367a973a5c3` |
| frozen native BgIcicle C | `0f327b528d01a20dc95f66756587cc18fd39a6f48a95efcefedc3addf95b7884` |
| native BgIcicle header | `90e6d08837fd3a16519f0bbe3324303303bd85a6ea3d8ba67d23b317dafa894d` |
| native collision C | `b37b9da97d3b8f7633bb5ec49f74e22ae1a7704e80d9563f8bc1315b436b8f2f` |
| frozen seven-row catalogue | `0c81b2aafb3627af3c86b51f4389f2d3d8a4a953c246e6cbdd247fd4ef219ec7` |
| frozen native Snowhead region C++ | `809398e14323507ad3a8ac3401e8c6b9860a2d7e84a5380e8d87e47b4be838cb` |

## Requested post-format control verification

After root's clang-format14 pass, the existing `run_tests.py --sanitize` passes again in both original and Combo guards with the prior official dependency environment and native `GBI_FLOAT_MTX`/`GBI_S32_VTX` ABI flags. The existing `run_exclusion_tests.py` also passes both guards for all 128 subsets and actual disabled-display/metric helper integration. No production or existing fixture changes were made during this audit/control rerun.
