# Final polish recovery checkpoints

Published PR #34 baseline: `1c29c83f5d676d02ececa7302c138f7aa46b952a`.

The historical final candidate disappeared from the transient workspace. The supplied partial recovery ZIP contained 14 exact blobs from GitHub staging tree `dc4171c3587e52e62359ab6ee1f06ecaf03a870a`; their Git hashes and archive SHA-256 checks passed. It did not contain the complete gameplay source or tests. The reconstruction plan names the accepted requirements and file ownership.

| Checkpoint | Content | Verification | Remaining work |
|---|---|---|---|
| `c54ca14d3555324731412c35a0242e5dd334afcc` | Exact recovered headers and accepted reconstruction plan | Remote tree `34ea8dc7c7a5c4ccd49ce13c42fc4622eb58004d` equals the local tree; fetched branch verified | Reconstruct gameplay implementations and fresh tests |
| `3f6ed4d42d7cb759ee1bb5f06cd97d3af4388fdf` | Tested skin, yellow magic and independent shimmer sections; checkpoint instructions | Remote tree `7068c516dd6b5488407ce7b91f25d86cc97cf1ab` equals the local tested tree; fetched branch verified | Wallet/editor, Wolf core/host, GI fitting/song effects, remaining receipts/icons and combined review/builds |
| `6d5902fb8ada8dd8b5ce9fedec4a1fa6e93a07c9` | Safe wallet digits and descriptive localized MM songs | Remote tree `e4f335a0d6e1b3288b904ec6f058e621274083d3` equals the local tested tree; fetched branch verified | Editor, Wolf core/host, remaining GI/text presentation and combined review/builds |
| `9d4886c54847b0eb267a8985d99bd55140011170` | Wolf loader, GI fit, traditional/exact-color receipts, Pikachu compatibility and WIP editor fixture | Remote tree `9e03696c4aa44929e0516d04360a4fa4f72a2b8c` equals local tree; fetched branch verified | Host/editor implementation, final presentation, review and builds |
| `a79093a7c4c6a601c334c44e7578cb6ca945f83b` | Live editor grants, Wolf combat, themed songs and sanitized skin fixtures | Remote tree `4832641839853c68f577c2108f48312c174db1c7` equals local tree; fetched branch verified | Host, sword/receipt integration, preservation review and builds |
| `167aa37c33c3f5be46de342287772100de5633fa` | Integrated Wolf host, upright swords, colored clefs and MM-owned Ikana icons | Remote tree `6942eb50ff1c26561b7624863a8d34abd1f16b62` equals local tree; fetched branch verified | Final edge cases, independent review and builds |
| `d5b9a68d3065f40bd02b468dd5dd468220c781d2` | Final editor/catalog/mod integration and explicit open review findings | Remote tree `9069ac12a0bd2020815aa959195619c7669b1851` equals local tree; fetched branch verified | Independent corrections, real arena bounds, whole gate and builds |
| `3ec2c2181378772909f9740342d2d66b2b923360` | Independently verified Wolf input, selected sword identity, concrete keys and earned-wand corrections | Remote tree `dc8d93dca68147f64b07a5268b1a47503f89795e` equals local tree; fetched branch verified | MM shop integration, real arena preflights, whole gate and builds |

## MM skin section

Local worker commit `12d055506bbdb49280cd30e8502672b9a10e91e7` adds opt-in root motion, fractional animation interpolation and definition-only scale while retaining zero-default Pikachu behavior. Production pose computation and native MM matrix code are exercised by `python3 -B tests/mm_wolf/run_skin_tests.py`. The fixture failed on the previous renderer's packed vertex position and passed with the change. Queries reject another character's pose and destroyed skins.

## Other completed sections

