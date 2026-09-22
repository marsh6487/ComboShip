# ComboShip working integration

Repository: `marsh6487/ComboShip`

Cumulative working ref: `integration/shared-items-soh-20260921`

Integration audit: 2026-09-22

Follow-up candidate `poc/mm-back-equipment-reloaded-sky-20260922` starts at
`639b526150898cee98ffdd09ccfa6ccd6a40f8ba` and adds the native MM hide-equipment
option and opt-in OoT sky texture support. It retains the complete cumulative
tree below. See `docs/stabilization/2026-09-22-mm-equipment-oot-sky.md` for
installation, test evidence, and pending runtime acceptance. This candidate
does not promote or replace the accepted baseline by implication.

The working integration carries compatible features forward together. Start new
integration work from the latest cumulative tree, preserve the accepted source
branches, and record any feature deliberately left out. A branch being pushed or
building successfully does not by itself establish in-game acceptance.

## Sources and retained features

This candidate starts at published weather commit
`51ad3c1f3fe92e76f79ec0a47c68f412380fa319` and adds only GUI-font commit
`075e4e197faa865b5542be5e9d2f6ac96b85e4ae`, plus this record.

| Feature | Retained source |
| --- | --- |
| Shared Items, existing NEI behavior, and the SoH feature stack | `cf1568d4c9a5040c459fb9ed427d27b7216c0ae5`, including prior `cac526a8` stabilization |
| Per-game mod directories | `01e22573` ancestry and subsequent upstream integration |
| Independent opt-in MM scene cosmetic and audio randomization | `93117d11` through `cc1346ce` |
| MM streamed audio IDs beyond 255 | `93117d11`, `9e492676`, `cc1346ce` |
| Ogg/Opus decoding guards and decoder regression linkage | `5887d319`, `10712653` |
| MM cosmetic display-list bounds and resource-reload guards | `626d0e32` |
| Outdoor rain, independent weather audio, pause timing, spin/story overrides, and overcast sky fixes | `835489f0`, `51ad3c1f` |
| Live custom heart and magic-pot colors, including placed/boss hearts and Double Defense | `835489f0`; its seven item source/test blobs match `e64aff98` |
| Optional mod GUI fonts loaded after MM mod archives | `075e4e19` |

The original weather source `4984ef21` was bundled into `835489f0`: weather
implementation and test files match exactly at that bundling boundary. The item
and font source branches are preserved through the second parent of this
integration; they were not merge ancestors of the original weather branch.

The mistaken ComboShip transformation/Torch candidate `9702b20e` is excluded;
that requested work belongs to Shipwright. Djipi font O2R files, sky replacement
packs, and the rain-splash texture fallback remain separate asset inputs. No
archive bytes are added by this integration.

## Verification and remaining coverage

The published `51ad3c1f` baseline passed the gate and Windows/Linux builds in
[Build Artifacts run 35670373874](https://github.com/marsh6487/ComboShip/actions/runs/35670373874).
That run predates the added GUI-font delta.

Local checks on the combined candidate passed:

- GUI-font selection: four configurations, zero failures; archive-before-font
  and font-before-GUI startup ordering. The same test on `51ad3c1f` reproduced
  three missing-font failures before applying the delta.
- MM scene randomization: opt-in defaults, independent toggles, repeated loads,
  and destination audio ordering.
- MM graphics patch bridge: all 13 existing bounds and reload cases.
- clang-format 14.0.6 for the two changed C++ files, and Git whitespace checks.

Fresh full Windows/Linux builds for the combined tree remain pending. In-game
font appearance, pack removal/restart, OoT/MM transitions, and the combined
Alt-assets/weather/audio configuration were not exercised here. Existing
pack-specific runtime limitations remain as recorded in the stabilization notes;
this integration does not claim general pause-crash or archive-mutex resolution.
