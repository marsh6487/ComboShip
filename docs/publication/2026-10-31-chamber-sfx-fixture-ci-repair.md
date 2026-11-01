# Chamber SFX preservation fixture and native Unbound controls

PR #77 run `41261568693`, head
`19542bbfcfe218d681d0b4a9885b5f6c2f224aec`, passed its fast source gate and the
equipment, entrances, items, rendering and systems regression groups. The world
group passed its first 46 rows and stopped at its final Chamber SFX preservation
fixture. The Linux build job passed; Windows compilation was still running when
the failure was investigated.

The same sanitized command reproduced the failure locally before this change:

```sh
python3 -B tests/temple_sages_bundle/run_chamber_sfx_tests.py --sanitize
```

The fixture expected the earlier 4,096-resource pool and hashed every resource
outside four authored room roots as one protected set. Native Unbound packaging
deliberately adds seven room/scene/collision controls, producing 4,103 payloads.
The old count assertion therefore stopped the test before its preservation
checks, and those added controls also require a separate preservation boundary.

The correction names exactly those seven controls, requires all of them, and
retains a 4,096-resource original set. Its four binary-room hashes, additive
emitter records, child Zelda placement and exact 4,092-resource protected digest
remain unchanged. Both canonical and Alt room JSONs now explicitly verify one
correct emitter in each child/adult setup, with all 14 actor records retained.
No production code or packaged asset is changed.

Verification:

- The formerly failing sanitized runner passes through the actual
  `scripts/ci/run_regressions.py` entrypoint using the unchanged world task.
- All five existing Unbound controls/collision/actor roundtrip checks pass.
- Five separate negative controls fail as expected: removed canonical child
  emitter, altered Alt adult emitter, changed original protected bytes, missing
  native collision bulk control, and an unrelated extra resource.
- The production audio-guard C fixture still compiles and executes with
  ASan/UBSan, and the original protected-resource digest is retained verbatim.

The prior publication/format checkpoints and original eight-commit recovery
bundle remain intact. This fixture-only descendant requires a fresh CI run;
source/native verification does not establish configured gameplay acceptance.
