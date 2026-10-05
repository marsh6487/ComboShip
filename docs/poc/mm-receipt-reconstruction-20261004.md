# Task 4 receipt reconstruction evidence

Task started: 2026-10-04; report completed and amended after independent review: 2026-10-05 UTC. Worktree: `/workspace/scratch/68d44ffc4c98/takeover-receipts`. Branch: `poc/reconstruct-receipts-20261004`. Final worker HEAD: `a7ee19c58007bcca1a0378261198de7a01414c05`.

This resumes the accepted Task 4 design in `docs/superpowers/plans/2026-10-04-final-polish-reconstruction.md`. The recovered song receipt, song palette and soul-color headers were preserved. Missing production implementations were reconstructed and newly verified; this report does not claim they are byte-for-byte historical recovery. The parent owns integration, GitHub checkpoint publication and draft PR #34 publication. This worker did not push, merge or edit the root gate.

## Coherent checkpoints

| SHA | Change | Red-to-green evidence |
| --- | --- | --- |
| `73d6fa7cba9c9d7f53e6c3561b3b59eec28714d1` | Yellow magic identity and overflow-safe remaining-upgrade count | Production builder failed the yellow name check before the fix. Counts now cover pre-grant levels 0, 2, 7, 8, 254 and 255 without counter mutation. |
| `220a6a7c8d8d5c390593d7a0da83259a8d71d6ea` | Descriptive localized native MM song receipts, including cold donor operation | Production MM builder initially returned no Sonata receipt with an unavailable donor. All 15 MM song identities now use the recovered local bodies; both lullaby tiers and the five shared song variants are included. |
| `8830dbd747e789269892a3c076aa1ba17acd1554` | Traditional OoT tutorial bodies preserved in MM export | Production exporter initially returned no Forest Temple small-key body. Native map/compass bodies now precede appended dungeon information; songs and boss/small keys retain native bodies. |
| `956a62b1da4f21b3625e3d27a0e44395ee563d67` | Exact soul-name colors in the two Latin textboxes | Each engine's actual color-control arm failed exact RGB before the fix. Five bosses in EN/DE/FR, wrapped names, page/control rejection, unknown names and red Goht Remains pass afterward. |
| `9d5b45ca63af089c6e2d41781d1f9d646c9465ac` | Colored MM clef staging, donor tint and MM Ikana ownership | Production fixtures caught the OoT icon selected for Ikana, missing cold alias icons and shared Time RGB for Double Time. Native and donor icon selectors, both clef renderers, white reset and geometry checks pass after the fixes. |
| `f15790182fdb653a7c79e5af58c5d6bea641c680` | Concrete Chest Game key body and all traditional families | A real-catalog fixture reported `Replaced traditional receipt: Chest Game Small Key`. Its native text ID is 0xF3, while randomized dungeon small keys need 0x60. The correction passes all 48 traditional catalog rows. |
| `a7ee19c58007bcca1a0378261198de7a01414c05` | Concrete local MM small-key consumer routing | Independent review found that all ten count-chain keys bypassed the donor. The actual MM builder fixture went RED with `Missing concrete MM key tutorial: Bottom of the Well Small Key`; all ten concrete and direct foreign consumer paths now preserve complete tutorials, including the Chest Game distinction. |

All seven SHAs and section evidence were sent to the parent as soon as the sections were committed.

## Resulting production behavior

### Traditional OoT receipts delivered in MM

`OOT_GetItemReceiptText` selects the original OoT message-table body before fallback/custom receipt handling. Maps use 0x66, compasses 0x67, randomized dungeon and fortress small keys 0x60, concrete Chest Game keys 0xF3 and boss keys 0xC7. Native OoT song receipt IDs come from the item's unresolved GI entry, so receipt composition does not advance the identity after a progressive grant. Enabled map/compass information is appended after the preserved body and a page control. The existing dungeon mode hint remains intact.

The expanded fixture imports real item names and unresolved text IDs from `item_list.cpp` and executes the production exporter for 12 songs, 10 maps, 10 compasses, 10 small keys and 6 boss keys. It checks the complete normalized native body as a prefix, not just a short acquired-name string. Message-table bodies are controlled test seams with distinct IDs; this is a routing/body-preservation proof, not an archive extraction or in-game text screenshot.

Independent final review identified a missing consumer route that the 48-row exporter fixture could not detect. The ten local MM `RI_OOT_SMALL_KEY_*` rows have `ITEM_NONE`/`GI_NONE`; their FC chain lengths (2..9) count identical keys. `ApplyItemReceiptText` allowed the generic direct donor fallback only for `chainLen == 1`, and these keys had no concrete name mapping. The reviewer's real-builder reproduction found `accepted=0`, no donor read and an unchanged generic receipt for all ten keys.

The follow-up maps those ten concrete identities to their canonical donor names in `ConcreteReceiptName`. It does not relax the generic progressive-chain guard or change FC tables, counts, grants or saves. The amended catalog fixture links the actual MM `ItemReceiptText.cpp` and real static/FC catalogs. It exercises every key at count zero and full count, with information disabled and enabled in a valid Rando save, verifies the complete 0x60/0xF3 tutorial supplied by the donor seam, compares `ApplyForeignItemReceiptText` with the local consumer, checks source/page/end appending and asserts that both game and NEI save data stay unchanged. The existing 48-row real exporter fixture separately verifies the donor side of this contract.

