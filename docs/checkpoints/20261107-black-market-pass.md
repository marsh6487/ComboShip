# Black Market source candidate, 2026-11-07

Baseline: restored regression/Ranch source tree `961a132fd3c9576678ce76847e6f0533b7e42763`, local commit `f55b8e11`, on exact source base `93309d745b260fa1eba1a55d658db717ee350d0f`. Prior source/asset fixes remain preserved.

Changes:
- Exclude Command Melody, Great Fairy Mask, Mask of Truth, Gibdo Mask, Garo Mask and Captain's Hat from new wildcards and replacement candidates. Keep all six lookup records so old seeds and purchased offers remain loadable. Fourteen wildcard choices remain.
- Save effective replacement keys separately from frozen seed stock and purchased bits. Replace owned unpurchased nonrenewables and unused retired offers with a deterministic unowned, usable, game-compatible and unique item. Match tier and prefer the same wildcard role, with same-tier role fallback if exhausted (needed by legacy OoT Command Melody). Keep original shelf price and dungeon-clear gate. If no candidate exists, retain the blocked offer. Renewable Chateau refill never rerolls.
- Refresh once on shop entry or after a purchase at safe idle boundaries; freeze during dialogue or pending receipt. Persist changes immediately through each host's save path. Receipt fingerprint includes immutable market and effective keys, excludes purchase bits so native receipt drawing can finish after acknowledgement. OoT waits for asynchronous saves before mutating live replacement strings.
- Purchased nonrenewables render the native Sold Out GI in their existing shelf position. They remain selectable for sold-out dialogue. Purchase bits survive reload; renewables keep normal merchandise and purchase behavior.
- OoT fixed shop rooms temporarily allow the native player item-get camera only for a matching active receipt. Suppress fixed viewpoint replacement during that view; native cancellation/tail returns normal room policy.
- MM source asset preparation now adds a separate native global ITEM_GET cutscene and assigns the player's spawn chain to it, preserving the original four shop camera records/chain and decoded yaw. Runtime readiness validates the new record and assigned player ID. Normal source preparation regenerates bundled resources; no external scene input is needed.

Verification:
- Production stock policy: 4,096 generated inventories plus replacement determinism, reload, legacy, malformed overrides, exhaustion, purchased and renewable tests.
- Both hosts' native transaction bodies with address/undefined sanitizers: saved effective offers, no mutation during quotes/receipts, exact acknowledgement/debit/save, stale receipt protection, entry and return recovery.
- Native rendering tests submit MM Sold Out display list and OoT Sold Out GI, retain refills and other stock.
- Actual OoT native camera request/setting path in a fixed room; actual MM shop/player item camera manager, all five forms and eight shelf selections, cancellation/ownership and ordinary-shop parity.
- Six MM generator tests execute actual resource factories and native player cutscene assignment; 141 production scene-readiness failure/lifetime cases.
- Both production Black Market C++ translation units and OoT z_play.c compile with real headers. This is bounded object verification, not a full platform executable build.
- Independent reviews cover shared stock policy and combined host integration. The review caught an OoT asynchronous save ordering race; a regression assertion checks that saves see the old fingerprint before replacement publication.

Remaining acceptance: full platform executable build and configured in-game visuals. Test both hosts: enter with an already-owned offer, confirm saved replacement and unchanged price/gate; buy once and observe native GI framing; see Sold Out immediately, after re-entry and after reload; verify Chateau refill can be bought again. A higher-priority old MM scene override with only four cutscenes will fail readiness; use the regenerated scene from the same build.

No seed fill placement changes, save editing, or accepted external archive changes. Optional HD Wolf remains external. Source publication has not been performed in this pass.

File-select follow-up: generation percent previously drew at y=184 while the goal footer drew at y=188. Progress now shares the existing y=168 status row (`Generating... 5%` / `Finalizing... 100%`), preserving the footer, localized phase text, alpha and non-combo layout. The actual file-select production C unit compiles successfully. Configured game screenshot acceptance remains pending.

NEI equip follow-up: both hosts now consume all matching C/D sources when relocating an NEI to a C button. The displaced C item moves back only once; extra copies clear their item/slot/full-ID metadata. OoT previously returned directly for every NEI assignment; MM stopped after the first matching source, allowing later D-pad copies to remain. Full identity matching also handles stale source slot metadata and MM legacy truncated extended IDs. Adding a D-pad shortcut retains the existing independent shortcut behavior. B, stored ages/forms and unrelated items are preserved. Existing progressive-family swaps remain supported.

Verified with failing baseline reproductions, sanitized actual-helper tests (all four D directions × all three C targets, empty/occupied targets, byte/full-ID items and preexisting duplicates), complete OoT NEI control group, combined/solo MM equip cases, MM duplicate input/dispatch tests and the Magic Mirror follow-up group. Both production ext_buttons.cpp units compile with real headers (MM's console-variable declaration supplied by its normal precompiled-header dependency). Fresh read-only review found no blocking issues. In-game acceptance/full executable build remain pending; save format is unchanged.
