# Unified trade selector regression checks

Run from repository root:

```sh
CC=gcc-14 CXX=g++-14 python3 -B tests/unified_trade_slots/run_tests.py
ASAN_OPTIONS=detect_leaks=0 CC=gcc-14 CXX=g++-14 python3 -B tests/unified_trade_slots/run_tests.py --sanitize
```

The runner compiles source extracted from the production files. It does not
reimplement the trade mapping, ownership helpers, normalization, native age
restore, native hatch, native inventory replacement, registry fold, or MM
page-zero mapping. Tests supply independent expected item arrays and controlled
engine input/audio/icon/service boundaries. Generated sources and executables
live in temporary directories outside the checkout.

Coverage:

- The reported shared Letter bit plus old native child-slot Letter, old saves
  with both selections populated, native child-only state, and flag-only state.
- All 23 OoT and MM unified trade bit indices with independently named expected
  native IDs. OoT table entries cannot become eligible corner masks, even via
  the raw-current shortcut. MM uses its actual placeholder and cursor for the
  fourteen entries without native MM IDs.
- Repeated normalization, an occupied different unified selection, unchanged
  ownership counts, the eight existing masks plus Rito, and refilling the corner
  from an already owned mask. Off-page and solo/nonrandomizer controls remain.
- Eight-button/seven-slot live, child, and adult binding banks; real native OoT
  SaveContext and ItemEquips in the native fixture; both complete native age
  restore branches; native Egg replacement and hatch with another trade selected;
  immediate age roundtrip after hatch preserves all seven migrated Cucco bindings.
- Production item-slot lookup and complete native registry fold, including
  normalized child ownership when another trade remains selected, with solo and
  nonrandomizer negative controls.
- MM's real table, cursor, mask ownership and page-zero visual mapping expose
  exactly one trade cell plus the virtual mask cell. Its hidden key/mama and
  couple native homes stay available to native gameplay.
- The actual native pause input block is compiled as C and linked with the
  production C++ normalizer using matching native ABI flags. Same-frame C equip,
  empty unified-cell materialization, imported-bit-only Letter and return from
  page two normalize before cursor/equip lookup. External rendering, wheel
  animation and audio remain controlled boundaries.

All native objects explicitly share F3DEX_GBI_2, GBI_FLOAT_MTX and GBI_S32_VTX.
Both compiled Combo and solo variants are exercised where applicable. Sanitizer
runs use AddressSanitizer and UndefinedBehaviorSanitizer with non-PIE executables.

These are source-level behavior and native compilation checks. They do not run
the configured game, load an actual save, render pause/HUD graphics, or establish
runtime acceptance of guard/Talon exchanges. Native/editor writes can still land
in the legacy child slot transiently; the shared load and early page-zero pause
boundary normalizes completed writes. Historical trade ownership bits and
unselected bit-only Egg lifecycle semantics remain the existing unified-wheel
policy. The native hatch synchronization is bounded to the real Egg-to-Cucco
hatch event and migrated bindings.
