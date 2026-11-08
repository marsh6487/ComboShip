# PR #87 remaining Ranch fixture dependency

Baseline: public `1ed68de6055036c37b214c928f73896168dd6713`, tree
`f622288cc408812a39ad6dc1707a23f130a46a29`.

Build Artifacts run `42429872593` passed Linux and Windows packaging, fast source
checks and five regression groups. The systems group passed every preceding
task, then its final Ranch fixture failed to compile `Player_InCsMode` and
`Player_SetupTalk`. The final aggregate check reflected that same failure.

The Ranch runner reuses the night-music host fixture, which now requires actual
native player-talk predicates/setup through `NATIVE_PLAYER`. Unlike the night
runner, it had not filled that placeholder. The correction adds `s32` to its
existing function extractor and injects the unchanged native
`Player_InBlockingCsMode`, `Player_InCsMode` and `Player_SetupTalk` bodies in
dependency order. The C parameter `this` is renamed for C++ compilation.

The CI error was reproduced before correction. The complete Ranch suite now
passes, including queued custom-bank ownership and live-disable handoff. The
complete night-music suite also passes both host adapters, native queue identity,
cancellation and owner preservation, lifecycle/dialogue controls and BGM table
order. An independent reviewer passed the Ranch runner and confirmed every
existing assertion and production body is retained. The exact configured CI
task is also verified locally before publication.

Only this document and the Ranch runner change. Production audio, night-dialogue
behavior, saved selections, assets and CI policy remain byte-for-byte unchanged.
cor explicitly deferred further daytime/nighttime audio changes until testing
the current nighttime hint correction. Daytime continuity is still unresolved;
source/CPU checks do not establish audible behavior in the user's configuration.
The severe Black Market slowdown is also unresolved.

The follow-up stays on the same draft PR with a separately restored recovery
archive. Fresh CI is required; no merge or accepted-master promotion.
