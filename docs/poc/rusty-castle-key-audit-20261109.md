# Castle rusty key: actual native interaction audit

The earlier En_Door-only mapping was insufficient evidence of a usable Castle Key. This audit fixes the active family rather than inferring functionality from the catalog, pool or classifier count.

Pinned donor: `OoTMM/OoTMM@0c0f8a8e1412f3bc979e77a2563293770d58a439`.

| Inspected pinned source | Blob | Finding |
|---|---|---|
| `packages/generator/src/oot/ovl/actors/En_Door/En_Door.c` | `52488bb34f3fecb838218d27d8a7c49463f8dea1` | Castle scene maps to Castle door ID; rusty ownership gates Idle player offers. DOOR_CHECKABLE instead selects WaitForCheck and never becomes openable merely by obtaining a key. |
| `packages/generator/src/common/item/item_add.c` | `8c0d5738a4dd6de2d5fb07a964bf71f1b97bdb98` | Castle item add is generic rusty bitmap ownership, without a native gate interaction. |
| `packages/generator/include/combo/doors.h` | `43d034db86ff88e9fa834819bde787abaf746e0e` | Castle is door family 2. |
| `packages/generator/src/oot/actors/En/En_Heishi2.c` | `6ef18812d396a6cf1cc63530b2fd10ee2e80f359` | Inspected guard patches concern letter/mask trade; no CastleKey opening handler. |
| `packages/generator/src/oot/doors.c` | `cc96f05544c08d3d4482999dba84fe7570134185` | Dungeon key-door policy concerns En_Door/door shutters, without the Castle collision gate. |
| `data/world/oot/overworld/hyrule_ganon_castle.yml` | `80b4b352bbda78398275e72ea1d3796ec2daaa89` | Inspected Castle world has no Castle rusty-key/get_key requirement or separate gatehouse interior. |

The pinned recursive tree was not truncated and contains no BgSpot15Saku replacement. Generic actor/scene/room and miscellaneous fix sources were also inspected for a Castle gate/key patch; no matching interaction was found. This does not claim every donor source file was searched or that actual Castle En_Door instance bytes were loaded.

The supported native crossing actor is `ACTOR_BG_SPOT15_SAKU`. Its existing Init registers dynamic collision and restores InfTable71 opening; its action functions animate the gate's world position from its scene position toward native threshold2660/position2659. The Castle guard is precise EnHeishi2 role2 in the Hyrule Castle scene. Its native payment function changes rupees and schedules opening; no other guard role needs rusty-key behavior.

## Implemented full27 contract

For new catalog version1 with LockOverworldDoors enabled, CastleKey or Skeleton Key authorizes the actual native collision gate. Without that ownership, Init and Update keep collision closed even if InfTable71 was previously earned. No earned InfTable/save flag is cleared. Pending native animation/request state is preserved; once ownership arrives the existing timer/action/audio movement opens the gate. Native mode, the control off, version0 legacy24 and other scenes retain the original behavior.

The exact Castle guard role no longer offers a redundant bribe in key mode. Its existing closed-door message or go-ahead message is selected; an already pending choice returns before currency/statistics/progression updates. Other scenes and guard roles retain their native payment behavior. The direct Castle key thus replaces this route's bribe/Hylian prerequisite only in the explicitly new key mode, as approved by the controller.

Only the native `RR_HC_GATE -> RR_HC_PAST_GATE` ground edge selects CastleKey in key mode. Legacy/off uses the existing wallet + Speak Hylian predicate. Above-vines alternatives and return edges remain unchanged. The unusable Castle En_Door classifier proxy is removed: the 27th active family is proved through the actual BG actor rather than fabricated En_Door parameter cases.

Hidden saved catalog version is also explicitly excluded from RandomizeAllSettings; migration metadata is not a user-randomized feature.

## Verification scope

The legacy fixture compiles the exact native gate Init/Update/three action functions as C with native structs, actual native InfTable getter, guard payment function, Rupees_ChangeBy and inventory capacity tables. It tests missing/own/wrong/Skeleton ownership, vanilla/off/version0, saved-open and paid/request cases, native animation/audio progression, preservation of InfTable bits, and exact guard role/scenes before charging. Logical checks execute the extracted native ground/vines/return expressions using actual Entrance condition evaluation and the production key-oracle branch. Native actor/source syntax checks use real headers and unchanged target flags.

Dynamic collision/resource allocation, audio/message/animation engine services and non-key inventory are bounded service seams. Unexecuted guard continuation callbacks trap if entered; no continuation behavior is claimed. The fixture does not load scene bytes or run the full engine. Configured gate collision, scene interaction and complete native generation/replay remain separate controller gates.
