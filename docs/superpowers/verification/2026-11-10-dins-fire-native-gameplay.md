# Native Din's Fire integration handoff

## Status and scope

The accepted appearance study has been implemented in the actual OoT and MM
player, actor, rendering and private-resource paths. The source candidate is
`candidate/dins-fire-usable-linger-20261110`. Publication is held for later
combination with cor's separately active vanilla NEI bugfix source.

The protected pre-linger checkpoint is
`28cd69306bfb4a8d13cac8f5d8810ef85758d306`, tree
`2dc62dab292882801112e71b996123b2aaef12a1`. It remains recoverable in
`ComboShip_Dins_Fire_Native_Pre_Linger_Checkpoint_20261111.zip`. The new candidate
extends the steady hold for practical stick/arrow use. Its recovery archive's
manifest records the exact captured commit/tree, including this handoff. No
merge, push, PR, master promotion or overwrite of the separate bugfix worktree
occurred.

The six-second implementation tested for this revision is
`20c1075e18a699db69ea0113f68165e9313fc208`, tree
`4a883c65a551d115cd4a07b71dd87d7dc718bd91`. Only the updated specification,
plan and this handoff follow that implementation checkpoint in the final
capture. The selected registered CI manifest and complete run output are
included in `dins-linger-final-ci-*` evidence; full translation-unit results
are recorded in `dins-linger-native-syntax-verification.json`.

This is source and native-harness verification. A linked application build,
configured gameplay session and native visual acceptance are still pending.
CMake and Ninja were unavailable in the implementation environment. The
accepted preview establishes the artistic target, not proof of native runtime
behavior or the exact physical hand-contact pose.

## UI and cosmetic controls

Both host menus place **Din's Fire Remake** under **Enhancements → Graphics**,
immediately after **Fire Damage → Ice Damage → Light Damage**. The checkbox
uses the existing `gEnhancements.DinsFireRemake` setting, defaults to false,
and uses its existing exact-CVar `WhatsNew::Kind::New` entry for the gold accent.
The normal What's New display preference remains respected.

The tooltip describes the revised cast, normal gameplay camera, earlier return
of player control, and the lingering fire's stick/arrow interactions. The choice
applies to the next cast. Both transition directions are latched: a stock cast
stays stock if the checkbox is enabled, and an active remake cast completes its
existing lifecycle if the checkbox is disabled. Missing assets at setup use the
native stock path; losing assets during an active remake uses the same-age
original sphere as a visual fallback.

The original **Din's Fire Primary and Secondary** cosmetic entries remain the
color controls. OoT reads the existing Changed/Value entries; MM uses its native
`CosmeticEditor_GetChangedColor` interface. No separate color preference was
introduced.

The default palette preserves the accepted full-color flame artwork. Custom
palettes select neutral RGB companions with identical independent alpha and
use the native combiner relationship:

```text
fire RGB = Secondary + (Primary - Secondary) * texture luminance
fire alpha = texture alpha * private vertex alpha
```

Primary controls the brighter fire; Secondary controls the darker fire. The
contact light and lingering local light read those same entries during updates.
The approved green startup remains green. White halos and sparks retain their
neutral-texture color mapping. Resetting the editor colors restores the exact
default artwork path. Live cosmetic changes do not alter cast timing, geometry,
collision, magic ownership or resource lifetimes.

## Native timing and gameplay

The normal gameplay camera remains active throughout. There is no spell camera
request and no explicit return-camera request. The two camera angles in the
appearance studies were only review views.

| Native update | Time from cast start | Event |
| --- | --- | --- |
| 0–12 | 0–0.60 s | Existing charge animation, green startup and pending actor spawn |
| 13–20 | 0.65–1.00 s | Existing strike animation reaches its frame-30 marker, clamped to the loaded end frame |
| 20 | 1.00 s | Contact signal, ground flash and local light |
| 21 | 1.05 s | Contact pose held; fire released at the captured point |
| 22–27 | 1.10–1.35 s | Existing recovery animation |
| 27 | 1.35 s | Normal player action restored while fire remains active |
| 21–146 | 1.05–7.30 s | 8 growth samples followed by 118 steady-hold samples |
| 147–158 | 7.35–7.90 s | The unchanged 12-sample burnout |
| 159 | 7.95 s | Fire, attack/ignition registration and local light expire |

The fire lasts 6.9 seconds from release. It provides six seconds after player
control returns before burnout, plus 0.6 seconds of fading fire. Durations are
native 20 Hz update counts, independent of draw frame rate. Opening the ordinary
inventory pause skips actor updates in both hosts, so choosing equipment while
paused does not consume this window. Moving, drawing the item, aiming and firing
use the unpaused time. The actual animation resources are retained and playback is
bounded to the accepted timeline. Frame 30 is an existing source marker; an
in-game pose check must confirm whether it visually matches hand contact for
the configured character and animation assets.

