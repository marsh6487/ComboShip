# Bowling Alley dependencies

Run `python3 -B tests/bowling_dependencies/run_tests.py --sanitize` from the repository root.

The test compiles the complete native bullseye and wall actors, native movement
helpers, and real actor/save headers in both original and combined build modes.
It checks repeated updates with no Bowling Lady or an unrelated NPC, discovery
of a Lady that appears later, failed child allocation, the resting/active-round
camera checks, target hits, both wall falls, and exactly one target for the next
round. Audio, visual effects, collision-resource registration, and actor
allocation are test boundaries; the dependency and minigame state machines are
production code.

NPC soul authorization is unchanged. Its existing missing/owned/off and
vanilla/credits controls run separately with
`python3 -B tests/shuffle_parity/souls/run_runtime_tests.py`.
This is a native logic regression, not a configured gameplay acceptance test.
