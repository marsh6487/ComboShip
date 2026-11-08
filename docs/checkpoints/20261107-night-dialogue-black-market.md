# Dialogue and Black Market source candidate, 2026-11-07

Parent is public cumulative PR #86, `2054eef2f12aeedfb2d2d0f5519cc23272227e4e`.
Candidate branch is `candidate/night-music-dialogue-20261107`; exact final head,
tree and per-file SHA256 hashes travel in the recovery manifest. All prior
cumulative integrations remain inherited. No master promotion or merge.

Changes:

- `8529e2ad`: ordinary player dialogue defers new night-track starts/edits without stopping existing playback. Real audio overrides and off/dawn cleanup retain priority.
- `7a02466c`: normalize retained OoT yaw to MM packed 180 degrees before linking the independent item-get cutscene. Exactly one generated scene byte changes; the other 213 prepared resources match the parent.
- The final bounded performance change skips locked-item bounds capture when seals are hidden and adds optional, shop-only MM RenderFlight. Named CPU spans cover keeper, room materials, shelves/effects, GI generation, model bounds and seals; renderer attempts and resource tables identify the submitted work. No full actor-attribution port or bounds cache was added.

The user reports the shop was smooth with magical materials, Granny's room and
stock textures before recent native-shop work. The historical comparison kept
that observation as a control: the native-shop conversion retained the same
eight-slot GI/bounds path; native stock actors have no draw callback; keeper
and stock initialization are one-time. No duplicate rendering, repeated
initialization loop or camera-related severe-cost defect was established.
Removing hidden-seal bounds scans is proven redundant-work removal, not a
measured cure for the severe slowdown. Do not replace accepted material packs
or claim the camera is exonerated without configured measurements.

Verification includes failure-before/pass-after dialogue/yaw/seal controls;
shared night lifecycle and production host/queue/combat tests; real-header OoT
night adapter objects in combined/standalone builds; native prepared MM
spawn/cutscene tests, seven generator/factory tests, both native draw fixtures,
frame/fallback and 4,096-inventory stock policy tests; native MM shop/camera and
sword-receipt/bounds fixtures; MM eligibility, collection/lifecycle and real MM
and OoT asynchronous recorder output tests; 25 CI policy tests; clang-format 14
and whitespace/JSON checks. Independent review corrected an unflushed MM
handoff and found no remaining blockers. The five-form spawn loops do not run
form-specific Player_Init.

Build limits: whole native shop/BenPort translation units remain blocked by
missing SDL2 development headers in this environment. Boundary fixtures and
the real recorder compile/run, but full Windows/Linux executable builds,
actual DX11/GPU capture, audible music, visible entry facing and FPS recovery
are unverified. Existing external packs/order and Alt settings still need the
configured game comparison. Rebuild matching modules and application archives
so source preparation includes the corrected MM scene.

Enable the existing console setting `set gDeveloperTools.FrameTimingProbe 1`,
reproduce idle entry, keeper conversation, shelf turn, inspection and return,
then disable with `set gDeveloperTools.FrameTimingProbe 0`. The dedicated
`logs/MM-RenderFlight-*.log` flushes on host handoff and uses its own bounded
capture budget. Compare diagnostics enabled/disabled to account for collection
overhead. Severe slowdown remains open until that evidence exists.

Public GitHub source-tree creation was rejected by automatic approval review
because permission to publish this new candidate could not be verified. No
new remote branch or PR was created. Source, tests, documentation, an
incremental bundle and independently checked restoration instructions are
preserved in the recovery artifact; private saves/gameplay logs and external
asset packs are excluded. Publication approval is distinct from runtime
acceptance or merging.
