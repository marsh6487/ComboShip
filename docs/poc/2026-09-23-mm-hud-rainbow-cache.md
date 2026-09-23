# MM rainbow HUD cache and separate upgrade colors

| Field | Record |
| --- | --- |
| Baseline | `f4ce8ba3ca8e7f815e2f055012b7391f2b473bed`, the complete chest/Midna/audio candidate. Its source tree `b3b47aea5b058b7d42643d85825e2be7cce41fed` equals the `fcdf04b` PR-merge build in the latest log session. |
| Candidate | `poc/mm-rainbow-hearts-cache-20260923`, based directly on that baseline. |
| Reported symptom | Enabling rainbow Hearts in the MM cosmetic editor considerably slows gameplay; standalone SoH does not show the same behavior. |
| Source finding | `CosmeticEditorUpdateTick` invokes color callbacks each game update. Hearts/Magic recolor pickup textures, then clear the renderer's entire texture cache and resolved-resource cache, even when custom textures make recoloring return early. The expanded audit found the same per-tick clear in Goron and Zora tunic colors. |
| Change | Invalidate only recolored raw texture addresses and cached textures that depend on modified palettes. Remove blanket cache clears from the cosmetic editor; shared recolor/reset helpers perform selective invalidation. Add a renderer palette-range eviction method for CI4/CI8 source pointers, including the upper palette half. |
| New controls | Cosmetic Editor > HUD: `Double Defense Hearts` (`HUD.DDHearts`) and `Infinite Magic / Chateau Romani` (`HUD.InfiniteMagic`), each with exact hex, rainbow, random, lock and reset via the normal editor row. |
| Compatibility | Before editing the new controls, DD retains the old Hearts-minus-55 color and Chateau retains the old Magic hue rotation. Edited colors apply directly. Reset returns to that fallback. DD white borders and HUD fade alpha stay native. Chateau styling applies only while the drank-Chateau flag is active; this does not grant infinite magic. |
| Preservation | Base Hearts/Magic still drive native/custom 3D ground drops, ChuChu contents and red/green ChuChu colors. New upgrade rows affect the HUD separately. Custom texture pixels remain untouched; draw-color commands continue updating. No archive, geometry, collision, scene, actor, progression, audio, weather, engine-switching, or SoH source changes. Existing renderer invalidation APIs retain their behavior. |
| Status | Implemented; focused regression tests, source compile checks and independent review passed. Game FPS, GPU rendering and vanilla/Alt parity remain untested. Not accepted or promoted. |

## Evidence

Supplied `Fleet of Harkinian.log` SHA-256:
`e82435ee5417087b486d6a52e663fdf28cb72786c8ebff9c26337b9488a564e4`.
The latest MM startup identifies commit `fcdf04b`. At 13:31:43.673 the log
records granting `RI_DOUBLE_DEFENSE` into the MM save. Older texture-null errors
are from an earlier session and are not evidence for this rainbow slowdown.
The log contains no paired FPS measurements or rainbow-toggle timestamp.

`python3 -B scripts/diagnostics/run_mm_hud_cosmetics_tests.py`:

- Failed first against the baseline because an unrelated scene texture was evicted.
- Passed after the selective-invalidation correction, including 120 heart/magic
  rainbow ticks, all cached size variants, original-pixel/alpha restoration,
  and custom/missing-texture controls.
- New-entry tests failed before their registrations were added. Real HUD draw
  command tests failed independently before the DD and Chateau paths were wired.
- Passed full, beating and empty DD hearts, white borders, ordinary hearts,
  Chateau on/off, idle/consumption meter paths, exact hex colors, alpha and reset.
- Expanded audit reproduced unrelated-cache eviction from Goron/Zora rainbow
  before their fix. All 47 built-in rainbow rows now run together for 120 updates
  alongside production custom-material prim/env callbacks without evicting
  unrelated raw/CI/custom textures or resolved resources. Rupee colors stay live;
  custom material alpha and prim parameters are preserved.
- CI8 and upper-half CI4 dependents expire when their Zora palettes change;
  Goron/Zora raw texture updates and reset restore the original pixels. Custom
  palette/texture sources keep both their pixels and cache entries.
- Uses production callback, pixel, color, draw and renderer-cache function bodies;
  resource/window services and GPU work are headless boundaries. It does not
  measure in-game FPS or render a screenshot.
- Compiles both changed C translation units against the real game headers.

The full `CosmeticEditor.cpp` and `interpreter.cpp` translation units also
compiled to objects with real headers, ComboShip defines, the project's
`-fpermissive` setting and the JSON/ImGui headers otherwise supplied through
precompiled headers. Existing controller-macro/UI declaration warnings remain.
This is not a complete game link or Windows build. The expanded cosmetic/cache
fixture also passed AddressSanitizer and UndefinedBehaviorSanitizer; leak
detection was disabled because LeakSanitizer cannot run under this environment's
process tracing.

Expanded custom-item color coverage passed (92 cases, zero failures), including
the actual 3D ground-drop and red/green ChuChu-content entry points, with multiple
hex colors, native/custom models and reset. The native pickup prim/grayscale
commands also retain base Hearts/Magic colors independently of the upgrade rows.
The scene-randomization regression passed. Independent read-only review of both
the original change and expanded audit found no actionable issues. The new
regression runner is included in the CI gate.

## Rainbow path audit

| Entries | Per-update behavior |
| --- | --- |
| Hearts and Magic | Update shared prim/grayscale/env commands for models; selectively expire recolored native pickup textures. Custom pickup texture pixels are skipped. |
| Goron and Zora tunics | Recolor native raw textures/TLUTs; selectively expire their GPU textures and CI palette dependents. Custom resources are skipped. |
| Human/Deku tunics and Human/Deku/Kafei hair | Rainbow updates draw-color commands. Enabling/resetting palette whitening now uses selective invalidation too. |
| Rupee Icon, including Tycoon wallet | Existing live prim-color override; no texture rewrite or cache flush. |
| Remaining HUD, buttons, menus, effects, trails and dungeon items | Existing live cosmetic color lookups; no pixel mutation or cache flush. |
| Custom material entries | Existing prim/env command updates with authored alpha; no texture rewrite or cache flush. |

Archive/Alt-Assets reloads still use their existing cache reset outside the
cosmetic editor. This fix does not change resource replacement behavior.

## Decisive runtime check

Use the same save, scene/camera, texture packs and graphics settings for a short
rainbow-off/on comparison. Check Hearts and Magic, then enable the separate DD
and Chateau rainbow controls. Compare FPS, full/partial hearts, DD white borders,
normal versus Chateau magic, reset and pause/re-entry. Also check Rupee Icon,
Goron/Zora tunics and one custom material row. In Termina Field, verify red/green
ChuChu bodies and their 3D contents, then their dropped hearts/magic jars, follow
the base Hearts/Magic hex/rainbow while DD/Chateau use their independent HUD rows.
Repeat with the currently used Alt Assets setting and its control before
accepting visual parity.

Keep the `fcdf04b` build and existing packs as rollback. This candidate requires
a new executable; it is not an asset-mod archive.
