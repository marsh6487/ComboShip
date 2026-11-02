# Recovery and native OoT quest-song investigation

## Verified recovery

Cumulative parent: `3796288605989833335193fbb2a85d1603656692`, PR #78.
Its Windows and Linux game builds passed. Required status remained red because
the items donor fixture expected the obsolete truncated boss-soul icon operand.
This follow-up requires the safe `ITEM_CUSTOM` operand and captured catalog
resource, 32x32 RGBA32 dimensions and single-box presentation instead. Existing
localized body/color and save-immutability checks remain.

The interrupted Combo Vanilla Task 1 checkpoint is now durable at
`517d408f5b7850f6ab0c7f72e28ea6539335a778`, tree
`8a78fc489edcbb358ab61afff579c19badc4449b`, branch
`candidate/combo-vanilla-icons-20261102`, draft PR #79. All 20 published blob
identities match the local payload; complete-tree comparison against parent
37962886 is ahead 1 / behind 0 with exactly those 20 files. The four native
ASan/UBSan suites passed again with leak detection disabled for this host.
Restore by checking out that branch, then follow the WIP checkpoint and plan.
This is paired-save foundation only, not a complete playable Vanilla mode.

## Report and source findings

cor confirmed that the friend was playing **OoT**, had received Minuet and
Nocturne, and could play them, but they appeared locked/absent on the OoT quest
page, preventing pause-menu instant play/warp.

Both native quest draw and manual playback's success branch check the same
`inventory.questItems` bits. Minuet is bit 6, Nocturne bit 10, combined mask
`0x440`. Native Item_Give sets those bits; FleetSync OR-merges the same field.
No native grant/render ownership split or song-specific hiding rule was found.
Do not infer a grant repair from this evidence.

Two existing selectors replace the native layout: persisted
`gEnhancements.SkijerNEI.MmQuestPage` and `pauseCtx.randoQuestMode`. A failed MM
background probe can retain OoT parchment while MM contents still draw. These
are plausible explanations requiring runtime confirmation, not diagnosed causes.

This source adds a read-only `[QuestSongs]` log when the quest page opens or its
ownership/layout changes. It includes slot, raw quest bits, Minuet/Nocturne,
both layout selectors, pause state, cursor point/slot, song index and both PausePlay/PauseWarp settings.
It neither grants songs nor changes the existing instant-play/warp behavior.
The trace is not emitted on every stable frame.

cor could not yet confirm whether manual playback actually reaches the teleport
prompt. Keep ownership loss and display failure both open until that is known.

## Evidence and next reproduction

The native fixture compiles the actual song grant arm and actual quest-song
draw loop, exercising all twelve grants, idempotence, unrelated-bit retention
and all 4096 ownership masks. The actual PauseWarp selection handler is checked
at its activation boundary for all six warp songs, ocarina/A/ownership guards
and shuffled-button requirements, including Minuet and Nocturne. Textbox and
transition effects remain outside the fixture. It also compiles the actual trace, checking layout
changes, reopen/slot changes, bounded logging and read-only behavior. ASan/UBSan
passed. Graphics submission is captured; installed archives, GPU output and the
full game loop are outside this fixture. Missing trace in the prior source is
the negative control; this does not claim to reproduce the original bug.

For the next reproduction retain a screenshot of the complete quest page, the
exact running build/commit and its `[QuestSongs]` line while the songs are
missing. Confirm whether manual play merely produces notes or actually reaches
the song-success/teleport prompt in that same session. `minuet=1 nocturne=1`
with both layout selectors zero directs the next investigation to rendering;
cleared bits direct it to grant/load/team resync. Harpoon's full team-inventory
replacement is a separate possible loss path if multiplayer is active, not a
confirmed cause here.

No accepted archives or masters changed. Full engine-build results for this
follow-up and configuration-specific runtime proof must be tracked separately.
Combo Vanilla Tasks 2-5 remain pending in its implementation plan.

Independent review found no consequential issues in the trace or corrected
soul fixture. The real donor builder/receipt executable passed a scoped local
run with generated entrance-graph coverage excluded because local GUI/log
dependencies could not be installed. This is not the full items gate; GitHub CI
retains the complete graph and all existing checks.