- Yellow magic receipts retain localized yellow identity and RPG remaining-upgrade counts; saturated tiers no longer wrap. `python3 -B scripts/diagnostics/run_mm_item_receipt_tests.py` passed on the combined checkpoint, including native/foreign receipt catalogs, grants, latch state, Skulltula totals and dungeon-information guards.
- Custom/legacy GI selection retains independent shimmer identity without attaching authored mesh-local energy. `python3 -B scripts/diagnostics/run_nei_identity_tests.py` passed all 17 owner themes and 19 native MM themes through selection and toggle cases. `python3 -B scripts/diagnostics/run_nei_gi_tests.py --held --combo` passed the combined renderer and engine-header checks, including foreign-host shop fitting.
- Native MM and Tycoon rupee digits safely display every nonnegative signed16 balance without changing saved currency, wallet capacity or accumulator state. `python3 -B tests/mm_wallet/run_tests.py` passed all 65,536 balances across five paths under ASan/UBSan/bounds and the real native HUD translation-unit syntax check. The regression reproduced the native index16 crash at 1667/wallet2 and Tycoon10000 masking to1296. Only leak detection is disabled because the fixture allocates no heap and this runner cannot perform LeakSanitizer's process inspection.
- All 15 MM song variants retain descriptive localized receipt bodies with a cold donor. The combined `run_mm_item_receipt_tests.py` runner passed again after this section, including EN/DE/FR song cases and existing grant/dungeon-information guards.
- Wolf's binary loader rejects malformed bounds, counts, trees, names and NaN/Inf using serialized float bits. `python3 -B tests/mm_wolf/run_core_tests.py` passed UBSan and optimized fast-math fixtures. Host activation was integrated after the scaffold checkpoint; see the next section.
- Traditional OoT dungeon/song receipts survive the donor export; enabled map/compass and MQ information appends as a separate page. The combined receipt runner passed again with these changes.
- Generated catalog bounds now fit all 61 authored GI models under native/OoT/MM routes and pickup/shop/freestanding matrices. The production GI runner passed; Four Sword's actual top edge now fits while retaining its shelf scale. Source/held asset bytes are unchanged.
- Exact boss-soul name RGB is applied by both Latin text renderers only in recognized localized Soul-name spans. `python3 -B scripts/diagnostics/run_receipt_soul_color_tests.py` passed five souls across EN/DE/FR, wrapped/malformed/page/unknown cases, ordinary palette and red Goht Remains, plus both real textbox translation-unit syntax checks.
- Independent review found an existing Pikachu metadata layout with 48 skeleton nodes and 47 weighted bones. The strict equality guard was corrected. A fixture now links the actual generated Pikachu mesh/skeleton/registration and passes production draw submission, legacy shared scale and bounded pose queries. `run_skin_tests.py` passed again after the fix; review follow-up is in progress.

## Newly verified sections

- MM editor grants now enumerate the live NEI/EXT item table and use production grants for all four canonical slots, powers, seasons and cane/wand variants. `python3 -B tests/mm_editor/run_tests.py` passed actual catalog/FC/grant/flag behavior, ordinary inventory byte preservation, three wand rules, repeat safety, partial cane and Roc Cape cases, plus real SaveEditor/GiveItem translation-unit syntax. The previously WIP fixture is now green.
- Wolf combat keeps native MM damage and freeze actions authoritative and releases attack ownership. `run_core_tests.py` passed native damage flags, freeze with zero invincibility, bounce, dash/wall rebound and cleanup in UBSan and fast-math modes.
- Themed song geometry includes actual Soaring feather vanes/rachises. `run_song_gi_tests.py` passed all 24 profiles, native/imported mappings, early MM dispatcher and wraparound frame cases.
- The skin fixture's actual graphics arena now initializes both head and tail pointers. The null-tail regression reproduced under UBSan and was corrected. `run_skin_tests.py` now passes ASan/UBSan/bounds for synthetic and actual Pikachu data; leak detection alone is disabled for the runner's process-inspection restriction.

## Integrated host and presentation checks

- `python3 -B tests/mm_wolf/run_host_tests.py` passed the production Wolf host/core and real full-width extended-button accessors. It links the unchanged native MM freeze and thaw bodies, exercises periodic damage once, A/B thaw input, native completion without reacquiring the Wolf action, C/D-pad toggles, tool/custom-form/PAK/O2R priority, masks, death/scene/destroy cleanup and asset fallback.
- `python3 -B tests/mm_wolf/run_skin_tests.py` and `run_syntax_tests.py` passed again with the host integration; all four changed production translation units use real engine headers.
- `python3 -B scripts/diagnostics/run_sword_pose_tests.py` passed actual standalone/native and producer sword bodies: +X blades become upright +Y through all spins. `run_nei_gi_tests.py --combo` passed again, including native selection priority, foreign shelves, custom models, all 61 serialized GI bounds and authored effects.
- `python3 -B scripts/diagnostics/run_mm_item_receipt_tests.py` passed on the integrated tree with colored native IA8 song clefs and MM-owned Ikana shield icons, including cold aliases, foreign ownership and aspect/reset behavior. `run_receipt_syntax_tests.py` passed all seven full production translation units, including the jointly edited MM DrawItem dispatcher.
- The combined regression gate now includes these host, syntax and sword tests.

## Final catalog and mod checks

