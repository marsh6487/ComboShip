# Magic Mirror followups native fixtures

Run `python3 tests/magic_mirror_followups/run.py` from the source root.
Requires Python 3 and a C++17 compiler with AddressSanitizer/UBSan (default: g++).

These compile the actual engine-specific extended-button implementation and
shared Magic Mirror player implementation. The OoT fixture also executes the
exact final equip block extracted from the native pause overlay, including its
production helper call. Engine state and the external animation/audio/icon APIs
are minimal fixtures; no game assets or window are required.

Checks cover every distinct C/D source/target pair, empty/native/extended
displacement, Rod of Seasons relocation between all D-pad directions, ordinary
items moved onto Mirror, old duplicate cleanup, full
u16 IDs, unrelated sentinel slots, B/age/form preservation, and native-only
equip ownership. Both player variants verify one existing chime at actual
casting start, silence for rejected casts and cancelled put-away, no repetition
in animation updates/readiness, and no additional release/arrival chime.

The original commit fails on the OoT move, MM duplicate cleanup, and both chime
checks. ASan/UBSan are enabled; LeakSanitizer is disabled because this container
cannot expose the process task list. This is source/native-fixture evidence,
not a full ComboShip build, Windows runtime test, audible quality assessment,
or game acceptance.

In game, move the Rod of Seasons or Mirror between D-pad directions over empty
and occupied buttons. Expect one D-pad mapping for the moved item, a swap with
an occupied target, and preservation of any separate C binding. Moving the
item to C consumes its old mappings. Check that Mirror's accepted outdoor
cast plays one brief treasure-appearance chime when Farore's casting motion
starts, and that cancellation and ineligible casts remain silent.
