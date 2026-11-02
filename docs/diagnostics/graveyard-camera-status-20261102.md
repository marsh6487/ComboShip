# Graveyard camera investigation retained from November 1

The recording's snap at about 2.166658 seconds resembled the first authored
Graveyard camera pose. Projection of the two visible torches supported the
match, but several camera rows used that pose and the recording did not reveal
the active setting/index or its selecting caller. The runtime cause was not
established.

The inspected Graveyard recipe retained type-2 room geometry, cameraMovement
zero and floor camera indices 2/3/4. It does not traverse the type-1 multi-image
background helper repaired by the separate Market camera guard. The Market
guard must not be described as a fix for this Graveyard snap.

The original diagnostic kit, `ComboShip_Graveyard_Camera_Trace_20261101.zip`, has
SHA256 `615392282c85a68753dd94282a84d6a38a2b74d755a58696d76dc374c0a7ae6f`.
Its `graveyard-camera-trace.patch` has SHA256
`4650d5f23a7c6ee8dd20f64ddf985999aaef8c0dfcc6f8b489e6b2cc9c5a2014`.
It was prepared against the same source tree
`b42f00013cddb567cdf134fee6c469c879f36e7b` as PR #77's pinned base.

That optional trace records entry, dispatched camera function, applied view,
accepted setting/mode/background-index requests and authored pose reads. Donor
evidence includes standalone/combined helper tests, complete native camera
object compilation and independent reviews. Those checks did not prove the
game's camera-loop cause or logger performance. The unchanged Camera_Demo5
pointer-to-SFX-ID compiler warning was recorded in both donor versions.

The cursor/Market integration in PR #78 intentionally leaves `z_camera.c`
unchanged. The next useful Graveyard evidence would distinguish an internal
camera selection from a change in the final applied view during the same snap.
Existing saves and accepted scene archives were not edited by this integration.
