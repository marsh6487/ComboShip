# NEI Editor and MM Wolf Link Implementation Plan

> **For agentic workers:** Use focused parallel tasks for the independent editor, Wolf core,
> and skin renderer changes. The root integrates the host and reviews the complete candidate.

**Goal:** Grant every current NEI item through the MM save editor and make Shadow Crystal
activate the existing Wolf Link gameplay in MM.

**Architecture:** Reuse randomizer grants for editor changes. Port the OoT Wolf runtime into
MM and connect it to the existing custom-form input, update, render and teardown seams.
Keep the shared skin changes compatible with existing Pikachu clients.

**Tech Stack:** C/C++, MM native structures, SSBB CPU skinning, Python/C++ regression fixtures.

**Spec:** `docs/superpowers/specs/2026-10-04-nei-editor-wolf-design.md`

## Global constraints

- Baseline `91aa4f9a`; preserve ongoing GI/icon work in its separate checkout.
- No save layout changes, rewritten seed placements, model reauthoring, or master promotion.
- Source/build checks and game-runtime acceptance are distinct.

## Review focus

- Blank saves receive full-width item IDs and all rune/season flags.
- Repeated grant-all is safe and preserves ordinary inventory fields.
- A missing/invalid Wolf asset never hides or freezes Link.
- Native masks and scene transitions leave no Wolf collision or input ownership behind.
- Pikachu retains root-motion, scale and animation behavior.

## Tasks

- [x] Editor: production failing fixtures for the missing grants; current-table catalog and
  individual controls; production grant tests; investigate Hourglass pool filtering.
- [x] Skin: numeric failing fixtures for root-motion modes, scale, fractional frames and
  paw positions; port required donor facilities; preserve Pikachu settings.
- [x] Core: compile and execute donor-derived Wolf loader/combat fixtures on MM types;
  adapt MM collision, movement, audio and input names; fail safely on invalid assets.
- [x] Host: failing input/lifecycle fixtures; connect Shadow Crystal C/D-pad activation,
  native-form exclusion, update, movement, render and cleanup; verify fixtures.
- [x] Integration: inspect full diff; real-header syntax checks; NEI regression suite;
  independent review and fixes; commit a named candidate with remaining runtime checks.

## Execution notes

- Baseline test first attempt stopped at missing `nlohmann/json.hpp` in the new worktree's
  compiler include environment. Reuse available dependency headers; this is environment
  setup, not a discovered gameplay regression.
- Final 40-command `run_combo_nei_regressions.sh` gate passed on 2026-10-04, including
  the full MM player syntax check and sanitized Wolf core fixture. The complete
  SaveEditor syntax check with `--full --combo` also passed. Dependency headers were
  supplied through `CPATH`; missing `stb_image.h` and `json_fwd.hpp` were retrieved from
  their real upstream sources for the check environment.
- Independent review findings were fixed: duplicate editor pickup bookkeeping, uncached
  icons, shared skin pose contamination, raw equipment-input conflicts, active Pendant /
  Beetle / hookshot arbitration, immediate contextual A, world effects and pickup position.
- Keep the candidate on its isolated branch. No master promotion or executable/runtime
  acceptance follows from these source checks. See `docs/testing/MM_NEI_WOLF.md`.
