# River frogs and ghost Dampé catalogue additions

Baseline: `51ce4e17cb4d397a8a4efae4c4fd17407406e066`, retaining the completed
night dialogue, Black Market, MM hint-color and legacy Saria source corrections.
Candidate branch: `candidate/frogs-dampe-catalogue-20261107`.

The requested result is an independent En_Viewer presentation for each of the
five Zora's River frog colors and an idle ghost Dampé. Use the established
catalogue, native skeleton/animation resources and replacement-model loading
path. Append identities without changing existing actor/pose parameters.
Do not run the original minigame, race, story, reward or camera actions, and do
not place actors into any scene automatically. Retain native blinking, frog
colors and the ghost's translucent body/lantern presentation where applicable.

- [x] Verify native resource/draw requirements and reserve stable new params.
- [x] Extend the registry and independent presenters; exercise placement,
  resource lifetime, animation, color/eye and ghost draw boundaries.
- [x] Add machine-readable placement presets and the parameter reference.
- [x] Run focused native/sanitized tests and relevant existing catalogue checks;
  independently review the bounded diff.
- [x] Commit and independently restore a cumulative source checkpoint, then
  provide the exact combined publication authorization requested by cor.

Ownership: implementation agent owns En_Viewer sources and focused fixtures;
root owns presets, parameter documentation, CI registration and source recovery.
An independent reader audits integration surfaces and draw requirements.

Full executable builds, appearance with the user's model packs, Alt toggles,
placement scale/pose in a running scene, and gameplay acceptance remain separate
from source and CPU fixture verification. No accepted master promotion.