- After integrating the final editor section, `tests/mm_editor/run_tests.py` passed all eight groups: actual current grants, clear/regrant host recovery without recounting earned powers, repeat/native inventory preservation, complete production Hourglass pools and Grace194/FireRod192 bindings, plus both real translation units. The extracted binding fixture now uses the production `Kind` alias and a declared effects-off CVar boundary. Its historical RED control reads published baseline `1c29c83f`, whose editor/registry blobs equal the original local control, so it survives a fresh checkout.
- The completed direct OoT exporter fixture passes all 48 traditional catalog rows, including the Chest Game key's native text ID `0xF3`. `run_mm_item_receipt_tests.py` passed again on the integrated root. The tracked receipt report preserves the worker's sanitizer and source evidence.
- `run_nei_gi_tests.py --combo` and `run_nei_identity_tests.py` passed again with arbitrary external OPA/XLU mod replacements. Those selected meshes retain independent shimmer while excluding authored fit and model-local energy.
- CI now provides BS thread-pool v4.1.0 and stb headers at the project's exact pins. Actual SaveEditor compiler dependency expansion proved they are required alongside the already-provided ImGui/JSON/spdlog/SDL headers. The accepted fixed historical source commits used by preservation tests have been fetched unchanged.

## Independent review corrections

- Wolf now captures effective input after MM's unchanged override/disabled/textbox selection and pass-input hook, restores only its reservation before that selection, and yields for remote tools. The native input and freeze fixture, sanitized core/skin and all four real translation units passed. Independent safety review verified the reported raw-input reproductions closed; its earlier Pikachu metadata and null-tail fixture findings are also closed. Source report: `docs/testing/MM_NEI_WOLF_RECONSTRUCTION.md`.
- Selected standalone/Din sword recipes now assign the concrete sword's identity before returning. Independent real full-producer/resolver and combined renderer sanitizer/fast-math tests passed all seven themes, with mandatory exact identity and no generic overlay.
- Concrete MM small-key names now route all ten count-chain identities to the OoT exporter. Actual `ApplyItemReceiptText` fixtures passed all keys at zero/full counts, complete tutorial bodies, Chest Game text distinction, source append and information toggles without save writes. Independent rerun passed after integration.
- Review also reproduced a medallion-only editor save silently gaining a wand host without earning or recording the wand. Host restoration now requires actual retained rod bits; new rods and missing medallion prerequisites are granted separately, avoiding a second award of a bare earned wand. The expanded production fixture first failed the new earned-bit assertion, then passed all nine groups including medallion-only individual/all grants, retained/cleared bare rods, FC recording, repeat safety, all wand rules and real translation units. Independent review confirmed the exact committed header/test/runner blobs. The final editor/receipt review report is tracked alongside the section reports.

## Remaining integration

The actual MM shop callback now forwards `ACTOR_EN_GIRLA` context through native and foreign dispatch. Independent sanitized/fast-math production callback fixtures passed every authored binding's shelf route and unchanged null-actor world route. The stale extracted key-cache fixture now includes the real identity policy header and passes sanitizers.

The MM shared skin's real arena preflight is integrated and independently verified: material/no-material draws reserve 10/9 opaque commands, aligned matrix space and two temporary entries in each side arena. Required-minus-one capacity previously crossed head/tail; genuine native helper/canary fixtures now reject before mutations and pass normal/fast-math sanitizers with actual Pikachu metadata. The takeover correction below propagates a capacity-skipped draw and reserves its new outer marker/freeze continuation. Existing arbitrary native get-item/equipment/mod submissions keep their inherited allocator contract; whole-frame/game headroom is not claimed.

GI arena completion from worker `282b0090` is integrated as `7df56cb8`. Actual OoT allocator/Open/Close/setup seams, overlay and exact-fit boundaries, native shell caller reservations, MM helper seams and real-header syntax checks pass the sanitized/fast-math production GI runner. The final review found intrinsic flame allocations outside the original model reservation; the takeover correction below covers them.

Complete combined regressions and exact-head Windows/Linux builds follow the independently reviewed corrections. This is still a partial checkpoint; in-game acceptance is not claimed.

## October 4 19:50 Chicago takeover

The user requested takeover after the original root remained at local commit
`9994843fec7798f5cb25e2ea0a95707f0f2f3292` while its final GI worker completed
`282b00900d73daa5b1b9e6ea4bcdeeae2e1b104c`. The original worktrees were found
and copied into a separate checkout; their source was preserved. Root source
includes actual MM shelf forwarding, shared MM skin arena capacity checks and
the earlier shared GI arena WIP. Worker completion was integrated as `7df56cb8`.
This recovers existing local source; the underlying implementations remain the
reconstruction already described above.

