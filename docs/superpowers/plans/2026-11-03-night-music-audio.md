# Optional night music and audio rows

User-authorized scope: optional daytime scene music at night with ambience and
native audio ownership preserved; Hyrule Field's dedicated night override has
priority. Move its editor row below daytime Hyrule Field and apply existing
release accents to all three custom rows. Defaults remain off.

Baseline: 2d65d33659fd9ef0c9f1812cab8f0dfb33332d98 (includes Jabu repair).
Candidate: candidate/night-music-audio-20261103. No scene archive changes.

Design: native time/ambience code keeps running. OoT's optional BGM uses SUB
while nature remains on MAIN, releasing SUB for combat, fanfares, previews,
cutscenes and positional music. MM retains its separate ambience player and
starts the ordinary scene sequence on MAIN. Both adapters use one lifecycle
policy, track ownership, clear on scene destruction, and defer to native forced
music. Sequence replacement happens once; virtual scene rows never reach queues.
Native Final Hours and scenes without a timed day/night ambience cycle remain
native. No audio-spec/player-count expansion or ambience substitution.

Plan:
- [x] Add failing lifecycle/order tests covering toggle, dawn/dusk, scene changes,
      competing owners, dedicated-field priority, refresh and full-width IDs.
- [x] Implement shared lifecycle and host adapters; preserve existing HF updater.
- [x] Order editor rows without changing keys and add checkbox/release metadata.
- [x] Run affected runtime, real-ImGui, sequence queue, scene-music and Jabu tests;
      compile affected native units where dependencies allow; review the diff.
- [ ] Commit audio separately; preserve a durable recovery checkpoint and publish
      a candidate PR with truthful source/build/runtime status.

Runtime acceptance: listen to both hosts across sunset/sunrise, enable/disable,
scene re-entry, custom selections, combat/fanfare/cutscene/preview return and
dedicated Hyrule Field priority. Automated fixtures cannot prove audible quality.

Verification record:
- Sanitized shared lifecycle and complete production-updater fixtures pass in both
  hosts: custom IDs, one request per pending start, native owner yield, toggle,
  dawn, scene reset, preview/fanfare/combat and dedicated priority.
- Native OoT command queue passes exact-ID, pending-start cancellation and later
  native-owner preservation checks. Legacy dedicated night combat/preview checks pass.
- Native ambience entry points reproduce protected Goron City/Windmill handoff
  refusal before the repair; after ownership-aware immediate release both pass.
- The actual BGM table loop failed its required-order assertion before correcting
  the integration, then passed with day followed immediately by night. Collection
  IDs and CVar keys are unchanged. Native collections/scene routes pass both hosts.
- Full five-test What's New suite and real ImGui audio menus pass. Eight affected
  native units compile; both adapters and MM cache compile without COMBO_BUILD too.
- Jabu's 48 routing cases pass and its source file has no audio-branch diff.
- Independent review found the table integration and protected-track handoff;
  both were corrected with failing-then-passing regressions. No master promotion.
- Full platform linking, audible game checks and host freeze/resume are untested.

Recovery: baseline commit above is unchanged. The candidate and matching
checkpoint/night-music-audio-20261103 refs preserve this complete source tree;
clone either ref, or revert the single audio commit to recover the parent.
There are no required new assets, archives, dependency changes or settings migrations.
