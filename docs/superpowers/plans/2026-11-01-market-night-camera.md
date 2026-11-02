# Market night camera crash implementation plan

> **For agentic workers:** Use `superpowers:executing-plans` for this bounded
> source task. The native regression is delegated; production edits belong to
> the root agent and the completed candidate receives independent review.

**Goal:** Prevent image metadata access for a door or free camera while keeping
valid fixed-camera backgrounds and room geometry unchanged.

**Architecture:** Preserve the multi-image hook call, then gate interpretation
of camera image metadata on the existing renderer eligibility condition.
Keep eligible lookup behavior and entrance-hook camera backup/restoration.
Treat an absent background as absent without suppressing geometry.

**Tech stack:** C room renderer, Python native test runner, repository C ABI
adapter, AddressSanitizer and UndefinedBehaviorSanitizer.

**Spec:** `docs/superpowers/specs/2026-11-01-market-night-camera-design.md`

## Global constraints

Exact upstream tree: `b42f00013cddb567cdf134fee6c469c879f36e7b`.
Do not modify assets, save data, camera selection, cross-game behavior, or cursor
source. No remote mutation or runtime/master acceptance is implied.

## Review focus

- Observed `CAM_SET_DOOR0`: no image metadata access; prior player parameter stays.
- Free camera: no background access; opaque and translucent geometry remain.
- Valid fixed camera: image ID remapping and player parameter update stay intact.
- Hook veto: no null image dereference; geometry remains available.
- Native ABI and solo/combined configurations: use actual project declarations.

## Task 1: Room renderer guard

**Files:** modify `soh/src/code/z_room.c`; add
`tests/oot_room_background/run_tests.py`; register the focused test in
`scripts/ci/regressions.json`.

**Interfaces:** existing `func_80096A74(PolygonType1*, PlayState*)` and
`func_80096B6C(PlayState*, Room*, u32)` remain unchanged.

- [x] Run the actual-function native fixture against the original source and
  preserve the failing door/free-camera evidence plus passing fixed control.
- [x] After the existing hook in `func_80096A74`, return no image for a camera
  other than `CAM_SET_PREREND_FIXED`. Require a non-null image in the caller
  before reading image fields. Do not change geometry conditions or hook calls.
- [x] Run the focused sanitizer suite for solo and combined configurations;
  verify fixed background remapping, hook veto, and geometry controls.
- [x] Review the source/test diff independently and address substantive findings.
- [ ] Commit the bounded candidate, restore its delta into a separate baseline,
  and rerun the same focused verification.
- [ ] Save the delta and investigation handoff with baseline identity and limits.

## Separate progress recovery investigation

The log proves the key's MM identity receipt persisted. It does not expose the
restored OoT inventory, receipt counters, or check flags. The user subsequently
reported deleting that save, so recovery is deferred and no save was edited.

## Verified milestone

The original eight-case fixture produced four expected failures and four passing
controls. The completed portable suite passes ten cases in both standalone and
combined configurations (20 total) under ASan/UBSan, including post-hook camera
changes. Independent review repeated the 20 passing cases without blockers.
The registered focused test also passed through the repository CI runner.

Full executable build and gameplay remain untested. The separate Graveyard
camera snap is under investigation and is not claimed fixed by this change.
