# Shovel and winter surface feedback — POC 1

Status: implemented and native-source verified candidate. No full game link or configured gameplay/audio run, remote publication, merge, or accepted-master promotion.

## Baseline and preservation

The supplied hidden UI/Debug Warp night-music checkpoint is preserved in full:
ComboShip_UI_Control_And_Debug_Night_Music_POC1_20261108.zip
Library identity libfile_721802b6703c819184ec2be8ffe04383.
All 25 resulting source/test/document files were independently matched before work and again after implementation.

The prior checkpoint names public cumulative source commit 008534e3b1abcdfa8805a555de8d4639991b740f,
tree 355ef7377653c3a6de7d8ebea4483fb7716939b7. This candidate uses the same affected source
boundaries and retains every prior UI/navigation/night-music change. Its local commits belong
to an isolated bounded source snapshot, not public repository ancestry. Baseline for the new
changes is 9fa3836; implementation milestone is 1f8b02d.

Protected: collision geometry and waterboxes, actor placements and grotto destinations,
randomizer progression/loot, shovel impact frame, season ownership, swimming below surfaces,
normal nonwinter water-walking boots, other seasons, weather/BGM ownership, and native footwear/form overrides.
No external asset archives, saves or gameplay logs are included or modified.

## Changes

### First shovel input

OoT started the dig clip in Shovel_Start while the native item-change action still owned
upperSkelAnime. The equip action could consume/replace that animation and hand off an already
completed clip; the shovel callback stopped before its impact. Start now latches the requested
dig and validates the animation resource; the first Player_UpperAction_Shovel callback installs
the actual clip, matching MM's existing approach. One press can draw the tool and complete one
dig without a second press. Dig timing, rewards, radius and entrance routing are unchanged.

MM shovel reveal now uses DoorAna's actual hidden-type macros and destroys its bombCollider
once when revealing a bomb grotto. The visible-scene-exit type (0x300) is no longer mistaken
for a hidden grotto, so digging cannot erase that exit's destination mode.

### Snow steps

Both engines select snow audio for dry, grounded outdoor scene terrain with ground, sand,
concrete, dirt or grass material. Existing weather eligibility determines where presentation
is Winter. Covered eligible OoT exteriors use the existing particle-season eligibility.
Moving platforms, ladders, special surfaces, water and ice retain their own material sounds.
This selection applies to walking sounds only; it does not rewrite collision material,
snow footprints, floor physics, jumps or landings.

MM uses its native NA_SE_PL_WALK_SNOW. OoT uses that MM sound (0x080E) through its existing
foreign-audio bridge. OoT's corresponding native slot is a dummy: when optional MM audio
cannot play, native sand audio is used as a soft, audible fallback. No new sound pack is required
for a Combo installation with its existing MM assets. Native heavy boots and the existing
transformation/wolf/giant audio overrides retain precedence.

### Frozen water

Before Actor_UpdateBgCheckInfo, the player supplies its native no-entry-ripple flag (0x40)
when Winter surface walking is eligible. Collision and water queries still execute.
Previously clearing the frozen WATER flag made the generic actor code repeatedly register
another water entry and spawn three ripples. The new flag prevents those entry effects.

The current frozen surface pin clears WATER and WATER_TOUCH and sets the floor sound to
native ice, independently of a lake-bottom polygon. MM checks the frozen pin before its normal
floor-sound selection can overwrite ice with the submerged floor. Its later wading-effects
branch is suppressed by the cleared water flag. OoT retains its existing frozen wading guard.
Nonwinter boots still use ordinary water presentation; underwater swimming is not disabled.

MM now resolves freezing from owned saved Winter, as OoT already does. It no longer depends
on a cached outdoor-weather presentation result. This extends freezing, ice audio and ripple
suppression to temple pools without adding snowfall or snow footsteps to dry temple flooring.
A manually selected Winter weather preview alone does not grant freezing. OoT's explicit
always-thawed Zora's Domain exception remains intact. Existing MM human-form eligibility
and both engines' nonswimming/descent requirements remain unchanged.

## Verification

