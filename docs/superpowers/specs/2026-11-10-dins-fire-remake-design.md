# Din's Fire remake effect — POC1

## Intent and baseline

cor requested an iteration of the Din's Fire spell effect based on the supplied
OoT remake clip, including authored textures and green pre-cast particles.
The clip was inspected directly; it begins mid-charge and lasts 2.738 seconds.
Its exact remake/engine and full cast duration are not established.

Source baseline: published PR #90 head
`a5b1abd697d284ecbca70c13091562821dfc3feb`, tree
`da99bc7dd7191661e4dae8f829b2a98d2f22602b`. This is the exact tree of the local
`beba199a5258c1ae36550ab8879f36d079d641fc` recovery checkpoint. The POC lives on
`candidate/dins-fire-remake-poc1-20261110`. Prior cumulative work is preserved.

## Visual target

1. Two fine mint-green sparkling trails curl around the caster and converge on
   the casting hand. A pulsing white-green star concentrates there.
2. The star turns warm orange, then becomes a compact hand flame before release.
3. Ignition expands into a hollow, ragged fire shell/ring: bright golden cores,
   orange edges, large transparent openings, and smaller moving flame filaments.
4. Material motion continues during the native hold. Dissipation thins/breaks up
   sheets rather than presenting an opaque, uniformly fading ball.

The clip's purple enemy shards and white impact streaks are separate effects and
are not part of this spell iteration. Its blurred social-video borders are not
part of the effect. Source/texture implementation is not an in-game match claim.

## Approach and bounded scope

Texture-only replacement cannot create the missing pre-cast phase. Replacing the
whole spell actor would unnecessarily risk magic timing and fire interactions.
Use a narrow visual adapter in both hosts, with privately named resources, and
an optional `gEnhancements.DinsFireRemake` checkbox (default off). This is the
implementation choice for an independently reviewable POC, not user acceptance.

The player charge begins before the fire actor exists. Add a Din-only draw hook
for that initial phase, then draw the final lead-in from the existing actor's
20-update pre-cast state. Do not spawn the damaging actor earlier. The actor's
zero scale during this phase requires a fresh world transform.

Prefer shared deterministic geometry/timing and small native renderer adapters.
Draw-only sparkles/ribbons avoid extra actors and never consume gameplay RNG.
Use bounded per-frame vertex/matrix allocations; do not mutate shared geometry.
Use named texture submissions, freshly resolved readiness, and the native draw
path when the option is off or required resources are unavailable.

## Resource contract

Resources are available in both hosts under private names:

- `__OTR__custom/dins_fire_remake/flameTex`: physical 256 x 256, logical 64 x 64.
- `__OTR__custom/dins_fire_remake/sparkTex`: physical/logical 64 x 64.

Both are version-1 whole-image textures, type 6 (I8 semantics), flags 3,
horizontal/vertical scales 1.0, with intensity replicated to RGBA. The flame
art is newly authored with image generation; the spark is a small exact native
glint primitive. Keep source art and deterministic encoding recipe. Bundle raw
resource files in each host's `assets/custom/custom/dins_fire_remake/` directory.
No override of shared gameplay_keep, the old Din sphere texture, or private
medallion textures is permitted. Alternate Assets remains usable in either mode.

## Protected behavior

- Preserve damage, collider radius/height, magic reservation/consumption/reset,
  actor spawn, cast duration, animation tables, screen tint and sound timing.
- Preserve MM frame-30 cast protection release and fire-arrow damage flag.
- Restrict the shared MM actor change to `ACTOR_OOT_DINS_FIRE`; the Fire Medallion
  continues its existing appearance, textures and live colors.
- Preserve sword/shield flames, their floor-clearance correction, and elemental
  combat/grass interactions. Preserve the trident's independent dome.
- Existing Din primary/secondary cosmetic colors still influence the fire.
- The candidate adds a visible control and What's New accent in both hosts and
  synchronizes it in Combo mode using the existing settings mechanism.
- No public push, PR update, merge or master promotion is part of this POC.
- Do not include videos or animated previews in the source recovery package.

## Evidence and acceptance

Verify real production draw output, finite/bounded geometry, early pre-cast
visibility, green-to-orange transition, option-off/resource fallback, both hosts,
medallion isolation, active fade and native gameplay updates. Verify the authored
resource headers, payloads, two-host equivalence and deterministic rebuild.
Use existing relevant renderer/weapon checks and native C compilation where the
environment permits. Record environment failures separately from test failures.

Visual acceptance needs the built candidate running with the option enabled:
child/adult (including Din), one open area and one fire interaction in each host,
then option off and Alt on/off. Compilation or command-stream tests cannot prove
final blending, animation attachment, floor contact or resemblance in game.

Retain the baseline and exact candidate refs/tree; deliver a recoverable narrow
source package with restoration proof and the remaining runtime checks.
