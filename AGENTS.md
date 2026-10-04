# Checkpoints for long running work

The user requires frequent recoverable checkpoints after interrupted sessions lost substantial coding work.

- Work on isolated branches and commit coherent sections as they are completed.
- During an authorized publishing task, persist those sections to a clearly named GitHub checkpoint branch before continuing a long operation. Target a checkpoint at least every 10–15 minutes of active coding, including an explicitly marked work-in-progress snapshot if the section is unfinished.
- A local commit alone is not a durable checkpoint. Verify the remote branch commit and tree hash after publishing.
- Record the base commit, recovered versus reconstructed source, exact test commands/results, limitations and next work in a tracked recovery note. Keep incomplete checkpoints clearly identified; do not describe them as build-accepted candidates.
- Resume from the latest verified remote checkpoint after an interruption. Do not repeat completed sections merely because chat context is missing.
- Checkpoint authorization remains within the user's stated repository and publishing scope. It does not authorize merging, promoting a sacred baseline or publishing unrelated content.

Current task: `docs/superpowers/plans/2026-10-04-final-polish-reconstruction.md`. Checkpoint branch: `recovery/final-polish-20261004` in `marsh6487/ComboShip`; final candidate target remains PR #34.
