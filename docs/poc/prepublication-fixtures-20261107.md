# Prepublication fixture corrections, 2026-11-07

Authorized source parent: `432c3f85d8780783bb7cd22b07ca1e8017aea4c6`.
These changes repair test integration only; gameplay and shared UI sources are
unchanged. The original authorized candidate remains independently recoverable.

- Existing MM native shop adapter and camera fixtures link the complete native
  shop draw code but did not provide the new visibility ABI. Both failed at
  `UiVisibility_IsHidden`. Their shared support now includes the real ABI and
  supplies the default-off result. Dedicated hidden-shop tests retain coverage
  of suppression; every existing stock/receipt/camera assertion remains.
- Formatting the new full MM Interface fixture sorted dependent editor headers
  ahead of `global.h`. A fresh complete UI run caught the missing native types.
  The required global header is now in a separate first include block, which
  remains stable under clang-format 14. The actual Interface fixture passes.

Fresh checks pass: both failed MM shop suites, shared native browsing/camera
suite and complete native MM Interface behavior. Night lifecycle/dialogue/combat,
prepared MM entry yaw and seven scene generator/factory cases, hidden-seal draw
cost, sanitized MM RenderFlight and five analyzer checks also pass. The remaining
scene-readiness check is blocked by the previously recorded missing SDL2 SDK.
No assertion is removed and no production logic is changed by this correction.

The subsequent complete UI rerun reached one more formatting-sensitive fixture:
`mm_buttons_test.py` instrumented an exact one-line helper definition, but
clang-format had expanded that helper. The replacement silently failed and its
test counter was undeclared. It now uses the existing brace-aware named-function
extractor to replace the same helper, preserving every assertion and the actual
production button draw bodies. The targeted sanitized button check passes; the
complete shared/OoT/MM UI suite also passes, including 46 native MM syntax
checks, MM SM64 clocks and both Harpoon vote lifecycle checks.

During publication preparation, cor separately authorized removal of unfinished
randomizer items, or an authored GI with shimmer if Fugue of Home has working
gameplay. That pool/functionality audit is a following bounded change and is not
claimed complete by this test-only checkpoint. Public upload remains pending.
