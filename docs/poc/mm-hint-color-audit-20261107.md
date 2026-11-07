# MM hint color audit, 2026-11-07

Parent: `e374318dc96024f03a981196fd97df0e0af79707`; candidate branch:
`candidate/hint-colors-mask-menu-20261107`. This inherits the earlier source
candidate; full builds and configured runtime acceptance remain unverified.

## Reproduction and correction

The supplied screenshot reads "They say a Bomb Bag can be found at Gerudo
Fortress (OOT)." The cross-game generator intentionally persists plain item
and area fragments, then the MM foreign gossip callback previously displayed
them directly without adding semantic colors. The native color decoder was
not losing a supplied red/yellow span: this branch supplied no spans.

`ComboHintPresentation::MmForeignGossip` now decorates only the known persisted
foreign placement sentence at the display boundary. It supports overworld
`can be found at` and dungeon `can be found hoarded in`, requires the existing
OOT suffix and nonempty item/location, and leaves unrelated rumors unchanged.
For that known sentence it normalizes recognized OoT percent-color escapes,
including MM-unsupported cyan/black, before applying red item, yellow location
and white prose resets. UTF-8 words, trap disguise names and unknown percent
escapes are preserved. Reapplying presentation is idempotent.

No generator, loader, saved hint text, order, weighting, RNG or tracker payload
is changed. Previously generated seeds receive presentation when displayed;
seed regeneration/migration is unnecessary. This avoids leaking color escape
strings into the combo tracker, which still receives its canonical plain pool
text and uses the same reveal index.

## Broad source audit

| Surface | Finding and outcome |
|---|---|
| MM foreign placement gossip | Missing spans repaired: red item/yellow location/white prose. |
| MM native free/paid/Moon gossip | Existing green item/yellow location retained. |
| MM Sensor Rune | Missing green item/yellow location spans repaired; separate enum-sentinel selector/picker bug corrected. |
| MM Moon-child trial requirements | Four remains and three existing transformation-mask item spans repaired; gates, counts and continuation IDs retained. |
| MM bank/spider-house/fairy and other NPC hints | Existing item/location emphasis retained; explicitly authored special NPC palettes preserved. |
| MM Saria, Hookshot, Soaring, Oath and poster location hints | Existing location colors retained. |
| OoT gossip/static/altar/entrance dialogue | Native authored palettes retained, including green-item/red-location gossip and WotH/Foolish category accents. |
| OoT entrance signs | Bare destination labels retained; these are not missing dynamic placement sentence spans. |
| Native/combo hint trackers | Plain prose with tracker-owned type/read colors retained. |
| Shared item/location getters | Remain bare fragments so shops, receipts and authored templates keep their own palettes. |

See `mm-sensor-moon-hint-colors-20261107.md` for the independent Sensor/menu
predicate correction and native callback tests.

## Evidence and limitations

The full native gossip-routing fixture first failed the literal screenshot
assertion on the parent. It now executes the actual MM `ReplaceColorChars`
body and validates literal red `0x01`, yellow `0x04`, native green `0x02` and
embedded-zero white resets. Fixed screenshot/dungeon/name cases coexist with
an arbitrary saved-pool audit: saved words, payload and reveal identities must
survive even when names differ from the synthetic fixture. Review caught and
removed a temporary fixture-only assumption from that audit.

Named checks pass normally or with ASan/UBSan as appropriate:

- `tests/cross_hints/run_gossip_routing_tests.py`: full generation, all four item/check-game combinations, all clarity tiers, saved old/new pool consumption, arbitrary legacy rumors, physical/grotto routes, gates, paid hints, Moon priority, trap disguises, generation-cache reload and tracker identity.
- `tests/cross_hints/run_item_hint_export_tests.py --sanitize`: real authored color encoding, translations, empty fallback variants, 64-seed downstream RNG and trap continuity.
- `tests/hint_dynamic_colors/run_tests.py --sanitize`, combined/standalone: actual Sensor/Moon callbacks and picker, native save/quest types and color bytes, complete enum-range controls, wish order and refusal/charge boundary.
- `scripts/diagnostics/run_hint_item_area_tests.py --sanitize`: all 13 area payload/lifetime controls.
- `scripts/diagnostics/run_hint_item_name_tests.py --sanitize`: all 756 item/clarity/language controls after supplying available spdlog headers.

LeakSanitizer is disabled under ptrace; address/undefined checks remain active.
The first standalone exporter attempt encountered the environment's ptrace
LeakSanitizer restriction and passed after the standard `detect_leaks=0`
setting. The name fixture initially lacked the available spdlog include path;
its corrected environment run passes. No production workaround was added.

Full EnGs/EnJs translation-unit checks were attempted with real game headers
and stopped at missing SDL2 development headers. Font upload/wrapping and
configured in-game pixels remain separate from these native byte/callback
checks. Next runtime check: reload the same seed and stone, verify the requested
red Bomb Bag/yellow Gerudo Fortress, then ordinary/paid/Moon/NPC hints, Sensor
wishes and tracker readability. No public upload, merge or master acceptance
is implied.

The separate native-MM worn-mask setting question was audited read-only:
`Invisible Non-Transformation Masks` currently exists only on the OoT host and
applies to imported MM masks worn there. No native-MM visibility feature was
added. The later reported Saria absence is a separate spawn/seed-state task;
this presentation milestone does not claim to fix it.
