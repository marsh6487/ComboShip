# Kakariko Keaton Crash Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans for the bounded repair and a final independent review.

**Goal:** Correct the demonstrated Keaton melee ABI crash and Pictograph hover-name mismatch, and determine the token pickup failure boundary.

**Architecture:** Preserve crash build 91496f5b88760c55a6fa1a340593ea31fbc174a3 in an isolated candidate. Put the existing four-argument native melee contract in functions.h, then make Keaton use it. Route the Pictograph name through the established shared custom-name resolver. Investigate token contact separately; make no claim that unrelated symptoms share a cause.

**Tech Stack:** C, C++, Python native regression fixtures, Git.

**Spec:** User crash dump/logs and reported camera turn, night, Keaton, missed Skulltula pickups after restart, and Pictograph icon with Lens hover name.

## Global Constraints

- Preserve accepted masters, archive bytes, moveset values, randomizer receipts, and original/combined-game guards.
- Keep implementation/static proof separate from configured Windows runtime proof and acceptance.
- Save an independently restorable source checkpoint before handoff.

## Review Focus

- Ground B and airborne B must pass play/player/quad/damage without argument shifting.
- Native melee helper and Keaton must share a compiler-checked declaration.
- Lens-only inventory must retain its native hover name; Pictograph mode must select its own.
- Crowded token pickup must retain cutscene, hookshot, and randomized reward gates.
- Do not treat absent runtime proof as acceptance.

### Task 1: Keaton melee ABI

Files: soh/include/functions.h; soh/mods/transformation_masks/keaton_form.cpp; tests/keaton_melee/run_tests.py; scripts/ci/regressions.json.

- [x] Compile the real native melee definition with the real Keaton ground and aerial entry functions; verify the current three-argument declaration fails.
- [x] Declare void func_80837918(PlayState*, Player*, s32, u32) in functions.h, remove Keaton's private declaration, pass play at all four call sites.
- [x] Exercise ground combo, airborne kick, jump-to-kick, both quads, cutscene guard, and no-input control with sanitizer; add the fixture to CI.

### Task 2: Pictograph name

Files: soh/mods/extended_inventory.c and a focused native name-resolution fixture.

- [x] Exercise the actual custom-name resolver with Picto ownership/mode on and off; observe Pictograph mode's missing override.
- [x] Add its existing MM Pictograph name resource to the shared resolver; preserve Lens fallback and mode switching.
- [x] Run the focused name/receipt checks.

### Task 3: Token boundary and handoff

- [x] Inspect EnSi collision admission and reward dispatch, supplied restart evidence, and crowded-scene limit. If evidence is insufficient for a repair, record the remaining uncertainty and add only useful bounded tracing if justified.
- [ ] Review diff and native checks, commit bounded repair, create baseline/candidate recovery bundle with hashes and restoration instructions, and verify independent restoration.
- [ ] Deliver exact candidate status and unresolved runtime checks without requiring new user testing to complete this investigation.
