# October 29 combined fixes publication plan

> **For agentic workers:** Use superpowers:executing-plans for this bounded integration; obtain one independent whole-candidate review before publication. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Publish one recoverable candidate containing the completed actor, missing-music-data, and OoT/MM chest fixes, with a downloadable Windows build.

**Architecture:** Preserve repaired Unbound parent `3cbb22d1f09e85deb206d2b085fc62105bc09fad`. Start from actor donor `4ca64d89b6a1fce834f8b234518ab227c5f92aa7`, cherry-pick the completed music and chest donors, and retain the already verified graph-fixture ABI update from the held combined snapshot.

**Tech Stack:** C/C++, Python native regression fixtures, GitHub Actions Windows/Linux builds.

**Spec:** The user's October 29 authorization, recorded in `docs/poc/morning-fixes-publication-20261029.md`.

## Global constraints

- Candidate publication only; no protected-branch merge or public release.
- Preserve existing integrations, accepted masters, original model/scene/music archives and game guards.
- Masks use Major art; fairies use Minor art and small size when contents-based sizing is enabled, independently of style settings, on both engines.
- Preserve concrete reward identities and progression categories.
- Report source verification, platform compilation and configured runtime acceptance separately.

## Review focus

- Same-joint skeleton switches with different matrix counts in either direction and swap path.
- Authored versus native head pivots, including static actor callbacks.
- Missing versus valid music sample data in both engines and solo/Combo modes.
- Native/randomized/foreign fairy and mask rewards, independent size/style settings and fallback.
- Existing graph-fixture resolver ABI and retention of every prior CI group.

## Task 1: Reconcile exact sources

- [x] Verify clean isolated actor donor workspace and run its native graph/skeleton/tracking fixtures.
- [x] Cherry-pick music `7edef8a9d35b1fcc996bafb1ec87402ad7307799` and chest `e2a999b59f911e489818a30b38f1120fe668e775`, `ccc0d4621a1b77f6bd30f71aafea76cb00db4ff8`.
- [x] Reproduce the existing entry-fixture ABI failure, carry its two verified fixture files from held snapshot `8cffd2ad09fa737522687f508c77e24384d82f01`, and add its command to the existing render group.
- [x] Compare integrated production files against the exact donors and the held combined snapshot; inspect all differences.

## Task 2: Verify and review

- [x] Run graph/skeleton/tracking, entry-fixture, missing-sample normal/sanitized, MM chest and OoT chest regressions.
- [x] Run affected existing rendering/audio checks where supported locally; retain full platform and all six regression groups in CI.
- [ ] Check CI manifest preservation, whitespace/conflict markers and changed-file formatting. Manifest and whitespace checks pass; the exact pinned formatter runs in CI.
- [x] Obtain independent read-only review of the combined diff; resolve material findings with focused regression evidence. Also reproduce and fix the donor's prior full-build vertex-type error; the new real-resource fixture and all affected actor tests pass.

## Task 3: Publish and deliver

- [ ] Commit the source references, checks, runtime test list and rollback instructions; push only `candidate/morning-fixes-20261029`.
- [ ] Verify the remote commit/tree and restore the bounded source change independently against the exact parent.
- [ ] Open a draft PR against `candidate/cel-unbound-smoke-repair-20261029`; let the existing PR workflow run CI without a release.
- [ ] Follow failures to resolution, verify the Windows artifact's exact commit and provide its download URL with the remaining runtime checks.
