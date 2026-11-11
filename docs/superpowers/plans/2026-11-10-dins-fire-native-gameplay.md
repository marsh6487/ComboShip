# Native Din's Fire Implementation Plan

Goal: implement the accepted spell in both hosts with normal-camera casting,
early player recovery and a persistent interactive fire actor.
Spec: docs/superpowers/specs/2026-11-10-dins-fire-native-gameplay.md.
User explicitly authorized execution; publication remains held.

## Owned workstreams

1. Rendering worker: combo/render/DinsFireRemake.h and DinsFireRemakeDraw.h,
   corresponding draw/geometry tests and their diagnostic runner. Port the
   accepted mesh/charge/burnout to native GBI with bounded arena use, sorting,
   full-color named RGBA resources and safe unchanged-path fallback.
2. Gameplay worker: OoT z_player.c and MM item_oot_spells.c plus a dedicated
   production-action test fixture/runner. Implement no camera request, release
   signaling, early action handoff and MM live-magic-owner protection.
3. Asset worker: deterministic asset encoder, tools/dins_fire_remake sources
   and manifest, private asset directories in both hosts. Preserve accepted
   atlas, provide agreed named RGB/alpha resources and verify host byte parity.
4. Root: shared actor state contract, both native MagicFire actor files/header,
   persistent lifetime/position, lights, stick and swept-arrow ignition, actor
   regression fixtures, integration review, checkpoints and recovery package.

## Dependency contracts

Rendering and actor workers agree on an explicit fire-age draw entry; actor
updates provide elapsed native ticks independent of player motion and render
FPS. Gameplay and actor workers agree on release/ownership signals before code
integration. Assets and renderer agree on names, formats and logical sizes.
Each worker edits and stages only its owned paths. No bulk staging or remote
operations. Use production behavior tests that fail before implementation.

## Verification and recovery gates

- Establish existing native render and binding tests as controls.
- Verify no camera calls for remake Din, unchanged other-spell paths, actor
  origin after Link moves, 138-update lifetime, six unpaused seconds after
  recovery before burnout, early control and magic cleanup.
- Test stick tip overlap, swept ordinary-arrow crossing, misses, expired fire,
  unrelated projectile types and repeat contacts using real integration logic.
- Verify draw arena exhaustion/rollback, resource failure, host guards, texture
  determinism, both-host compilation and protected medallion behavior.
- Independent review of combined changes, meaningful narrow regression gates,
  full build attempt when available, and honest runtime limitations.
- Commit independently verified milestones. Preserve source checkpoints before
  a long build/context handoff, then final exact-tree restoration verification.

## Accepted steering during implementation

- Persist the actual anchored actor, including stick-tip ignition and swept
  ordinary-arrow ignition, after player recovery.
- Keep the optional checkbox off by default; place it immediately after Light
  Damage in Enhancements/Graphics in both hosts with the existing What's New
  registry accent. The setting is latched for each cast in both directions.
- Preserve existing Din Primary/Secondary cosmetic controls. Add neutral
  companion texture data with alpha identical to the accepted RGBA assets for
  accurate custom palettes, and use the same native palette for point lights.
- Verify native MM category ordering, cleanup ownership through actor deletion,
  pre-spawn action cancellation, and the real graphics arena capacities. These
  checks address concrete integration issues found during independent review.
- Extend only the steady hold for practical stick/arrow use. Keep startup and
  control at updates 12/20/21/27; start burnout at fire age 126 and expire at 138.
  Use one pure timing header for actor/render/asset recipes. Preserve original
  strong-damage falloff and the existing weak tail during the added hold.
  Re-bake fade frames 127–137 at their actual elapsed age, retain original
  recorded geometry as a parameterized recipe control, and sweep all 138 native
  samples for continuity, GBI allocation and late ignition. Preserve the prior
  52-update candidate in its immutable pre-linger source checkpoint.
