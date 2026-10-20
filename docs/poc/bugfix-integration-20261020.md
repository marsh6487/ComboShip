# October 20 bug-fix integration candidate

This is an isolated source candidate on `fix/bugfix-integration-20261020`.
`develop` and the previously accepted gameplay baseline are unchanged. cor
authorized implementation, source checkpoints and publication to
`marsh6487/ComboShip`; publication does not establish configured gameplay
acceptance.

## Source inputs

| Input | Exact local source | Integration |
| --- | --- | --- |
| Current cumulative/platform/Saria-placement baseline, PR45 | `613802447cf8a4fbc3e058ba084f9e3dd7b9f7ae` | Parent of all inputs |
| Geometry-only native forest/Black Market and real donor bundle | `9d5f550d98f46ff50523a8f6f6a011eddf56e1c8` | Initial candidate |
| Black Market structural access, manifest lookup and rejection diagnostics | `1d19e86cd` | Merged without conflicts |
| Native transformation camera, shield-light teardown and NPC resource/TLUT repairs, PR46/47 source | `a01321fe0` | Merged without conflicts |
| Completed elemental damage, selector/HUD and finish source revision | `412058d4c` | Merged without conflicts after its verification record arrived |

The first combined checkpoint is local commit
`a2b774ec0c0ef528fc35d23022e50376f9d5219c`, tree
`a6363ff62c95aaeae0c9440d2431b72c060b4ed6`. It contains both Black Market
checkouts and the camera/NPC/crash input, before the elemental and new
particle/dialogue/input changes. Published snapshots must reproduce their
recorded local source tree exactly; local merge history is retained separately.

## Preservation contract

- Keep native forest geometry, collision, moved mirror stump and existing
  portal/door routes, with Saria beside the portal. Native sources use stock
  textures; no new scene texture bundle is introduced.
- Keep the existing native/combined/solo game guards, actual door ownership and
  player-safety checks. Purchases retain seed/session validation. Empty stock
  and optional presentation packs do not bar physical shop entry.
- Preserve original model inputs, accepted audio, poses/props/blink, progression,
  save layout and source provenance. Repaired model archives remain separate
  installs; source publication alone cannot replace an installed old NPC pack.
- Woods of Mystery remains limited to its existing bridge/tunnel/seasonal
  integration. No Woods of Mystery scene archive is added.
- Elemental immunity must leave the ordinary sword strike's damage intact.
  The reported built-in Biggoron casing case remains an open configured runtime
  check; the elemental checkpoint does not claim that screenshot is resolved.

## Required evidence before a full build

cor specifically requested entry tests before the build. The environment has
no runnable Linux game executable, Wine or Xvfb. The available Windows DLLs and
port archives belong to an older build and cannot prove this candidate's
gameplay. Production-function executable tests against actual generated scene
resources are recorded separately from live game entry/exit, drawing and
interaction.

The candidate's entry evidence must cover the native MM shop and solo OoT
private scene, authored and stock forest routes, valid structural resources,
optional pack independence, genuinely missing destination rejection, empty
stock, exit/re-entry and retained safety checks. A generated seed or native
object compilation alone is insufficient.

Full hosted builds remain held for the live entry-proof requirement. Draft
publication uses a CI-skip commit so creating the reviewable source PR does not
silently start the long Windows/Linux builds.

## Verification record

The integration verification results and exact reproduction commands are
completed after the independent particle, entry and dialogue/input tasks have
finished. See their focused POC reports as well as the inherited elemental,
native forest, camera, NPC and Black Market diagnostic records for the
distinction between source checks, native objects and configured gameplay.

## Recovery

Restore from the published candidate source commit/tree or the original
checkpoint refs in the table. Original input archives remain unchanged. Discard
only a bounded new change when a regression is found; retain the independently
verified inputs. No accepted runtime master is promoted by this integration.
