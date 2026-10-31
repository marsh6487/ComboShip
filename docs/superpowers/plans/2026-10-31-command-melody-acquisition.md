# Command Melody acquisition implementation plan

> Implement inline with the existing native companion candidate. User supplied the
> exact behavior and requested the necessary code on October 31, 2026.

**Goal:** Vanilla learns Command Melody by default; rando shuffles it; Black Market
may offer it as a wildcard.

**Baseline:** `349a4188f` on `candidate/partners-enemies-20261031`; edits isolated in
`candidate/command-melody-acquisition-20261031`. Baseline six companion tests pass
with `NLOHMANN_JSON_INCLUDE` pointing at the existing dependency fixture.

**Design:** Set the existing native learned-song bit on active vanilla frames only.
Use the native save-type guards, including OoT's combined randomizer save guard.
OoT Solo supplies one fixed-count song; MM supplies one independently of quest/NEI
settings, reusing the combo swap when present. Combined OoT adds no second copy.
Add one tier-1, 80-rupee, nonrenewable wildcard catalog entry. Reuse existing native
song grant dispatch and check the actual learned bit for duplicate ownership.
Preserve other songs, rewards, stock quotas, existing frozen seeds, and save layouts.
No new acquisition checkbox or NPC teaching quest is needed.

**Approach:** Existing canonical flags and pool/shop hooks are sufficient. A virtual
control-only entitlement would leave pause-page playback inconsistent; a new save
flag/item identity would duplicate an already registered cross-game song.

**Files:** Both `mods/partners/Partners.cpp` adapters; OoT `3drando/item_pool.cpp`;
MM `Rando/Logic/GeneratePools.cpp`; shared `combo/rando/BlackMarket.h`; both native
BlackMarketItems adapters; `combo/gui/WhatsNew.h`; companion/market/pool fixtures;
publication authorization and donor integration notes.

- [x] Add failing tests for vanilla/rando/dormant ownership, solo/combined supply,
  wildcard eligibility and native shop delivery.
- [x] Add native grants, pool hooks, wildcard mapping/ownership and explanatory text.
- [x] Run companion/encounter tests, actual MM pool and native market fixtures,
  and modified native translation-unit syntax checks in Combo/Solo configurations.
- [ ] Inspect the bounded diff, commit, and save a source recovery candidate whose
  binary patch independently restores the exact committed tree.

**Review focus:** Existing vanilla slots; rando slot switches; dormant game state;
combined duplicate supply; already-owned song purchase; unchanged 6/2 and 2/4/2
stock quotas. Full executable build/link and in-game acceptance remain separate.
