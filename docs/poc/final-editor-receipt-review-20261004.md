# Independent final editor and receipt review

Reviewer scope: read-only review of MM editor/wallet, receipt/export, textbox icon and soul-name color changes from published baseline `1c29c83f5d676d02ececa7302c138f7aa46b952a`, against `AGENTS.md` and `docs/superpowers/plans/2026-10-04-final-polish-reconstruction.md`. GI rendering and Wolf runtime have separate reviewers. Initial reviewed production head was `704fcb9b`; subsequent recovery-report/checkpoint commits through `b4815382847dc83c68f74a662872a1c5eb03c372` did not change the identified paths. Final reviewed scoped head: `e868618b21f45fc5f1357da5c2a6d663d3c0b48f`, including both verified corrections. The committed editor helper, fixture and runner exactly match the independently tested blobs listed below. No repository files, commits or remote refs were changed by this reviewer.

## Findings

### Critical

None identified.

### Important 1 — Local concrete OoT small keys in MM bypass the full receipt exporter

Status: CLOSED after independent review and verification of integrated correction `b43b766fed433d1355d8ff193e3bc5565baaad26` (worker `a7ee19c58007bcca1a0378261198de7a01414c05`). Ten explicit canonical name mappings address the local count-chain route without changing grants or FC counters. The independently rerun complete receipt suite passed, including all ten concrete keys, count0/full, information setting off/on, complete traditional bodies, Chest Game distinction, foreign parity, source append and unchanged save/NEI state. Evidence: `/workspace/scratch/68d44ffc4c98/final-review-receipts-corrected.log`.

Locations: `mm/2s2h/Rando/ItemReceiptText.cpp:ConcreteReceiptName` and lines 405–428, especially the `chainLen == 1` donor filter at lines 410–411. Actual count-chain rows are in `mm/2s2h/FleetShipCombo/FleetComboItems.h:580–598,774`. The existing catalog consumer fixture's concrete identity cases in `tests/item_receipts/catalog_test.cpp:364–405` omitted all ten small keys.

The final polish contract requires traditional OoT small-key receipt bodies when delivered in MM, including concrete routes. `OOT_GetItemReceiptText` now selects the correct traditional bodies, including Chest Game text `0xF3`, but a local concrete `RI_OOT_SMALL_KEY_*` never reaches that exporter. These identities have no `ConcreteReceiptName`, their FC rows have count-chain lengths 2–9, their MM metadata has `ITEM_NONE/GI_NONE`, and the NEI registry has no matching fallback. `ApplyItemReceiptText` returns false, leaving CheckQueue's brief generic receipt intact.

Reproduction uses the actual MM `ItemReceiptText.cpp`, actual MM/FC item metadata, and the existing linked catalog fixture's donor seam. On a ready donor, all ten calls return `accepted=0`, `donorReads=0`, and preserve `GENERIC RECEIPT UNCHANGED`. Forest/Fire/Water/Spirit/Shadow/Well/Training Ground/Fortress/Ganon/Chest Game chain lengths are respectively 5/8/6/5/5/3/9/4/2/6. The direct foreign sentinel exporter still passes its independent traditional-family tests, which is why the existing aggregate suite did not catch this consumer gap.

Evidence: `/workspace/scratch/68d44ffc4c98/final-review-small-key-repro.log`; reproducible scratch source and wrapper are `final-review-small-key-repro.cpp` and `final-review-small-key-repro.py` beside that log. Command after sourcing `test-env.sh`: `python3 -B /workspace/scratch/68d44ffc4c98/final-review-small-key-repro.py`.

Recommended correction: explicitly identify these count-chain items as concrete receipt identities, without relaxing the progressive filter globally or changing their grant/counter semantics. Cover every local concrete key and preserve Chest Game `0xF3` separately from dungeon `0x60`.

### Important 2 — A medallion can fabricate prior wand ownership and bypass canonical grant recording

Status: CLOSED in root commit `e868618b21f45fc5f1357da5c2a6d663d3c0b48f`, after independent source review and the expanded actual production-grant editor suite passed, exit0. The tested helper blob is `c76826efd229b2befd9657401c7e92cda974202d`, fixture blob `1406c8621846196e762dd392f59a524e6475bf0c`, and runner blob `816a74648b6f180be850d91683509759a4377de3`. The correction uses retained valid rod bits for host restoration and separates earned ownership from active medallion usability. Canonically earned bare rods now receive only their missing medallion prerequisite; independently earned medallions no longer bypass the real wand grant. All9 pass groups, including clear/regrant, existing full-rule repeats and actual TU syntax, are recorded in `/workspace/scratch/68d44ffc4c98/final-review-editor-corrected.log`. The exact tested blobs were checked against that committed head; no production implementation remains unresolved.

Locations: `mm/2s2h/DeveloperTools/NeiEditorItems.h:203–225`, in particular `RestoreOwnedHost`'s use of `Wand_ModeOwned` for earned-host evidence. Under `WAND_RANDO_MEDALLIONS`, production `Wand_ModeOwned` at `mm/mods/extended_inventory.c:1565–1577` reads the independent `ootQuestItems` medallion bits.

The clear/regrant exception is intended to restore a previously earned host cell without replaying its grant. Instead, it treats an independently obtained medallion as evidence that a wand was previously obtained. On a fresh save, canonically obtain Spirit Medallion, leave the wand slot empty, then click the editor's Elemental Wand or Sand Rod grant. `RestoreOwnedHost` writes the host, `IsOwned` becomes true, and `Grant` returns before `Rando::GiveItem`. The result is a usable wand with no earned rod bit, no wand FC obtained/applied entry, and no canonical acquisition notification or shared recording.