The actor captures the cast location once. It no longer follows Link after
release. The world's actor categories continue updating during the cast;
player-only protection is refreshed briefly while preserving longer existing
invincibility. Ordinary equipment handling can resume while the fire actor
continues to own the spell's magic state.

`combo/render/DinsFireTiming.h` supplies the shared actor, renderer and asset
timing. The original 8-update growth and 32-update strong-damage falloff remain
unchanged, followed by the existing one-damage tail throughout the extra hold
and burnout. Longer collision availability can still produce extra weak hits,
depending on target invulnerability. The cast consumes magic once and owns the
shared magic state longer, so other magic actions must wait for cleanup;
ordinary sticks and normal arrows remain available.

The fire's native attack cylinder remains active during its visible lifetime.
A held Deku Stick is lit when its actual tip enters that volume, using the
same 210-update initial burn and 200-update refresh rules as a native torch.
Ordinary player arrows are converted to the existing normal-lit-arrow type
with the native fire damage flag. This does not consume Fire Arrow magic or
replace an elemental projectile. The previous/current arrow positions are
tested as a segment against the finite fire cylinder, so a fast arrow can
cross the whole volume within one update. Held, dead, embedded, already-hit,
enemy and unrelated projectiles are excluded.

MM appends new actors. Only a tagged native Din actor moves to the later MISC
category, through the engine's deferred migration, so arrows fired after the
cast move before fire ignition is evaluated and before collision resolution.
Ownership lookup covers both the old and new lists during migration. OoT's
existing insertion order already puts newly fired arrows before the fire.

Magic ownership continues through the killed-but-still-linked interval until
actor destruction. MM stale-state recovery cannot arm a new spell that would
then be cleared by the previous fire's destroy callback. A successful native
reservation interrupted before actor spawn is released by the normal action
transition; rejected requests, foreign magic and linked actor owners are
preserved. Pending actors abandoned before release are bounded and cleaned up.
Light removal is idempotent, including expiration, cancellation and a full
native light pool.

## Rendering and asset contract

The renderer retains the accepted six irregular curved ribbons, each with 32
angular segments and three vertical cells. Horizontal UV motion and geometric
oscillators use continuous actual elapsed age throughout the longer hold. Growth,
burnout erosion, peeling widths, alpha profiles and deterministic embers retain
the preserved recipe coefficients. Burnout envelopes and ember events shift
4.30 seconds later, with ember anchors sampled from those later ribbon positions.
There is no repeated hold loop or slowdown. The renderer applies no full-screen
tint.

The accepted 1536×1024 RGBA source remains byte-identical:

```text
9705407ffa7226f6d4f704a7b20575d66e4aa704759ef3a0670309de78fc186e
```

Each host has 144 resources: 74 default-color resources and 70 neutral companions. The
neutral views derive their luminance from already encoded RGB, while every
paired alpha byte and 92-byte resource header remains identical. All 12 resources
independent of burnout timing remain byte-identical, including the base rows,
halos/trail, and protected first flame/spark textures. Only the 132 derived
default/neutral burnout resources are rebaked at actual fire ages 127–137;
the previous samples at ages 41–51 remain in the immutable pre-linger checkpoint.

| Resource payload set | Decoded bytes per host |
| --- | ---: |
| Original/default | 36,664,320 |
| Neutral companions | 36,312,064 |
| Both complete sets | 72,976,384 (69.60 MiB) |

These are asset payload totals, not measured runtime residency. The renderer
selects the required core family and only the current six burnout variants;
it does not load every fade texture on every draw. Missing optional burnout
variants fall back to the matching base row with horizontal scrolling restored.
The current deterministic private asset archive is 31,904,120 bytes with SHA256
`3f0c4a6f83520733bfab0e3de82fe0ec6b75c34dcfbf9079935e84add951b13c`.
Its identity is also recorded in the recovery manifest and asset evidence. See
`tools/dins_fire_remake/README.md`, its manifest and
`pre_linger_resource_provenance.json` for the complete recipe, timing-header
identity, preserved resource identities and intentionally replaced samples.

The renderer allocates private vertices, two matrices and an exact-sized child
display list from the existing OPA tail, then references it from the XLU head.
This fits MM's real smaller XLU arena without resizing any global pool.

