# Din's Fire publication without source PNGs

> Execute inline under the user's explicit publication authorization. Use the
> executing-plans workflow; retain one independent review before publication.

**Goal:** Publish the complete reviewed November 10 evening candidate while
omitting its two Din's Fire authoring PNGs and keeping required validation.

**Architecture:** Keep all native asset bytes and the resource manifest intact.
Add a read-only native-output validation mode to the existing encoder; retain
source regeneration as an explicit operation requiring separately supplied art.

**Tech stack:** Python, existing texture parser, unittest, GitHub Git data API,
existing Windows/Linux build and regression workflows.

**Spec:** The user's November 11 publication authorization in this conversation.

## Constraints

- Preserve published parent `a5b1abd697d284ecbca70c13091562821dfc3feb` and all
  reviewed source identified by original tree `d8c6eec2b54168c82bd7a7f4b77388adb5f392f2`.
- Exclude only `tools/dins_fire_remake/flame_ribbons_luminous_rgba.png` and
  `tools/dins_fire_remake/flame_source.png`, plus their upload requests.
- The filtered reviewed delta is 464 paths / 319 blobs / 145 binary blobs.
- Keep PR #90 open and draft; no merge, master promotion or release.
- Preserve original/combined guards, all prior regressions, gameplay and assets.

## Review focus

- Both hosts can contain identical corruption: hash validation must reject it.
- A modified manifest must not bless changed resources.
- Missing/extra resources and changed timing/declarations must fail.
- CI must work without the PNGs; regeneration must still require genuine inputs.
- Publication must preserve the base and exactly bind paths, modes and hashes.

## Task 1: Restore and validate without source art

- [x] Verify recovery archive and manifest hashes; independently reconstruct the
  original reviewed tree using the unchanged published baseline.
- [x] Restore the reviewed delta with the exact two exclusions.
- [x] Add a real CLI regression and observe failure for missing native mode.
- [x] Add `verify_committed_resources(root: Path) -> dict` and `--check-native`.
- [x] Exercise successful checks and corruption/missing/provenance controls.
- [x] Wire the existing Din gate to native checks and retain every other command.
- [x] Document optional exact-source regeneration; verify it independently.

## Task 2: Verify and publish

- [x] Check the final diff, byte preservation, registry and relevant local gates.
- [x] Independently review the bounded source/CI changes.
- [ ] Commit the candidate and record its complete manifest/tree.
- [ ] Save and independently restore a durable source checkpoint.
- [ ] Reuse native blobs; publish verified text/tree stages and a leased branch update.
- [ ] Read back parent/tree/head and draft/open state; update the PR description.
- [ ] Complete Windows/Linux/resource/AppImage builds and all required regressions,
  repairing scoped integration defects if exposed; record build links and runtime limits.
