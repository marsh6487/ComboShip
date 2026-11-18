# Sheikah Slate normal put-away

Baseline: `d3cf01741084c188eb0a01a263018fa3de23fbe1`; additive unpublished candidate.
The user reports that only drawing and then sheathing a sword clears the Slate.

Both games omitted the Slate from `CustomItems_HasStowableHeldItem` and
`CustomItems_PutAwayHeldItems`. The Slate lives in the extended u16 item space
and deliberately has no native held-item action, so native put-away/HUD guards
could not recognize it by the ordinary action index. A subsequent sword action
incidentally stowed it through the Slate's blocking path.

The native stow classifier now recognizes `Slate_IsDrawn`, and the shared cleanup
calls `Slate_PutAway`. The wrapper clears the drawn/cast/hold state and MM's forced
right-hand grip, closes only the Slate-owned rune wheel, cancels held Cryonis
placement targeting, releases the Slate sensor consultation when present, and
clears Stasis offer highlighting. It does not delete a placed bomb or ice pillar,
release a frozen Stasis target, stop the bike, revoke runes, or modify ownership.

Because Slate reads raw ext-button presses directly rather than using the u8
item input state machine, it has a local same-frame/held-button release latch.
The press that coincides with native stow cannot immediately redraw or cast the
tablet. Passive rune updates continue above that latch. An unrelated item wheel
is left to its own controller. The existing native A/Put Away path is reused;
no sword equip workaround or new binding is introduced.

Verification: `tests/nei_item_stow/run_slate_stow_tests.py` compiles the actual
Slate teardown/latch, native classifier and `Player_PutAwayHeldItem` entry with
real native Player/PlayState layouts in SoH and MM, standalone and Combo. All
four ASan/UBSan runs pass. Unrelated item/rune/UI services are explicit fixture
boundaries. Tests cover a Slate with no native held action, cast/hold clearing,
MM grip restoration, own versus unrelated modal UI, same-frame and held-input
release, idempotence and preserved persistent-effect services. The immutable
baseline `--ref d3cf01741084c188eb0a01a263018fa3de23fbe1` fails the actual stow
classifier assertion. The existing `tests/nei_item_stow/run_stow_tests.py` gate
passes for native action guards, HUD behavior and other item input latches.

No full executable, controller/HUD gameplay test or user acceptance is claimed.
Runtime review should draw the Slate with the sword sheathed, use normal Put
Away, then check repeat drawing and each owned rune without losing placed effects.
