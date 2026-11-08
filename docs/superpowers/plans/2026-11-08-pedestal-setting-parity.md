# Lost Woods ritual setting parity

Requested result: both ages at the Lost Woods Master Sword stump follow the Temple of Time's repeatable time-travel sequence when Skip Story Cutscenes is enabled or disabled.

Baseline: combined UI/shovel/winter candidate 40123a27fee98e547d486de37da7a928198e97a9, with unchanged pedestal boundaries captured at 5d1740f. The prior checkpoint and accepted source remain recoverable.

- Preserve the complete native child/adult player cues, four departure camera splines, animation/sound handoffs and 20-frame scripted scene fill.
- With Story Skip on, mirror the native terminator's frame >20 early exit directly into the normal white transition. Do not add the local pure-white eight-update fill.
- With Story Skip off, complete the source script to child frame230/adult210, then use the same normal white transition.
- Restore the native 20-update first-pose arrival hold in both ages. Reuse the native hop/flourish action.
- Preserve local room10 placement, authored fixed arrival camera, selected sword/hands, progression-free age/equipment swap and the separately accepted manual B convenience shortcut. The Temple's one-time first-story Sheik/Light Medallion branch is not appropriate to this reusable local prop.
- Validate both ages, normal/randomizer saves and explicit/default setting values; retain reset/teardown, camera/equipment and progression tests.
- Compile full actor/player TUs in solo and Combo guards. Deliver a combined recoverable source checkpoint; do not claim in-game visual acceptance or publish/merge.
