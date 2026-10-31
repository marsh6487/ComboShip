# Command Melody acquisition candidate

Parent: `349a4188f` (native companion/enemy candidate).

Vanilla OoT/MM learn the existing song while active, including older saves.
Randomizer saves never use this grant. Both solo generators supply the song
independently of unrelated options; combined generation sources its one normal
copy from MM. No save-layout or song-identity change.

Black Market `wildcard.command_melody` is a nonrenewable tier-1 offer (80 rupees),
eligible in OoT, MM and combined stock. Existing two-wildcard/six-NEI stock quotas
remain. Native song handlers grant it; physical song ownership prevents repeat
purchase. Existing saved placements and frozen stock are not regenerated.

The exact source tests and native compilation outcomes are recorded below before
checkpointing. Full build/link and configured in-game proof remain pending.

## Checked

- Nine companion tests pass, including actual acquisition function bodies, active /
  inactive worlds, vanilla / randomized slots, repeated calls and slot switches.
- Four existing custom-encounter tests pass.
- Black Market stock test passes across 4096 deterministic full-catalog seeds,
  all stock quotas and catalog eligibility; OoT native shop mapping/grant tests
  pass with address/undefined-behavior sanitizers.
- Actual MM generator compiles and executes with NEI, quest-item, Plentiful,
  starting-song, solo/combined and Mirror settings varied. Command Melody appears
  once, or zero times when explicitly selected as a starting item. Mirror checks
  retain their original cardinality.
- All six changed native C++ translation units pass Combo syntax compilation;
  both companion adapters and both generator files also pass Solo syntax checks.
- Existing Seasons-mode/computed-start and Grace-mode native pool suites pass
  after their source-extraction runners include the new pool helper.
- Independent code review found no concrete defects after the Plentiful and
  starting-inventory checks were added. `git diff --check` passes.

Native compilation initially exposed missing JSON declarations in the MM include
chains. Both touched MM translation units now include the required JSON header
before the randomizer declarations. No suppressed compiler errors were used.

## Remaining verification

No full executable build/link or game runtime was available for this addendum.
Check vanilla new/existing save learned-song display and control; randomized
unowned/owned songs; shop purchase/receipt and save/reload; solo/combined switching.
This is an implemented, host-tested source candidate, not runtime acceptance.

Publication/consolidation with newer parallel work remains a separate operation.
This addendum starts from the native partner/enemy candidate and preserves it.