Existing production foreign-latch and queued receipt fixtures continue to cover frozen progressive identities, live appearance changes, slot/generation reset, source attribution, ordinary grants and trap suppression. The MM concrete receipt catalog and actual native/FC item catalog checks also remain green. Grant logic and save layout were not changed.

### Native MM songs and clefs

MM's receipt builder now uses `ComboSongReceiptText.h` locally before requesting the OoT donor. Language selection supports the recovered English, German and French bodies, with English fallback for other language values. All 15 MM identities have descriptive receipts with a cold donor; German and French Healing receipts are exercised explicitly alongside the English catalog. The implementation applies the same language choice to every recovered entry.

MM's icon selector stages a native MM 16x24 IA8 clef with the recovered palette. The custom icon loader uses native song metrics; the draw branch keeps 16x24 aspect and applies the staged RGB. OoT's custom clef draw also reads the recovered palette. Ordinary custom icons reset to white, and 24/32 square icon behavior remains covered.

| MM song | Textbox RGB |
| --- | --- |
| Song of Double Time | `80D8F0` |
| Elegy of Emptiness | `FF6200` |
| Epona's Song | `925731` |
| Song of Healing | `FF96E6` |
| Inverted Song of Time | `4A70CA` |
| Goron Lullaby Intro | `FF6464` |
| Goron Lullaby | `FF1414` |
| New Wave Bossa Nova | `1414FF` |
| Oath to Order | `620062` |
| Saria's Song | `6ACB62` |
| Song of Soaring | `C8A0FF` |
| Sonata of Awakening | `62FF62` |
| Song of Storms | `929292` |
| Sun's Song | `EDE73E` |
| Song of Time | `62B1D3` |

The early NEI song dispatcher and model/effect presentation are owned by the GI worker. This worker did not reimplement those dispatchers or alter song models/effects. Parent coordination confirms they are included in the integration work.

### Yellow magic and Skulltula totals

The magic-stat receipt uses a yellow Magic Meter name in all authored languages. The pre-grant count is widened before adding one, so byte values 254/255 cannot wrap. It reports remaining upgrades after this pickup using `StatUpgradeRequired`, including adjustable caps (3 with 0 owned and 100 with 97 owned both report 2 remaining). Descriptions do not change the stat counter. Default-cap, reached-cap and already-max cases pass.

Existing production MM builder tests retain OoT, Swamp and Ocean running totals: 42->43, 7->8 and 19->20 respectively, using item identity rather than the current scene. Exporter tests cover the corresponding dormant counters. Tests confirm that description composition leaves those counters unchanged. RPG cap/formula/migration and reward-delivery regressions also pass.

### Exact boss soul names

Both Latin textbox color arms call the recovered `ComboBossSoulSpanColor` only after a normal green color control. Exact names receive Goht `0A8A2E`, Gyorg `1363A5`, Majora `E88015`, Odolwa `911485` and Twinmold `A8B414`. The helper bounds its scan, recognizes localized names and line wrapping, and rejects pages and other controls. Other names and body spans retain the ordinary engine palette. Goht's Remains remains red.

There is no persistent soul tint latch or new message control. Native boss-soul flame/render code was not edited. Fixtures execute the actual color-control switch arms against actual game `MessageContext`, `SaveContext` and `PlayState` definitions; only the ordinary OoT palette setter is a renderer service seam.

### Ikana resource ownership

The shared icon helper identifies the native MM path `__OTR__icon_item_static_yar/gItemIconMirrorShieldTex`. `OOT_FillItemIconInfo(RG_EXT_SHIELD_OF_IKANA)` selects it explicitly rather than interpreting a colliding native OoT item ID. MM foreign receipt staging uses that raw MM path and preserves MM resource/mod override lookup; ordinary OoT donor icons continue to use `@oot:` routing.

Cold recognition covers the eight declared aliases: Shield of Ikana, Ikana Mirror Shield, Ikana Shield, MM Mirror Shield, Mirror Shield (MM), Mirror Shield (Ikana), Shield of Ikana (MM) and Shield of Ikana (OOT). The foreign icon fixture directly exercises the six unsuffixed/MM mirror aliases with an unavailable donor. It also verifies donor tint metadata, unknown-name native MM ownership, trap suppression and invalid dimension guards.

Pause/editor preservation was inspected at the production callers and existing canonical tables. Both games' `mods/extended_equipment.c::sExtEquipIconPaths` already select the same native MM path for shield row/index 3, and OoT pause/editor call `ExtEquip_GetIcon`. MM's native mirror entry uses the same path in `gItemIcons` via `mm/mods/mm_sources/archives/icon_item_static_yar.h`; the editor uses the native shield entry. These existing routes were preserved. This worker did not run a pause/editor UI session or modify the editor files.

## Fresh verification

