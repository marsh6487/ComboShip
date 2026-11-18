# Magic Mirror shortcut and ocarina ownership

The Mirror's once-per-frame shortcut polls raw C/D-pad press edges before native
player and message processing. It previously treated a note as a travel request,
then played the ordinary rejection sound when the instrument/message made travel
unsafe. Native item dispatch's instrument gates did not cover this separate hook.

The shared shortcut now declines input while the native ocarina playing flag,
forced-pull request, or ocarina item-change action is active. It also declines
the native instrument message-mode interval, covering startup, demonstrations,
lessons, recording, and the OoT instrument minigames. MM `stateFlags3_20` is the
native forced-pull request consumed by `func_80847994`; OoT names that request
`PLAYER_STATE3_FORCE_PULL_OCARINA`.

The guard is in `PressedButton`, before polling equipped Mirror buttons. It
does not clear or synthesize input, alter the ocarina or suppress the general
Mirror error sound. No blocked edge is queued: keeping a note held after exit
does nothing, and a new press retains normal Mirror activation. Active ritual
cleanup/cancellation still runs before this shortcut path.

Verified with `tests/magic_mirror/run_tests.py`: both real native layouts and the
shared runtime execute under ASan/UBSan. Coverage includes all seven C/D bindings,
all instrument message modes, playing/forced-pull/item-change phases, exact input
preservation, held input after exit followed by a fresh press, ordinary rejection
feedback, and interrupted active-ritual cleanup. Existing cast/choice/arrival,
pickup, camera, save and teardown checks continue to pass.

This is source/native-fixture verification. Configured in-game ocarina notes and
Mirror use after stowing remain the final runtime check; no gameplay clip or full
executable build is claimed.
