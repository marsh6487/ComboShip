# CI fail-fast recovery checkpoint

Base: `develop` at `54c0e5d2c4302116f998142284b502da2191a40a` (tree `2995b61682e497984fc215d6d2b7b00939998730`). Candidate branch: `fix/ci-failure-visibility-20261021`.

The base's hosted run [39639205569](https://github.com/marsh6487/ComboShip/actions/runs/39639205569) compiled both platforms successfully. The `world` regression failed at about five minutes because forest draw fixtures still referenced removed `ForestGiParticles::kHexVertices`; the final gate waited until the Windows build finished at about 62 minutes.

The candidate puts both builds and all six existing regression groups in one native fail-fast matrix, retains their existing implementations and command inventory, emits immediate regression errors, and links final diagnostics to the original failure. The first checkpoint deliberately retains the existing broken forest fixtures so a draft-PR run can prove that a real regression stops both builds. That checkpoint is not mergeable. A subsequent checkpoint must update and run the stale fixtures, preserve production game code and assets, and pass the CI helper/graph checks and workflow lint before promotion.

The remote candidate branch and its PR retain the complete source tree. Recover it with `git fetch origin fix/ci-failure-visibility-20261021` and `git switch --detach FETCH_HEAD`; compare the commit/tree recorded in the PR. The base remains independently recoverable by its exact commit above. Reverting the bounded follow-up commits restores the earlier parallel workflow and fixtures; no game assets, generated archives, upstream pins, build targets or release workflows are changed by this follow-up.

Source checks and native fixture tests do not establish configured gameplay acceptance. Hosted cancellation and the subsequent complete CI result must be recorded separately in the PR before this work is described as verified.
