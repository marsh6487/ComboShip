# Recoverable checkpoints

The user requires frequent durable checkpoints because interrupted chats have lost substantial coding work.

- Commit coherent sections as they are completed.
- During an authorized publishing task, push a recoverable checkpoint at least every 10–15 minutes of active coding and before a long build, test or upload. Save explicitly marked work in progress when a section remains unfinished.
- A local commit or a created Git tree alone is not a durable checkpoint. Verify the remote branch commit and its tree after publishing.
- Keep tracked recovery notes with the starting commit, restored versus reconstructed source, exact verification results, open findings, limitations and next steps.
- After an interruption, resume from the newest verified remote checkpoint. Check the live PR head and preserve concurrent corrections; do not repeat completed implementation or verification merely because chat context or scratch files disappeared.
- Checkpoint permission stays within the user's existing repository and publishing scope. It does not authorize merging, promoting a baseline or publishing unrelated work.

For the current ComboShip work, recovery branch `checkpoint/mm-runtime-review-20261005` and `docs/poc/RECOVERY_MM_RUNTIME_REVIEW_20261005.md` contain the handoff. PR #34 remains draft. Existing authorization covers its checkpoint and candidate pushes; no merge or master promotion is authorized.