| Measured default maxima across three camera bases | Value |
| --- | ---: |
| Quads / private vertices | 596 / 2,384 |
| Total OPA allocation | 134,752 bytes |
| Executed commands | 4,883 |
| XLU head submission | 9 commands / 144 bytes |
| XLU head reservation | 512 bytes |
| Mesh scratch structure | 50,408 bytes |

MM's actual OPA/XLU capacities are 421,888/65,536 bytes; OoT provides 16 MiB in
each. Both palettes at all 138 samples and three camera bases fit both hosts
with a fixture reserving half of each arena for other work. That is a stress
case, not measured room occupancy. The custom palette's measured OPA maximum
across the same three camera bases was 134,352 bytes. These are independent
maxima, which need not occur in the same sample.

## Verification and its limits

The registered Din CI gate covers deterministic assets, preserved geometry,
actor lifecycle/ignition, gameplay transitions, actual GBI submission, fallback,
MM resource bindings and every manifest-listed resource through both native
texture factories. The final package includes the exact selected manifest,
run output, tested implementation commit and exit status.

Specific evidence includes:

- The original 40/12 recipe remains an explicit parameterized control against
  eighteen recorded reference keyframes across three camera bases, covering
  16,578 quads: maximum position difference 0.000216423 world units, recipe UV
  difference 0.000005, and vertex alpha difference one byte.
- The new 138-update lifetime is verified separately: initial 41 updates match
  the preserved recipe, every steady-hold update keeps the accepted angular and
  unwrapped UV speed, and the unchanged fade envelope is shifted 86 updates.
- The actor fixture verifies full radius, alpha and light across the added hold,
  unchanged damage samples, late stick/arrow ignition at fire age 100 and no
  attack or ignition on expiration at 138.
- Production actor and action functions compiled against native host layouts,
  including the actual animation advance and both hosts' action transitions.
- Actual native category insertion and MM deferred migration in the arrow
  scheduling fixture, plus first-crossing ignition before collision flags.
- Native GBI decoding of the child list, materials, every private vertex,
  sorted quads, matrix restoration, exact end-of-list and arena boundaries.
- Actual encoded texture samples evaluated through the packed combiner for
  primary-only, secondary-only, blue/green and equal-endpoint palettes, during
  hold and burnout. Default RGB, alpha and geometry are preserved.
- Both real texture factories accepted all 144 resources in each host: 288
  imports. An independent audit checked 9,078,016 neutral companion pixels.
- Full OoT and MM player translation units pass native syntax checks in combo
  and original modes. The MM player unit includes its complete actor/router
  code. The separate OoT MagicFire unit also passes in both modes.
- Menu binding probes use both hosts' real checkbox paths, checking default
  false, saved values, shared toggles, callbacks and the existing New entry.

Sanitizer checks use AddressSanitizer and UndefinedBehaviorSanitizer. Leak
scanning is disabled because LeakSanitizer is incompatible with this runtime's
ptrace supervision. The gameplay runner additionally excludes only shift-base
UBSan because unchanged native player flags contain signed `(1 << 31)` forms.
These qualifications are recorded in the evidence rather than treated as a
clean full-program sanitizer result.

The configured runtime still needs to establish the physical contact pose,
camera feel, transparency intersections, real scene memory occupancy and GPU
cost. Whole-quad centroid sorting approximates the reference compositor's
per-pixel ordering. Half-resolution texture encoding and sampled 20 Hz burnout
are explicit native approximations. The fixtures do not run an entire scene's
`Actor_UpdateAll` with real arrow movement and native collision resolution.

## Recovery and combining the separate NEI source

The final source archive contains a cumulative binary patch/bundle from the
published baseline `a5b1abd697d284ecbca70c13091562821dfc3feb`, a native increment
from `cd12567fe1c0d64a205e1d360165cb000166f159`, complete changed sources and
assets, exact identities, and restoration instructions. Both patches are
verified through fresh independent indexes against the captured tree. No
preview movies are included.

Preserve cor's bugfix branch and restore this candidate to a separate branch.
Combine the branches in an isolated integration worktree. Review and resolve
the player, spell logic, menu, What's New registry and regression-manifest
changes together; copying whole files over the bugfix source would discard
independent work. Use the native increment only when its exact base is already
present. The bundle is the preferred route for a merge retaining history.

After combination, rerun the Din gate and the affected NEI regression gates,
build the full intended target, and test both games with their matching assets.
Check normal-camera casting, hand contact, move/equip after recovery, a stick
tip, an arrow crossing and hitting a torch/enemy in the same update, expiration,
repeat casting, both color controls, stock mode and mid-cast setting changes.
Only a later explicit publication step should promote the combined result.
