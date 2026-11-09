# MM adult voice follow-up implementation plan

> **For agentic workers:** Execute this narrow repair under systematic debugging and verification-before-completion. The user authorized adding the correction to the cumulative candidate; no further design approval is needed.

**Goal:** Honor the existing contract: actual adult Marin PCM when supplied, native OoT adult recordings otherwise, and no automatic adult pitch reduction.

**Baseline:** `e2c4fc737bf087b2f35c879e92875729dc95d9fc` on the active cumulative shuffle candidate. Work is isolated on `fix/mm-adult-voice-trace-20261108`.

**Evidence:** Latest attached log mounts POC3 and exercises modded adult rendering in native MM. It records no provider selection. POC3 SHA-256 `343a067df0edaff08f6da7477e411248b36d2fec8e6cfbe958b30e200f0c1c41` contains 82 adult and 45 child PCM recordings. Existing real-pack fixture passes solo and Combo, but bypasses the player pitch hook and SFX request boundary. Current source returns to the native queue if both adult providers fail, retaining an obsolete `0.85` multiplier.

## Preservation

Keep archive bytes, `custom/din_voice/` compatibility, child/variant/action mapping, native adult donor decoding, custom transformation routing, stop/renewal, manual editor controls, volume, and unrelated cumulative work intact. No runtime acceptance or accepted-master promotion is implied.

## Task: close and expose the fallback boundary

- [x] Extend `tests/link_voice` to exercise production player dispatch, pitch callback and SFX queue. Reproduce automatic adult deepening with optional/native providers unavailable before editing production.
- [x] In `LinksVoicePitchMultiplier.cpp`, remove automatic adult frequency manipulation; retain explicitly enabled editor pitch and custom-form behavior.
- [x] In `link_voice.cpp`, report successful pack validation, precise load failure, first child/adult provider selection and effective pitch, and a bounded warning when native adult fallback is unavailable. Keep logging on the game thread and outside steady per-frame audio mixing.
- [x] In `oot_adult_voice.cpp`, report a bounded precise native-source failure and successful donor availability without changing decoding or retry semantics.
- [x] Verify real POC3 provenance and native archive reads; run narrow dispatch/mixer and native compilation checks. Test missing/invalid optional pack and native-unavailable states without claiming native fallback works when it cannot load.
- [x] Review the isolated diff, commit owned paths, and integrate only the reviewed source/tests/docs into the active cumulative candidate. Implementation checkpoint: `7bf9980911920c4e5202ddb44fd4921a71943896`; cumulative integration: `b0cc40098ab28ab5f64e366b14dfc08ea6e9d2a7`. Configured gameplay/listening remains pending; see the follow-up report.

## Review focus

The reported sound could originate from an old/partially rebuilt executable, failed pack activation, manual pitch, or uncovered demo IDs. The log does not distinguish these. The configure-time `gGitCommitHash` is not conclusive executable identity. Keep those limits explicit and do not claim this patch proves the cause of cor's audible report.
