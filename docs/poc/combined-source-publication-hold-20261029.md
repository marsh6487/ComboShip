# Combined source held for 2Ship investigation

User instruction: combine the completed music crash repair with the actor candidate, then investigate the 2Ship side before publication.

Latest published actor candidate: `candidate/native-cutscene-vertices-20261029`, commit `4ca64d89b6a1fce834f8b234518ab227c5f92aa7`, draft PR71. No music-combined source has been pushed.

Integrated donor: `ComboShip_Missing_Music_Data_Source_Fix_20261029.zip`, based on the same repaired parent `3cbb22d1f09e85deb206d2b085fc62105bc09fad`. Both donor AudioSampleFactory.cpp files and all added test/document files match the donor manifest SHA-256 identities. The donor CI manifest was merged additively, retaining actor checks and all 259 prior groups, for 260 groups total.

Also retained: the verified resource-entry fixture callback update and its focused regression, which correct the old fixture ABI found by world CI. No actor or model fix was overwritten.

Local checks after combination: shared graph sanitizer suite; all seven skeleton cases; all 1,920 tracking checks; sanitized resource-entry graph fixture. The music donor includes 108 normal and 108 ASan/UBSan checks and independent review, recorded in its supplied verification logs. The music suite has not been rerun in this narrow local snapshot. A full combined platform build and configured game testing remain pending.

Publication is intentionally on hold. Preserve the already accepted repaired executable, actor source checkpoints, original model archives, and the separate completed music donor. The next work item is the user's forthcoming 2Ship investigation; do not infer its cause or scope from this hold.

## OoT/MM mask and stray-fairy chest routing

The user's final policy is to preserve OoT's existing Major classification for stray fairies and mirror it in 2Ship. Masks also use Major art. Both reward groups are large when the existing size-matches-contents setting is enabled, and retain authored size when disabled. The earlier Minor/small experiment is superseded; its local commits remain in history. Exact repaired-parent MM and OoT source/test files were recorded first so this narrow snapshot retains reviewable patches.

The corrected MM and OoT suites passed in the complete source worktree, including UBSan. OoT tests all five concrete fairy rewards and executes the unchanged real foreign-category mapper; its complete actor compiles for ordinary OoT and `COMBO_BUILD`. Revised Major/large assertions failed on the superseded implementation before correction. Independent review found no issues. The sparse combined snapshot has not had a full platform build or in-game test.

The final chest production diff against the repaired parent is only MM's two model-table entries, both selecting Major. OoT's three production paths and MM's size predicate match the repaired parent exactly. No archive, item category, reward delivery, audio repair, or prior actor repair changed. Runtime verification remains pending. Gameplay publication stays on hold; source-only recovery checkpoints are authorized separately. Keep all local source and checkpoints intact.
