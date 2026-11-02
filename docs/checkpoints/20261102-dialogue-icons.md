# Dialogue icon checkpoint, November 2

Parent complete upstream: `ac19dda4a7c580988c7641c7b8bf93faaba1790e`.
This bounded milestone precedes the approved Combo Vanilla implementation.

## Source changes

- OoT's fire-capture tutorial embedded icon operand `0xB4`, now Minish Cap;
  `ITEM_LANTERN` is `0xB5`. Use the named item and snapshot the captured fire's
  resource in value-owned receipt metadata. MM's corresponding tutorial now
  receives the same captured-state icon rather than no icon.
- Route regular, blue, Poe and green fire to the existing Fire/Blue/Poe/Green
  lantern icons, with the existing unlit icon for empty/unknown values. The
  four actual authored PNGs were inspected as 32x32 RGBA8 sources serialized as
  RGBA32. Held/GI models, fire effects, ownership and gameplay are unchanged.
- OoT boss-soul messages now retain their custom 32x32 RGBA icon independently
  of the get-item actor. They no longer cast an RG identity through eight bits
  into an inventory icon operand. The current source soul PNG is genuinely
  32x32 RGBA32; MM's prior format-safe soul substitutions remain intact.
- The Market fixture adopts the existing foliage gate's compiled logging
  boundary (`SPDLOG_ACTIVE_LEVEL=6`, `SPDLOG_COMPILED_LIB`). The previous full
  GitHub systems job `125200609977` exposed spdlog/fmt undefined references when
  it linked this fixture. This changes the fixture compiler command, not game
  logging or production gameplay.

## Evidence and limits

The new extracted native builders/soul-branch fixture failed both original
metadata assertions, then passed under ASan/UBSan after the changes. It checks
four fire states, empty/invalid fallback, all three OoT tutorial language
strings, captured-state lifetime and owned soul image metadata. Formatting and
message transport are fixture boundaries; this is not an in-game reproduction.

Fresh existing gates pass: all 14 OoT/native-imported and 14 MM/native-imported
soul icon paths with real asset metadata/native renderer branches; native OoT/MM
receipt wrapping, typography, page/reset and sprite layout; actual message
replacement overloads; Market's 11 eligibility guards, 906 native/Alt binding
cases and 56 actual archive textures. The local Market run uses the existing
optional disabled-spdlog test include; full CI remains the decisive check for
the repaired system-spdlog link configuration. No earlier CI registration was
removed. clang-format 14.0.6 dry-run and whitespace checks pass.

Independent read-only review found no Critical, Important or Minor defect and
verified full MM active-message metadata propagation. Configured runtime,
external mod texture overrides and user visual acceptance remain untested.
No archive was edited. No Combo Vanilla product behavior is claimed yet.

## Recovery

The remote checkpoint overlays only the listed source/test/document changes on
the complete parent tree. Local commits importing unchanged upstream fixtures,
headers or source are recovery-only and are not uploaded as feature changes.
Restore the parent for the pre-icon candidate, or use the checkpoint for these
fixes. Keep accepted assets and the earlier crash/cursor/hint repairs intact.
