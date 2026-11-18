# Optional Zelda Run POC7

The selected input is `zzzzzzzzzz_Din_Zelda_Run_POC7_AnimationOnly_20261117.o2r`,
SHA256 `0dd723fc6064d0d9855f2ac55e6e32c768700b839fd76385c9a4853761d73f7e`.
It supplies only the accepted 20-frame `gPlayerAnim_link_normal_run_free`
header/data, with native and Alt aliases. No held-weapon/carry/walk/jump-slash
animation, model, skeleton or hand geometry is invented or included.

`Zelda Run` defaults off and has a What's New accent. `Zelda Run Ages` selects
Child, Adult or Both, default Both. The age preference persists while off.
SoH uses its native age, and MM uses the Time Gate's `AdultLink_IsActive` value.
Only local human Link is eligible; transformation/custom forms, Wolf and Mario
keep their own animations. This is independent of seasonal outfit selection.

The two native run-selection sites in each player implementation resolve the
optional header before their native animation helpers calculate length/range.
The shared table and native resource names are never overwritten. Thus a
20-frame replacement does not inherit a shorter current/native run's end frame.
Off, an excluded age, a missing or invalid pack, or any different animation
returns the original selection, including another installed run pack. Changes
apply when native locomotion next selects its run; no mid-frame pose rewrite or
data-only replacement is performed. Both header and data resources are retained
for active SkelAnime lifetime across Alt/cache changes.

The private animation-only archive is
`zzzzzzzzzz_Zelda_Run_POC7_Private_Optional.o2r`, SHA256
`7945d8b41b782641b6427ec63d2e07fa745a61412b92dff16737226fd1651b12`.
`tools/zelda_run/private_poc7.py` deterministically produces it from the input.
Raw motion data is byte-identical; only the header's private data reference is
changed. All paths live in `objects/optional_zelda_run/poc7/`, with corresponding
Alt aliases. The archive contains no `objects/gameplay_keep/` override.

Install the private archive in the active game's mods directory, or both
`mods/soh` and `mods/2ship` in Combo. Remove older fixed-path run overrides so
checkbox Off can restore the normal/current animation selection. Leave run
animations separate from model-only/outfit packs. This archive requires the
matching checkbox source; an older executable simply will not select it.

Verification covers both actual host resource/header types, production age/form
eligibility and resolver, 20-frame header selection, Off/missing/invalid assets,
all age choices, unrelated animations, local-player identity, Alt selection and
retained lifetime, under ASan/UBSan. Actual changed source translation units are
checked against native headers; full gameplay/build acceptance remains pending.
Child uses the same unchanged user-authorized clip, not a newly authored child
pose. Confirm both ages, Off and each age filter, native held-run/walk/jump-slash,
seasonal model switching and Alt settings in-game before promotion.
