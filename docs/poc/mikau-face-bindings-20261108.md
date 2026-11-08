# Mikau bridge face resources and blinking

## Scope and baseline

This follow-up is based on cumulative POC6 commit
`a9325be94db41b245496a31b805496f8b507f55a`, tree
`6bde44e8bf494f9397109cff0e826790daf9f9c8`, on draft PR #88.
cor approved adding the reviewed Mikau-only source patch and explicitly
requested that this source candidate include working blink selection.

The only production change is the face-binding block in
`soh/src/overlays/actors/ovl_En_Viewer/z_en_viewer.c`. Existing cumulative
integrations, actor poses, blink timing, expression selection, resource
loading, geometry, props, and the separate native MM actor route remain
unchanged. This is candidate source, not a merge or runtime acceptance.

## Defect and correction

The bridge loaded Mikau's face resources from the native MM archive and bound
their raw pixels while alternate display lists could select MM3D geometry.
This produced native eye/mouth artwork on the replacement mesh.

The corrected Mikau branch binds static, aligned `__OTR__` resource names for
the existing selected eye and mouth states. Each row has an even 80-byte
stride, and out-of-range indices fall back to state zero. Other actors retain
their existing bindings. The ordinary host resource path supports alternate
selection and native fallback; runtime lookup remains a separate test.

| Eye state | Resource suffix | Segment |
| --- | --- | --- |
| Open | `object_zog_Tex_024750` | 8 |
| Half closed | `object_zog_Tex_024F50` | 8 |
| Closed | `object_zog_Tex_025750` | 8 |

All resources use the `objects/object_zog/` prefix. Mouth states retain
`object_zog_Tex_025F50` and `object_zog_Tex_026750` on segment 9. The bridge's
existing face resolver holds mouth state zero; no talking behavior is added.

The existing update code waits on the per-instance blink timer, advances
open → half closed → closed → open, and selects a fresh delay. The source
change preserves that progression and lets every selected frame resolve to
the matching alternate resource.

## Regression coverage

`tests/mikau_face/run_binding_tests.py --sanitize` compiles the actual binding
block with strict warnings and address/undefined-behavior sanitizers. It
checks all six eye/mouth combinations, aligned and stable resource addresses,
bounded fallback, and unchanged non-Mikau bindings. The unpatched production
block fails exactly 12 path checks; the corrected block must pass all 138
assertions. Both outcomes were observed in this integration, including a clean
sanitizer run for the corrected source.

The existing `soh/tests/static_story_mm_viewer_test.c` fixture is extended to
exercise Mikau's real initialization/update/face-resolution/draw functions,
compiled with real headers by `scripts/diagnostics/run_regional_actor_tests.py`.
It checks complete blink cycles and named eye/mouth bindings in Mikau's actual
regional draw branch. These checks run through the existing regional actor CI
entry; the strict binding regression is added to the same required world task.

Run the focused required commands from the repository root:

```bash
python3 -B tests/regional_actors/run_tests.py
python3 -B tests/mikau_face/run_binding_tests.py --sanitize
python3 -B -m unittest discover -s tests/ci -v
```

The regional harness substitutes engine boundaries such as rendering and
archive access. It executes the production actor update and draw code but is
not the complete game. Its resource-command checks do not prove image lookup
or visual appearance in the renderer.

### Observed local results

- The new viewer assertions fail against the original binding at the expected
  `__OTR__` signature check.
- The corrected viewer passes 24 complete timer-driven cycles and 1,144 actual
  update calls in each of the solo and `COMBO_BUILD` configurations: 48 cycles
  and 2,288 updates total. Cases cover both poses, Alt disabled/enabled and a
  live toggle while closed, 30/45/59-frame delays, independent instances, and
  repeated drawing without advancing the blink.
- The complete regional actor source suite passes, including native parameters,
  presentation, spawn gates, the viewer chain, and regional placement.
- The exact registered regional/binding CI row passes through
  `scripts/ci/run_regressions.py` and its normal combined graphics ABI adapters.
- All 25 CI policy tests pass. Independent review found no blocking findings.

Local execution used GCC 14.2.0. The existing regional fixture contains legacy
pointer conversions, so an environment-only compiler adapter supplied
`-Wno-error=incompatible-pointer-types -Wno-error=int-conversion`; the pinned
nlohmann JSON 3.12.0 header was supplied through `CPATH`. No production code,
runner, or assertion was weakened for that environment accommodation. The
separate binding harness passes strict warnings with `-Werror` and sanitizers.

The original POC6 Actions run already had five regression-fixture compile/link
failures before this follow-up (equipment, entrances, rendering, items, and
audio/saves/weather). They are not counted as Mikau successes or attributed to
this patch. Candidate platform CI and runtime remain separate gates.

## External model archive and runtime follow-up

The paired model candidate is `Mikau_MM3D_Face_Arm_POC5.o2r`, SHA-256
`1f7a334a17a039116a9508e3509e2639bb1d060db6a4cefe66669929ad37302d`.
It corrects the separate filtered-UV error that causes the thin left-arm
stripe and face atlas misalignment. Its dynamic face commands and all ten
texture resources are retained. The broad gold cuff is authored geometry.

The archive remains external and is not included in source publication.
Use it with the rebuilt source candidate; the archive alone cannot change
the old bridge's native face-resource bindings. Preserve the original POC3
archive separately for rollback.

Full platform build and in-game checks remain required: both Mikau poses,
all three blink frames, the closed mouth, arm seam during motion, Alt Assets
on/off, room re-entry, and cross-game return. Check native MM Mikau separately
because it uses a different draw route. Rebuild all engine-sharing modules
together as required by the inherited POC6 texture-layout changes.

## Recovery

The source delta is bounded to the Mikau binding, its regression fixtures,
CI registration, and this record. The exact production hunk is the previously
reviewed `Mikau_Face_Binding_Proposal.patch`, SHA-256
`60ca16580f8cb415b7220fc1bcfca92dad05251dce0c27407770c529165f7a64`.
The original POC6 commit remains the rollback point. Git history retains the
integrated source and its tests; no asset packs, saves, or gameplay logs are
part of this source change.