Fresh `python3 -B scripts/diagnostics/run_nei_gi_tests.py --combo --sanitize
--fast-math` and `python3 -B tests/mm_wolf/run_skin_tests.py --fast-math` passed.
The complete NEI gate initially stopped at an older presentation fixture that
omitted the restored song and colored-icon helper dependencies. Its production
boundary coverage is being updated, not bypassed. A new independent final review
also reproduced two arena integration gaps: intrinsic weapon flames and Wolf
draw rejection reaching the subsequent native player effects. Their corrections,
the whole regression gate and exact-head platform builds remain pending. This
is an explicitly incomplete, recoverable source checkpoint.

Fetch `recovery/final-polish-20261004`, inspect this note and the reconstruction plan, and continue the uncompleted sections. The checkpoint commit's message records its exact tree and tests. Do not treat the 43-command gate reported by the lost session as fresh proof. Keep the final PR draft and preserve its published ancestry. Existing authorization permits the final push to PR #34; no merge or master promotion is authorized.

## Takeover correction checkpoint

Recovered source and worker integration were published as `ee6e5d6fbced1327612012bc6ec87d9bdc927229` on `recovery/final-polish-resume-20261004-1950`. The fetched remote commit/ref and tree `04d65c9d69aec99ccecef93dc65236a33f8e0f01` match the local snapshot `6957b3bf`. The original recovery branch/worktrees were preserved.

The independent final reviewer inspected `7df56cb8` against published baseline `1c29c83f` and returned two Important findings, with no Critical or Minor findings. Both were reproduced before correction. Wolf draw rejection now returns a distinct selected/skipped status and returns from native Player_Draw before any continuation. The opt-in skin reserve includes nested native markers and the frozen shell's twelve-Gfx scroll, matrix and four translucent commands. Existing Pikachu callers retain the original zero-reserve API. Intrinsic OoT Somaria/selected True Master Sword flames and the MM Somaria bridge now preflight their real scroll, matrix, setup and debug costs with the subsequent model.

Fresh verification after these corrections:

- `python3 -B tests/mm_wolf/run_host_tests.py`: passes production host/core/skin, exact native Player_Draw Wolf branch, native Open/Close and frozen scroll. Exhausted arenas remain untouched, the selection remains active and sufficient capacity submits the skin and native shell. Other equipment/get-item/effect callbacks remain boundaries, as documented.
- `python3 -B tests/mm_wolf/run_skin_tests.py --fast-math`: passes native marker/setup/allocator bounds, caller reserves and actual Pikachu metadata under sanitizers.
- `python3 -B scripts/diagnostics/run_nei_gi_tests.py --combo --sanitize --fast-math`: passes native/foreign shelves, all 61 frames, exact capacities and shared shell reservations.
- `python3 -B tests/nei_gi/run_flame_arena_tests.py`: passes actual OoT/MM flame, twelve-Gfx scroll and debug bodies under ASan/UBSan; short OPA/XLU arenas skip without allocations, full arenas draw the flame and model. LeakSanitizer alone is disabled because this sandbox cannot inspect processes.
- The old presentation fixture now includes the production song draw policy, actual RandomizerGet enum, icon ownership and icon tint helper. Native/foreign/Ikana and colored song icon boundaries pass.
- Pinned clang-format 14 checks pass for all 4,524 workflow-scoped source files; the collision gate reports no new paths; conflict-marker and whitespace checks pass.

The expanded complete 43-command NEI gate is in progress. Windows/Linux builds and packages have not yet been started for this correction. This checkpoint is source verification, not game acceptance or promotion. See `docs/testing/FINAL_POLISH_INTEGRATION_2026-10-04.md` for the final review scope and pending acceptance.

## Harness finalization resumed October 4 at 21:02 Chicago

Resumed from local correction `de8d87f9896bfc7b2dce696538d8b54e58688d4a`; preserved all earlier worktrees and recovery branches. The last full gate stopped in the held-model runner with a missing generated `mm_nei_graph.inc`.

- RED: `python3 -B tests/mm_nei/run_held_tests.py` reproduced the missing native graph include.
- Native graphics helper generation is now shared by the main and held runners, and the held compiler receives its temporary include directory. The fixture initializes bounded OPA, XLU and overlay buffers because real native Open/Close touches all three; after adding the include, the previous uninitialized overlay caused SIGSEGV.
- GREEN: the same held runner compiled and executed successfully: `PASS native MM held model dispatch`. No production game code was changed for this harness repair.
- The complete 43-command regression gate and exact-candidate Windows/Linux builds remain pending. Recovery records stay on checkpoint branches; the final PR publication preserves its existing ancestry and excludes added recovery/workflow documentation.

