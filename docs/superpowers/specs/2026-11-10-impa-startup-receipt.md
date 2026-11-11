# Impa startup receipt

## Approved intent

When starting a ComboShip randomizer file with Zelda's Letter and Letter shuffle disabled, visibly identify the actual item from **Song from Impa (OoT)** in the foreground starting game. Run that game's get-item cutscene when its existing category/skip settings require it. Always identify the item and source even when the animation is skipped.

## Baseline and preservation

- Local baseline `beba199a5258c1ae36550ab8879f36d079d641fc`, tree `da99bc7dd7191661e4dae8f829b2a98d2f22602b`; identical to published PR90 `a5b1abd697d284ecbca70c13091562821dfc3feb`.
- Keep Letter shuffle eligibility, gate/story flags, other starting inventory, solo and vanilla modes, ordinary pickups, existing GI models/cameras/fanfares, and all cumulative candidate changes.
- No publication or master promotion is part of this implementation request. Deliver recoverable source and named verification evidence. No GIF/video previews in recovery archives.

## Design decision

Use a durable, value-owned startup receipt in the combined save container. Separate awarding from displaying: presentation callbacks must never grant inventory, dispatch generic receive hooks, replay a trap, change equipment, or trigger item-specific story transitions.

Keep OoT-owned starting grants at their existing save-creation point and freeze the actual resolved entry/name before granting. The existing source initializes the MM save *after* the OoT starting-grant block, so an MM-owned Impa reward must instead wait until its matching MM save and starting inventory are initialized. Resolve and grant that one reward there. Atomically record its concrete identity, the resulting MM save, and the OoT save containing any shared effect in one container write. Do not resolve a progressive MM item against the previous/dormant slot.

The MM award uses a local receipt value, then publishes READY under the container mutex before native save callbacks run. Subsequent READY calls capture saves only. Drain existing OoT save workers before arming the required fresh post-award OoT capture; earlier queued saves cannot satisfy that proof. Keep native contexts resident until both post-award snapshots exist, and retry failed container writes without another grant or shared-item fan-out.

Offer the pending receipt at the first safe foreground gameplay frame. Retain it across loading, canceled presentation, scene changes and save/reload until it is visibly acknowledged. A completed receipt does not replay on game switches or reloads. If the disk acknowledgement fails after visible presentation, remember that visibility in the running process and retry persistence without repeating the presentation in either game. A process exit before that failed write succeeds may repeat the receipt after restart, but never repeats an already committed reward. Existing saves without the new record receive no inferred/backfilled reward.

Reuse native OoT actorless GI presentation and MM CustomItem scheduling. Use a bounded startup token to suppress all OoT inventory/story/equip effects while retaining native model, pose, camera and fanfare. MM uses a presentation-only actor callback and `DrawResolvedItem`, or the existing foreign renderer addressed by the full frozen descriptor. Preserve raw draw identity for progressive custom items and static or animated trap disguises. Accept MM's native GID_NONE draw sentinel while preserving strict bounds for other fields. The actual resolved item controls classification; no new cutscene checkbox is introduced.

Show the item first and the source second. Full GI uses its native item textbox. A skipped GI gives nonblocking corner feedback, including junk items, so choosing a skip setting does not introduce a replacement blocking textbox. Native per-game category settings continue to control animation.

## Alternatives considered

1. Put Impa into the ordinary check queue again: rejected because the check is already saved and a replay can grant another progressive tier.
2. Show only a corner notification: does not satisfy the requested native GI presentation.
3. Persist a receipt, freeze identity and reuse native presentation without awarding again: selected. MM initialization ordering requires deferring its owned reward until the correct destination save exists.

## Verification and acceptance

Exercise both foreground hosts, both item owners, all supported native skip settings and junk/minor/major classes. Check progressive identity stability, exactly one award, canceled actor retry, no replay after acknowledgement, slot/seed changes, malformed/absent records, MM-first loading, and no duplicate trap/equipment/story effects. Run the affected existing receipt/delivery regressions and compile native changed boundaries where available. Runtime visual acceptance remains separate from source/native-fixture verification.