Checks source `/workspace/scratch/68d44ffc4c98/test-env.sh`, which supplies actual upstream json/imgui/spdlog/SDL2 and other dependency headers. No fake Cw icon ABI or invented game structs are used for the production syntax checks.

| Command | Result | Evidence |
| --- | --- | --- |
| `python3 -B scripts/diagnostics/run_mm_item_receipt_tests.py` | PASS after concrete MM small-key consumer correction, including all 48 donor rows and ten concrete/direct foreign consumers | `/tmp/receipt-concrete-small-keys-green.log` |
| `ASAN_OPTIONS=detect_leaks=0 python3 -B scripts/diagnostics/run_mm_item_receipt_tests.py --sanitizers` | PASS with AddressSanitizer and UndefinedBehaviorSanitizer after consumer correction | `/tmp/receipt-concrete-small-keys-asan.log` |
| `python3 scripts/diagnostics/run_receipt_soul_color_tests.py` | PASS for MM and OoT behavioral fixtures plus both full textbox C source syntax checks | `/tmp/receipt-formatted-soul.log` |
| `CC='cc -fsanitize=address,undefined -fno-omit-frame-pointer -g' ASAN_OPTIONS=detect_leaks=0 python3 scripts/diagnostics/run_receipt_soul_color_tests.py` | PASS for both engines with AddressSanitizer and UndefinedBehaviorSanitizer | `/tmp/receipt-soul-asan.log` |
| `python3 scripts/diagnostics/run_combo_rpg_tests.py` | PASS caps, all stat formulas, partial sync, idempotency, MM save migration and real OoT RPG pool/export | `/tmp/receipt-rpg-regression.log` |
| `python3 scripts/diagnostics/run_combo_reward_delivery_tests.py` | PASS immediate magic floors, capacity, duplicate/echo suppression, invalid saves, Chateau/native/infinite/ordinary milk behavior and active-only notifications | `/tmp/receipt-reward-regression.log` |
| `python3 -B scripts/diagnostics/run_receipt_syntax_tests.py` | Six full receipt TUs PASS after consumer correction; isolated DrawItem TU has one GI signature dependency | `/tmp/receipt-concrete-small-keys-syntax.log` |
| `git diff --check` / staged whitespace checks | PASS before each section checkpoint | Commit preparation output |

Full production translation units passing actual-header syntax: MM `z_message.c`, MM `z_message_nes.c`, OoT `z_message_PAL.c`, MM `ItemReceiptText.cpp`, MM `StaticData/Items.cpp` and OoT `Messages/ItemMessages.cpp`. Compiler warnings remain (34, 16, 5, 14, 15 and 1 respectively); the harness reports them rather than claiming warning-free compilation. MM C++ checks include the real CMake precompiled-header console-variable, Context and Window prerequisites.

The seventh TU, MM `DrawItem.cpp`, fails in this isolated branch only because recovered `ComboForeignDrawMM.h` calls `MM_DrawNeiGi(recipe, shop)` while the old isolated `NeiGiPresentation.h` declares a single argument. The parent confirms the GI worker corrected this in `cfdbfbfa` and all seven receipt TUs passed in the integrated tree before this consumer follow-up. The parent will independently recheck the follow-up. Per ownership instructions this worker did not copy/cherry-pick or duplicate GI implementation changes to silence that dependency.

Consumer follow-up RED evidence: `/tmp/receipt-concrete-small-keys-red.log`. The independent review's exhaustive ten-key reproduction and log are `/workspace/scratch/68d44ffc4c98/final-review-small-key-repro.cpp`, `/workspace/scratch/68d44ffc4c98/final-review-small-key-repro.py` and `/workspace/scratch/68d44ffc4c98/final-review-small-key-repro.log`. The initial source inspection of the existing generic guard was insufficient; the actual local-consumer fixture now prevents this specific omission from passing an exporter-only test.

The first sanitizer attempt reached LeakSanitizer's sandbox limitation (`Can't open /proc/.../task`, `LeakSanitizer does not work under ptrace`). The successful reruns explicitly disabled leak detection. They prove ASan/UBSan checks, not leak analysis.

## Remaining acceptance limits

- No full platform build, DLL link, Windows build or in-game acceptance was run in this worker. Actual-header syntax and executable production-code fixtures are separate evidence from those checks.
- GI model/effect rendering, authored palette display, early NEI dispatch and integrated bool-shop signature require the parent's GI integration checks. The receipt fixtures prove color/staging and glyph geometry at the renderer service boundaries.
- Pause/editor Ikana routes have source inspection evidence; live native-MM mod override rendering still requires a configured runtime session.
- Traditional message ownership is tested with real catalog data and distinct message-table bodies. Archive extraction and visible native page layout need runtime acceptance.
- Temporary `/tmp` logs are local evidence; all production/test code is committed. The parent owns durable publication of this report and integrated verification artifacts.

No worker implementation concern remains beyond the explicitly coordinated GI integration dependency and the acceptance limits above. Wolf, editor, wallet, root gate, held geometry, save layouts, native flame design and optional asset-pack boundaries were left within their assigned ownership.
