# GI polish reconstruction, October 4

Candidate: `poc/reconstruct-gi-20261004`, based on recovered header checkpoint
`d1dfbc7b` and published baseline `1c29c83f`. Reconstructed host source is new
implementation, not byte-for-byte recovery. No authored or held asset bytes
change. Native forms, player/mod selection, save layouts and grants are outside
this probe.

## Checkpoint 1: model-independent shimmer identity

The OoT producer now emits `neiShimmer` and live appearance metadata before
archive/model selection. Declining the authored mesh leaves its local energy
unset. Native MM selects the same identity for its fallback overlay, including
rods, spells, Slate powers, Byrna and Four Sword; the fallback restores the
incoming GI matrix before sampling. Mario's legacy drawer still owns its sole
mandatory mask overlay.

Verification:

- `run_nei_identity_tests.py`: production owner descriptor failed on discarded
  identity, then passed 17 themes through authored/mod/selected/missing paths
  with effects off/on. Native MM descriptor failed on discarded identity, then
  passed 19 exact sampler themes through owner/native model selection.
- `run_nei_gi_tests.py`: native production renderer, serialized shelf checks and
  common/shop/overhead C translation units passed using actual engine headers.
- Real `mm/2s2h/Rando/NeiGiPresentation.cpp` C++ syntax check passed. Existing
  engine controller macro redefinition warnings remain.

The recovered foreign recipe consumers still require the missing shop fitting
and themed song implementation. Full candidate build, native/foreign in-game
appearance, Alt parity and buffer headroom remain unproven.
