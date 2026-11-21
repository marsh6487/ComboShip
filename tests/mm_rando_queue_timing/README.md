# MM randomizer queue timing simulation

Run `python tests/mm_rando_queue_timing/run_tests.py --sanitize`. The runner
builds both standalone and `COMBO_BUILD` variants. Use the repository native
compiler adapters for the combined `GBI_FLOAT_MTX` / `GBI_S32_VTX` ABI.

The fixture verifies that the two starting checks arrive without interrupting
movement and that regular checks keep their existing timing.

## What executes

The runner extracts unmodified production bodies for:

- `Rando::MiscBehavior::CheckQueue` and `CheckQueueReset`.
- `Rando::StaticData::ShouldShowGetItemCutscene`.
- `ProcessEvents` and native `Player_InBlockingCsMode`.
- `CustomItem::Spawn`, `CustomItem00_Init`, `CustomItem00_ItemTouched` and
  `CustomItem00_Update`.

The fixture uses real MM `PlayState`, `Player`, `EnItem00`, `RandoSaveCheck`,
static check/item declarations and `GIEvent` variant types. It uses the actual
`RC_STARTING_ITEM_DEKU_MASK` and `RC_STARTING_ITEM_SONG_OF_HEALING` check IDs
with controlled major, junk and foreign awards. The two South Clock Town
chests exercise normal timing with controlled green-rupee awards.

External boundaries are actor allocation, pose-parent acceptance, engine audio,
graphics, collision/collider services, item naming/conversion, UI messages,
notifications and the final inventory `Rando::GiveItem` operation. The final
inventory operation records callbacks; actual obtained/eligible check-state
changes and exactly-once callback flags run in the production queue callback.
The fixture measures grant callbacks and check consumption. A Combo stub
records the foreign startup handoff, where the cross-game save and notification
normally occur. Trap effects and persistence are outside the fixture.

Native parent acceptance is supplied explicitly after seven offer updates.
The native `Actor_OfferGetItem`/player animation action itself is not simulated.
Message open/close is supplied as controlled native state. Hook calls are made
in a stated CheckQueue-then-ProcessEvents order, rather than running the complete
native main loop; actual hook ordering could add an update boundary.

## Observations

- Nine controlled message-blocked updates followed by five controlled native
  cutscene-blocked updates retain exactly one queued event. A native transition
  also retains it. The first unblocked `ProcessEvents` invocation creates the
  custom item immediately; there is no extra first-load countdown in this path.
- While native parent acceptance is withheld, seven item updates offer the
  item but leave `unk152 == -1`, scale zero and grant count zero.
- The first parent-accepted update initializes the counter to zero. Nineteen
  more updates remain hidden and ungranted. The twentieth update after zero
  invokes the actual queue grant callback and makes the item visible together.
  Further presentation updates do not invoke a second grant.
- A second eligible check waits behind the first ungranted check. After the
  first grant, it can enqueue, but an active receipt message still blocks its
  actor from spawning. Both checks then consume once in order.
- Destroying an ungranted actor requeues its pending event; destroying the
  granted replacement does not enqueue a duplicate grant.
- With the real selector's existing skip setting at one for the junk award,
  the overhead path grants and becomes visible on its first in-range update.
  Its remaining overhead presentation counter is fourteen after that update.

## Existing pacing setting

`BenMenu.cpp` exposes **Skip Item Get Cutscene**, with the CVar
`gEnhancements.Cutscenes.SkipGetItemCutscenes`. Options are Never (zero/default),
Junk Items Only (one), Everything But Major (two), and Always (three).
`Items.cpp::ShouldShowGetItemCutscene` keeps major cutscenes below three, keeps
health/lesser/stray-fairy/token/small-key cutscenes below two, and keeps junk
cutscenes only below one. Foreign checks use the separate existing Combo
importance selector. The two starting checks always use overhead delivery.

`ProcessEvents` also selects overhead delivery for its listed player motion/
action flags or held explosives. When the event requested a cutscene but falls
back to overhead, the existing callback applies a thirty-count freeze. Neither
the setting nor overhead fallback bypasses queue-level active messages, native
blocking cutscenes, transitions or an existing current event.

The full game's cold-load and rendering time remain runtime-unverified.

## Authentic randomizer start-state extension

The fixture also executes unmodified `PrepareNewRandomizedSave` from
`Rando/Logic/SeedGeneration.cpp`, the two actual eligibility assignments from
`MiscBehavior/OnFileCreate.cpp`, and native `Player_StartMode_D` plus
`func_8083A878`. The latter two functions rename only the C parameter `this`
to `self` for C++ compilation. Pose/movement setup and water queries remain
engine boundaries; the fixture uses a dry floor. Equipment masks/shifts and
week-event bit storage follow native representations; flag-hook subscribers
are not run.

The real initializer establishes Human Link at South Clock Town entrance zero,
cutscene index zero, reset count one, first-cycle progression completed, Tatl
first-arrival suppression bits and the Happy Mask Salesman tower switch. Both
starting checks become eligible without waiting for a Clock Tower actor or
flag event. Configured starting inventory is a separate direct grant path.
Combo's headless builder mirrors the same two eligibility assignments in
`BenPort.cpp`; it is source-traced rather than executed by this fixture.

Native start mode D sets the negative twenty counter and the native blocking
`PLAYER_STATE1_20000000` bit. The actual event-processing predicate rejects the
starting check during an incoming transition; after clearing transition state,
the arrival player bit independently keeps it queued. Clearing that bit lets
the event spawn on the first subsequent call. The fixture does not execute the
full movement/camera controller `Player_Action_35` or claim that cor's actual
scene entry selects mode D. Source trace shows that controller increments the
negative counter toward zero before `Player_StopCutscene`; modes E/F use their
own counters. Actual scene/respawn parameters are required to identify cor's
selected mode and measured wait.

At the first safe opportunity each starting check spawns overhead and grants
on its first item update. The next check can spawn while the first receipt is
still visible. The fixture verifies two notifications, one grant per check,
and no get-item offer, textbox, freeze or movement-speed change, even with
the item-cutscene setting at Never and the native pre-dawn border active.
