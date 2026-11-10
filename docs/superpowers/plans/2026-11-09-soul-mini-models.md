# Enemy and NPC Soul Miniatures Implementation Plan

> **For agentic workers:** Use superpowers:subagent-driven-development with explicit
> file ownership. The user's existing parallel-workflow authorization and current
> implementation request cover execution without another design approval pause.

**Goal:** Render all enemy and NPC soul miniatures in both ComboShip hosts with
separate live simpler-model preferences and reproducible render checks.

**Architecture:** Reuse the owner-described animated GI bridge for native and
foreign pickups. Add bounded static/composite recipe support for non-skeletal
models and share each owner's catalog between its native and foreign draw paths.

**Tech Stack:** Existing C++20, native OoT/MM graphics and resource managers,
append-only C draw ABI, Python test drivers, native C++ renderer fixtures.

**Spec:** `docs/superpowers/specs/2026-11-09-soul-mini-models-design.md`

## Global Constraints

- Baseline published R21 `0063920e18145299afc1682aab98407ebd366fc5`.
- Detailed default; independently controlled NPC and enemy categories; boss unchanged.
- Canonical owning-game resources; no gameplay actors or new item/save identities.
- Preserve all shuffle/gameplay behavior and original/Combo compilation guards.
- Source-only checkpoints, no game assets; separate fixture/compile and gameplay proof.

## Review Focus

1. A group/name alias selects the wrong native character: independently enumerate
   expected catalog IDs and verify recipe ownership and representative asset paths.
2. A valid path has the wrong skeleton/animation layout: check actual loaded native
   resources and reject invalid limb/table data before emitting graphics.
3. Toggling simple/Alt models leaves a stale cached draw: test same-frame preference
   changes and owner Alt selection changes in both receiving hosts.
4. Non-skeletal enemies bind incomplete palettes/materials: exercise actual bounded
   recipes and both host consumers, including all eight MM exceptions.
5. One draw contaminates the next: verify inherited transforms, stack balance,
   segment cleanup, resource scope restoration and missing-resource fallback.

## Task 1: Shared renderer and foreign consumption

**Ownership:** `combo/menu/ComboItemDrawABI.h`, `ComboForeignAnim.h`,
`ComboForeignDrawOOT.h`, `ComboForeignDrawMM.h`, related new common render headers,
and `tests/soul_mini_models/renderer*` / shared renderer fixtures only.

**Interfaces:** Existing `CwItemAnimDrawInfo` remains append-only. Publish new
bounded composite fields to the two catalog workers immediately. Native and foreign
callers continue using `ComboForeignAnim_Draw(info, owner, play)`; ordinary skeletal
recipes remain compatible. The two host producers remain owned by Tasks 2 and 3.

- [x] Add failing detailed/simple cache and composite-render checks.
- [x] Extend bounded recipe/renderer support for actual static parts, matrix palettes,
      extra material/texanim state needed by the owner catalogs.
- [x] Preserve ownership and fallback selection through the receiving-host caches.
- [x] Verify both host render consumers, malformed inputs and cleanup.
- [x] Commit only owned files and record evidence/limitations.

## Task 2: OoT enemy and NPC recipes and native routing

**Ownership:** new `combo/menu/ComboSoulMiniOOT.h`,
`combo/menu/ComboItemDrawOOT.h`,
`soh/soh/Enhancements/randomizer/draw.cpp`, and OoT catalog tests/evidence.

**Interfaces:** Consume common recipe fields from Task 1 and
`ComboSoulModels::SimplerEnemy()` / `SimplerNpc()` from Task 4. Export an ID-based
native descriptor helper alongside the existing name-based foreign exports so
native and foreign pickups share one catalog. Existing animal/misc souls may
retain their existing presentation.

- [x] Establish independent coverage and representative-resource assertions for all
      43 enemy and 46 NPC identities; confirm the baseline fails.
- [x] Derive each model, pose, segments, scale and centering from native sources.
- [x] Route native Combo and both foreign exports through the detailed recipe unless
      the appropriate simpler control is checked; preserve simple fallback and
      existing MM imported enemy aliases.
- [x] Verify catalog, native/foreign selection and compile the affected native TUs.
- [x] Commit only owned files and record evidence/limitations.

## Task 3: MM NPC recipes, all enemy cases and native routing

**Ownership:** new `combo/menu/ComboSoulMiniMM.h`,
`combo/menu/ComboItemDrawMM.h`, `mm/2s2h/Rando/DrawItem.cpp`,
`mm/2s2h/Rando/DrawFuncs.cpp` if needed, MM catalog tests/evidence.

**Interfaces:** Same common recipe and settings functions as Task 2. Reuse the
existing native boss helper pattern for the new native catalog path. Describe
all eight formerly unsupported enemy cases for both hosts, using the exact native
special draw requirements rather than substituting another enemy.

- [x] Establish independent coverage and representative-resource assertions for all
      53 NPCs and 47 enemies, especially the eight special cases; prove baseline red.
- [x] Implement NPC recipes from native actor/resource definitions.
- [x] Complete special enemy material/composite recipes, preserve existing native
      enemies, and apply the independent simpler enemy preference.
- [x] Verify catalogs, native/foreign toggles and affected native compilation.
- [x] Commit only owned files and record evidence/limitations.

## Task 4: Controls, integrated checks, review and recovery

**Ownership:** root; new `combo/menu/ComboSoulModelSettings.h`, UI files,
`combo/gui/WhatsNew.h`, test orchestration/asset validation, CI manifest, docs.

**Interfaces:** `ComboSoulModels::SimplerEnemy()` reads
`gRandoEnhancements.SimplerEnemySoulModels`; `SimplerNpc()` reads
`gRandoEnhancements.SimplerNpcSoulModels`; both default zero. Expose matching labels
in both game UIs, alongside relevant visual randomizer enhancements, using these
same CVars. Live rendering only; no new seed options.

- [x] Add two independent controls and What’s New entries with clear tooltips.
- [x] Verify both gold accents through actual native checkbox code and pinned ImGui
      in both game tabs, including on/off styling, input, state and layout.
- [x] Run every recipe through independent native asset/resource validation.
- [x] Run both receiving-host production render checks and toggle/fallback/cache
      regressions; compile the changed real translation units with actual dependencies.
- [x] Obtain independent review, fix material findings, and rerun decisive checks.
- [x] Save independently restorable source checkpoints after verified milestones.
- [x] Integrate onto the newest cumulative candidate without losing other work;
      report exact refs and remaining GPU/runtime evidence honestly.

## Source delivery status

All implementation and source-verification tasks above are complete. The isolated
candidate includes the latest inspected cumulative content (local 4608e863's sole
additional test fix is preserved as ce019e99), without rewriting the original
checkout or remote PR. Reviewed renderer repairs are committed as bd9b8397 and
material-factory repairs as 390d5b5e. The real-resource Gomess material correction
is dd6ca9c2, and the focused two-game gold-accent gate is 750c29f1. See
`docs/poc/soul-miniatures-verification-20261109.md` and the recovery manifest for
final evidence and exact delivery refs. A newly linked binary and the documented
in-game visual acceptance matrix remain separate pending runtime work.
