# 2Ship adult Link item framing

## Authorized scope and baseline

cor asks to port SoH adult get-item framing to adult Link/Din in 2Ship because the received item intersects the adult head. Latest explicit instruction: leave Zelda's Letter and gate code alone and implement only adult framing.

Baseline: published PR90 commit `fdbfb868a5d70d73a29ba79b1ac7701c122fff37`, exact tree `639ce09af9df72adda2913b000d7486aa7278b51`. Preserve all cumulative source and assets. Work on isolated branch `candidate/mm-adult-gi-framing-20261110`. No public push, merge or master promotion is part of this source candidate.

## Source findings

MM's `Player_GetHeight` returns44 for the human form, even when the adult overlay is drawn. Its modified collider/ageProperties do not affect that camera value. `CustomItem00_Update` also chooses a fixed human receipt height, yielding a model origin at worldY+53.3 during the held receipt. The adult `Player_Draw` branch returns before submitting native GI models.

The existing `AdultLink_UsesAdultPresentation` query correctly requires local player, human transformation, ready non-child rig and active presentation, but includes short custom forms. Restrict the new receipt behavior to ordinary adult Link plus alternate assets such as adult Din (`CustomForms_ActiveForm()==CUSTOM_FORM_NONE`). Leave the existing broad predicate untouched.

## Approach

Use the native SoH framing and hand anchor, translated into the existing MM camera and actor code. A camera-only adjustment would leave the item embedded in the model; changing global human height would also change unrelated gameplay/cameras. Both are excluded.

- Ordinary item camera: only `CAM_SET_ITEM0` with the local loaded ordinary adult. Native OoT adult height is68 at model scale0.01. Respect the existing adult model scale, and account for render YOffset as a translation rather than as a change in physical size. OoT `CAM_ITEM_TYPE_9` target offset is0.1*height*yNormal, radius0.5*height*yNormal, pitch-20 degrees, yaw preserved, FOV60, interpolation0.25 and timer6; yNormal=0.5+0.5*(68/height). At default scale/offset this is targetY+74.8, radius34. Translate yaw behavior to MM `KEEPON4_FLAG_3|KEEPON4_FLAG_2|KEEPON4_FLAG_1`; never copy OoT's literal0x2540. Preserve MM HUD/letterbox, ownership, collision and return lifecycle.
- Large chests: under `CAM_SET_LONG_CHEST_OPENING`, supply ordinary-adult height to the existing MM `Camera_Demo2` final framing. The native default chest arrays already match SoH; retain their sequence/timers and MM's Goron branch.
- Custom held receipts: after their existing20 hidden frames, position the actual model at the average of `leftHandWorld.pos` and right-hand body-part position, +14Y and +3.3 along facing, just like the native GI. Subtract `shape.yOffset*scale.y` from the custom actor's worldY because Actor_Draw adds it later. Preserve the existing0.007 actor scale, callback scale30, spin, reward callback timing and all native-form behavior. Use actual hand fields: GI_SHIP/GID_NONE does not reliably refresh the native global GI reference. Update-stage uses the prior rendered pose; disclose this narrow behavior and require visual observation before acceptance.
- Custom overhead receipts: no raised-hand pose exists, so retain existing bob/timer/scale with the SoH adult lift20 at default scale; honor adult scale/offset consistently. Do not add this lift to native EnItem00, which already has an adult branch.
- Native GI: after adult skeleton drawing, call the native GI draw with the existing `getItemDrawIdPlusOne > GID_NONE+1` condition. GI_SHIP uses GID_NONE so custom receipts must not double draw.

## Preservation and verification

Preserve child human, child-rig/custom forms, Deku/Goron/Zora/Fierce Deity, peers, unavailable adult-rig fallback, non-item cameras, shop return lifecycle, resource selection/shimmer, grants and save behavior. No OoT production files, gate/letter code, textures, assets, general movement/collision camera, seed logic or save schema changes.

Write native production-function regression fixtures before code changes. Demonstrate baseline failures for ordinary adult receipt position/camera/native-GI submission and passing child/native-form controls. Verify adult guard, adjusted model scale/offset, ordinary/large-chest scoping, custom callback timing, native-vs-GI_SHIP draw guard. Reuse existing native camera/math structures; fixture boundaries and untested full renderer must be named. Run adjacent adult-rig and camera/Black-Market lifecycle tests when dependencies permit. Independently review final source and recover exact source checkpoint. Compilation/native fixtures are separate from full game build and configured adult Din visual acceptance.