The first complete run reached the quest-held group and reproduced the same missing-include dependency in the third and final `MM_REAL_RENDERER` consumer. Independent publication review found no Critical or Minor issues and this one Important harness issue. The quest runner now uses the same native helper generation/include directory; its OPA/XLU/overlay buffers are initialized and bounded. `python3 -B tests/nei_quest_held/run_tests.py` passed all native/foreign, partial-mod, fallback, missing-resource, wrist-pose, legacy-loader and boot-material scenarios after the repair. The final full gate must still pass.

## Complete source gate passed

`bash scripts/diagnostics/run_combo_nei_regressions.sh` completed all 43 commands with exit 0 on implementation source `8f771524`. Pinned JSON/spdlog/SDL/ImGui/thread-pool/STB headers and the unchanged historical preservation commits were provided, matching CI inputs. Exact resource/GLB parity passed for 62 GI models and 39 held components; all HD icons/routes and optional-pack cases passed. Formatting with clang-format 14.0.6, existing collision baseline, conflict-marker and whitespace checks passed. The worktree has no tracked source changes from that verified implementation. The final PR snapshot excludes the nine newly added recovery/instruction/plan files while preserving all tested implementation/tests and the technical verification reports. Exact-candidate Windows/Linux builds and packages remain the final publishing check.

Published candidate `8cf71afc532917d62f1be61e4a7800dd3c48e0c2` now heads draft PR #34, parent `1c29c83f5d676d02ececa7302c138f7aa46b952a`. Its GitHub tree `712e9f700397bfb880d2511d660a0d166e7e45ec` equals the local release tree; its differences from tested implementation source are precisely the nine excluded recovery paths. Complete-source checkpoint `0c2170f5d5aee8c8fd538caf601d28f7d0e2fcb8` on `recovery/final-polish-harness-20261004-2102` retains all recovery records. Exact-candidate platform CI is being verified.

CI run 37255727563 on 8cf71afc stopped at the standalone editor pool fixture: distro spdlog/external fmt required definitions absent from its link. Reproduced locally under external fmt (RED), added only the explicit `FMT_HEADER_ONLY` test compiler flag, then the entire MM editor suite passed both external and default bundled configurations (GREEN). Native game source is unchanged. Republish this correction on top of 8cf71afc and verify its new exact-head CI run.

The fmt correction was published as `4612bd1a558dd00e8a1bd27a874ce0afb76cca2f`, tree `798b479f0f19f928aca623e03af1baf50d38a4b4`, with complete recovery checkpoint `b4933510197e48133accf7e846df53ad805222cd` matching local source tree `bd671d0c867bf531524c54101b423d649c8179d9`. CI run `37256522886` passed every gate stage before the final Young Epona group, including all 43 NEI commands. That group's whole `z_message_PAL.c` syntax check lacked the production `combo/menu` include path. Local RED reproduced the same missing `ComboBossSoulColor.h`; adding only `-Icombo/menu` makes the runtime fixture and all five translation units GREEN. All seven remaining Epona workflow commands also pass with the pinned local dependencies. No production source, checks or asset paths changed. Republish this harness correction on top of `4612bd1a`; verify the new exact-head complete gate, Windows/Linux builds and packages. The PR stays draft; runtime acceptance and master promotion remain separate.

## Exact-head CI gate complete

Published candidate `1147a1a893ad2e2c0425049cc9b2ed618d944656` heads draft PR #34, parent `4612bd1a558dd00e8a1bd27a874ce0afb76cca2f`, release tree `a13eb980994f4bdf772cf453ce65401fbde13724`. Its full-source recovery checkpoint `4b63a8cb99a7361d67deece7a319506ed8039d95` has tree `b6cd5011119c6d7d96630882592f6cec9ee2ab8e`, exactly matching local source `0ab4371f`. All identities were verified through live GitHub reads. CI run `37257543429` gate job `111597748060` completed successfully on this exact candidate: every workflow regression stage, all 43 NEI commands, the complete Young Epona stage, formatting, asset-collision and conflict checks passed. Windows job `111599947963` and Linux job `111599948016` are running; full build/package acceptance remains pending. This note does not change the published candidate or restart its CI. Continue monitoring this exact run, address any actual build failure, then retain the draft for runtime verification.