Actual production-grant reproduction:

```
Elemental Wand: granted=1 host=208 wandRodsOwned=0 FCobtained=0 FCapplied=0 serialDelta=0 sharedDelta=0
Sand Rod: granted=1 host=208 wandRodsOwned=0 FCobtained=0 FCapplied=0 serialDelta=0 sharedDelta=0
```

Evidence: `/workspace/scratch/68d44ffc4c98/final-review-wand-medallion-repro.log:126–127`; scratch source/wrapper `final-review-wand-medallion-repro.cpp` and `.py`. Command after sourcing `test-env.sh`: `python3 -B /workspace/scratch/68d44ffc4c98/final-review-wand-medallion-repro.py`. The standard editor fixture started from empty medallion ownership and therefore passed without observing this case.

Recommended correction: use retained earned wand/rod evidence for host restoration, and verify Grant All completes variant ownership when independent medallions already exist. Also cover the inverse partial save: an already-earned bare wand with its medallion still missing must receive the missing prerequisite without recounting its wand FC acquisition; clearing and restoring that host must preserve its earned counts.

### Minor

None identified in the reviewed scope.

## Verification and positive evidence

All commands sourced `/workspace/scratch/68d44ffc4c98/test-env.sh` for the actual pinned JSON/imgui/spdlog/thread-pool/SDL dependencies. These were independently rerun, with exit status zero:

| Command | Evidence and review result |
| --- | --- |
| `python3 -B tests/mm_editor/run_tests.py` | `final-review-editor.log`: production grant handlers, four canonical full-width slots, all five runes/four seasons/cane variants, all three wand rules from empty starts, repeat and clear/regrant fixtures, actual pool generation and Grace/Fire Rod binding characterization; real SaveEditor/GiveItem TU syntax. Initial partial independent-medallion coverage was missing as noted above; the corrected expanded suite also passed independently as `final-review-editor-corrected.log`. |
| `python3 -B tests/mm_wallet/run_tests.py` | `final-review-wallet.log`: all 65,536 signed16 balances across native wallet levels 0–3 and Tycoon, full decimal texture selection, unchanged rupees/upgrades/accumulator, and actual TU syntax. |
| `python3 -B scripts/diagnostics/run_mm_item_receipt_tests.py` | `final-review-receipts.log`: full exporter families, information setting behavior, running token totals, magic counts including saturated stored values, localized MM songs, native IA8 16×24 clefs, tint reset for square icons, cold Ikana aliases and owner routing, dimension/trap guards, binary-length codecs, foreign latch slot/generation reset and source attribution. Initial missing local small-key consumers are noted above; the corrected complete suite also passed independently as `final-review-receipts-corrected.log`. |
| `python3 -B scripts/diagnostics/run_receipt_soul_color_tests.py` | `final-review-soul-colors.log`: actual Latin color-control arms and real headers in both engines; exact Goht `#0A8A2E`, Gyorg `#1363A5`, Majora `#E88015`, Odolwa `#911485`, Twinmold `#A8B414` for EN/DE/FR names, wrapping newline handling, page/ordinary name preservation, red Goht's Remains. |
| `git diff --check 1c29c83f5d676d02ececa7302c138f7aa46b952a..HEAD` | No whitespace errors in the reviewed committed range. |

The current live catalog has 61 active NEI/EXT identities excluding retired Grace. Hourglass is included exactly once iff NEI is enabled in the exercised complete production pool generator with empty placement/region input, across solo/combo and wand settings. This characterizes new pool construction and does not rewrite existing seeds. Randomizer item 194 requests `hylia_grace`, while 192 requests `fire_rod`, including isolated legacy override behavior.

Both wallet paths now format a maximum of five decimal digits, with each texture index bounded 0–9. The helper never changes the balance or wallet capacity. Native and Tycoon origins/spacing/override routing are retained; the new necessary digits extend rightward.

Icon review found the shared `CwItemIconInfo` extension append-only and zero-initialized by the exporter/consumer, with new format/dimension/color flags guarded at the foreign boundary. Ikana's cold aliases stage the MM path before requiring the OoT donor; donor-reported MM Ikana ownership avoids the `@oot:` rewrite. Other foreign textures retain that explicit OoT ownership. The staged square icon setter resets its tint to white. Existing pause/editor extended-equipment tables in both games already select MM's `icon_item_static_yar/gItemIconMirrorShieldTex`; native MM shield receipts retain their native MM GI/icon route.

The exact soul-name tint has no persistent state and requires the complete authored localized name followed by the native white reset. Ordinary palette-setting behavior remains in each engine's existing color arm; no native soul flame geometry was altered by the reviewed text implementation.

## Acceptance limits

No game was booted by this reviewer. This review and its behavioral/sanitized fixtures establish source-level behavior at production boundaries; they do not establish actual HUD fit, pickup presentation, item use, mod-pack visual selection, a full placement-solver result, platform package acceptance, or in-game acceptance. Root reports all seven receipt production TUs passed real-header syntax separately. Public PR push/CI gates and their exact uploaded tree remain the root's responsibility.

Both Important findings are independently CLOSED in the tested source. No remaining Critical, Important or Minor findings were identified in this scope. Source/compiler/fixture acceptance is supported; in-game and platform acceptance remain separate.
