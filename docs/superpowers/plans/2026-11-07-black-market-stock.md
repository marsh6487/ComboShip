# Black Market stock and purchase presentation

Goal: remove six requested wildcard choices from new stock, replace already-owned unpurchased nonrenewable offers, retain a Sold Out model after purchase, and release the browsing camera for native item-get presentation.

Baseline: restored source checkpoint tree 961a132fd3c9576678ce76847e6f0533b7e42763, based on 93309d745b260fa1eba1a55d658db717ee350d0f. Existing rain, foliage, potion, Saria, cane, Wolf and Ranch music repairs remain included.

Design: immutable seed market identifies the save. Effective replacement keys live in saved state alongside purchased bits. Replacement candidates must be unowned, usable, game-compatible and unique; preserve shelf price, tier and dungeon-clear gate. Prefer matching wildcard role, fall back within the tier for a legacy slot whose role has no active choices. Keep a slot unchanged if candidates are exhausted. Retain retired catalog records for old saves; exclude them from generation and replacement candidates. Never refresh during dialogue or a pending receipt. Purchased nonrenewable slots remain Sold Out; renewable refills do not.

Implementation and verification:
- [x] Shared stock policy: generation exclusions, effective offers, deterministic refresh, strict override loading. Test legacy seeds, duplicates, exhausted pools, save/reload, purchased and renewable slots.
- [x] Both hosts: native Sold Out GI and camera handoff/restoration. Exercise actual production rendering/camera bodies where fixtures permit; verify receipt cancellation and camera ownership.
- [x] Host integration: refresh before browsing, persist replacements, use effective offers for display and grant, freeze receipt state.
- [x] Fresh independent review, targeted native tests and real production translation-unit compilation as available.
- [x] Save a cumulative recoverable source artifact, independently restore its tree and identify remaining runtime checks.

Runtime acceptance still requires both game hosts: buy an item, observe the traditional item-get framing, return to browsing, see Sold Out, leave/re-enter/reload, and verify refill remains purchasable. No seed fill placement changes or scene archive changes are included.