| Check | Result |
| --- | --- |
| New native feedback fixtures | All 20 host/case/guard combinations pass |
| Sanitizers | ASan and UBSan; native signed-shift flag spelling excluded |
| First-use regression | Fails on original OoT equip-overwrite boundary; passes after delayed clip installation |
| Native hidden grotto semantics | Storm/bomb reveal once, collider lifetime, destination and MM scene-exit preservation pass |
| Surface feedback | Snow/ice priority, dry/special materials, boots/form offsets, missing MM-audio fallback pass |
| Collision fragments | Suppressed entry-ripple flag, null bottom, pin, jumping, swimming, shoreline and nonwinter boots pass |
| Frozen gameplay gate | Saved owned Winter in temples, no preview-only/unowned freeze, OoT thaw exception pass |
| Full player C translation units | All four compile to objects: OoT C23 solo/Combo; MM C17 solo/Combo |
| Hidden UI | 736 native control dispatches pass; focused OoT/MM suites pass, including 46 MM syntax checks |
| MM ordinary item actions | Existing climb/tool input-camera checks pass |
| Rod input lifecycle | Existing MM dispatch and OoT stow checks pass |
| Existing MM winter-water case | Pass with fixture updated for saved-season getters |
| Night music | Lifecycle and OoT normal/Debug Warp/MM adapter checks pass |
| Independent review | No substantive source defects; reviewer independently passed all 20 new combinations |
| UI/night preservation | All 25 prior checkpoint files remain byte-identical |
| Patch/installer restoration | See packaged manifest and restore evidence |
| Whitespace | git diff --check passes |

The broader existing season/native-weather and night-music runners reach unavailable
nlohmann/json.hpp in later unchanged dependency-heavy units. Their reached checks pass;
their later stages are not claimed. MM's initial full-player probe used the global C23 standard,
which rejects older no-prototype compatibility APIs; rerunning with the C17 standard explicitly
required by mm/CMakeLists.txt produces both MM objects successfully. No dependency stubs or
compatibility APIs were changed to obtain those results. No formatter executable is installed.

The new shovel fixture simulates the native equip overwrite and directly invokes the production
shovel callback. Collision fixtures execute the production pre-call and late-pin fragments with
native types and an instrumented Actor_UpdateBgCheckInfo boundary. Source review also traces
the real generic ripple and later wading branches. These checks are source-level evidence,
not an observed configured game run. Wolf/giant precedence was inspected in source rather
than exercised in the new fixture.

## Apply and test

The combined package's checksum-gated apply_checkpoint.py accepts the matching Nov 7 cumulative
source or that source with the hidden UI checkpoint already applied. It refuses conflicting
affected files before writing. Keep an isolated source copy/branch, run --check first, then apply
and rebuild normally. The source.patch is for the unpatched matching cumulative baseline;
use the installer when the UI checkpoint is already applied.

Run from the complete source tree:

    python3 -B tests/winter_feedback/run_tests.py
    python3 -B tests/seasons/run_tests.py winter_water
    python3 -B tests/mm_nei/run_action_tests.py
    python3 -B tests/seasons/run_rod_lifecycle_tests.py
    python3 -B tests/seasons/run_oot_rod_stow_tests.py
    python3 -B tests/ui_visibility/run_control_input_tests.py
    python3 -B tests/ui_visibility/oot_run_tests.py
    python3 -B tests/ui_visibility/mm_run_tests.py

Smallest decisive in-game checks:
1. From empty hands, press the shovel button once near a hidden grotto, release, and wait for
   the impact. Repeat after holding another item and with the shovel already in hand.
2. In owned Winter, walk from snowy dry ground onto a frozen outdoor pool: hear snow then ice
   and see no walking/landing ripples. Jump, walk off, return and change seasons.
3. Enter a water-bearing temple during saved Winter, walk/land on a pool, and confirm ice audio
   with no ripples. Dry temple floors keep their native sound.
4. Confirm swimming below water, nonwinter water-walking boots, MM forms, iron boots and
   OoT always-thawed Zora's Domain retain their prior behavior.
5. Confirm startup navigation and inventory L paging still behave as the hidden UI checkpoint
   describes. A preview-only Winter selection must not grant frozen-water collision.

Await configured runtime evidence and user acceptance before any master promotion.
