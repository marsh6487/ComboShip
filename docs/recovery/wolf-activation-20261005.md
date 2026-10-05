# Shadow Crystal activation followup

Baseline: `7c17a37e`; isolated candidate `poc/wolf-activation-20261005`.
The user supplied `MM_NEI_failure_trace(1).zip` and requested these diagnoses
be addressed in PR #34. This is an implementation checkpoint, with no game
runtime acceptance or master promotion.

The production Wolf host fixture passes with its explicitly synthetic donor
rig. The 11 available scratch archives were inspected, including both partial
recovery bundles and the supplied Din O2R packs. They contain no real Wolf
binary. Personal context was unavailable. Exact Library Wolf-title queries
and the accessible Shipwright default-branch filename search did not locate
`wolf_link.bin`; that repository has no published releases.

Confirmed boundaries: Wolf rejects the PAK loader's general active predicate,
which includes equipment-only selections and slot mixes. The existing loader
requires a loose `nei/2ship/wolf_link.bin`, while packaging has no copy/install
rule and activation/load rejection reasons are silent.

The bounded correction preserves general equipment activity, adds a separate
body ownership predicate for Wolf, records rejection only on Crystal attempts,
and provides explicit optional real-asset packaging. The synthetic fixture is
not a runtime deliverable. The actual donor binary and in-game acceptance
remain outstanding.

Baseline check: `python3 -B tests/mm_wolf/run_host_tests.py` passed with genuine
MM headers/native input and freeze boundaries; fixture rig and other engine
boundaries remain documented in `MM_NEI_WOLF_RECONSTRUCTION.md`.

First checkpoint: `run_model_owner_tests.py` compiled the complete production
PAK model structure and exact activity/age-selection functions on native MM
headers. The old broad predicate failed the equipment-only ownership assertion;
the new body-only predicate passes selected/forced equipment, slot mixtures,
disabled/enabled/forced body selection and both NEI ages. UBSan is enabled.
Host wiring, diagnostics and packaging remain in progress in this checkpoint.
