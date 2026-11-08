# PR #87 early CI fixture corrections

Baseline: public candidate `008534e3b1abcdfa8805a555de8d4639991b740f`,
tree `355ef7377653c3a6de7d8ebea4483fb7716939b7`, stacked on PR #86.
cor requested tracing and correcting the red checks in PR #87. The draft remains
a candidate; no merge or accepted-master promotion is authorized.

## Failures traced from CI

Build Artifacts run `42426099568` exposed four independent fixture failures.
Fast source checks, Windows compile preflight, equipment and entrances passed.
The main platform builds were still running when the failures were inspected.

| Group | Actual error | Correction |
|---|---|---|
| Items | Wallet extraction searched for the old exact rupee `if` condition, which now includes the native `fullUi` guard. | Find the complete current condition around its behavior marker, retain the guard, and supply an explicit fixture visibility input with a false-guard control. |
| Render | Extracted `Actor_DrawAll` used `OotUiDrawScope` and begin/end helpers absent from the celestial fixture. | Import the real bridge declaration and complete production scope helpers; model the actual command-head/tail field types and test visible/hidden/restored diagnostics. |
| Systems | Extracted solo-MM pool prefix referenced `FleetCombo_UnifiedPoolActive` without its dependency. | Include the real public header and actual implementation body, retaining modern standalone/paired semantics. |
| World | The C flow parser rejected the new `OotUiDrawScope diagnostics` declaration before reducing unrelated draw statements. | Add the type name to the syntax-only typedef vocabulary; preserve all native branches, labels, gotos and cleanup calls. |

Each reported failure was reproduced locally before its correction. All existing
assertions remain; no production source, seed data, ABI, sanitizer policy or CI
job selection is changed. The celestial negative control omitting `EndDraw`
fails the new hidden-command-head assertion, so the new coverage detects a
missing scope restore rather than accepting a no-op binding.

## Verification

- Celestial policy: six sanitized fixtures pass. Native actor, sky, fairy,
  light and teardown fixtures all pass with address/undefined instrumentation.
  The actor fixture retains culling, audio, traversal, lens and ordering checks
  and adds all four cel/shadow combinations through visible/hidden/restored
  diagnostics, one visibility sample, head rewinds and retained tail allocations.
- Solo MM: all seven native boundary executables pass, including the previously
  broken pool prefix, save/menu/boss/entry/opening and Triforce completion.
- World: all four commands in the affected room-assets/native-ownership row
  pass, including seven asset checks, both native attachment hosts, real private
  and source XML geometry, palette/resource lifetime and exhaustive draw exits.
  MM retains six branch choices/four goto edges/four end-hook bypass paths;
  OoT retains six choices/three edges/26 bypass paths, with same-draw restoration
  and ownership guards intact.
- Wallet: the sanitized 65,536-balance matrix and new false-guard control pass;
  complete native `z_parameter.c` and `TycoonWallet.cpp` syntax pass using real
  development headers, including the source-pinned ImGui v1.91.9b-docking and
  thread-pool v4.1.0.
- An independent reviewer verified the complete extracted scope helpers,
  actual `.p`/`.d` types, C linkage, retained production bodies and assertions,
  and independently passed the reported native fixture boundaries.

Local LeakSanitizer is disabled under ptrace; address/undefined/bounds checks
remain enabled where selected by the original runners. Real upstream/Debian
development headers are supplied outside the repository, without SDK stubs or
dependency files added to the public source.

The original Linux platform build completed successfully while the fixtures were
being corrected; the full Windows build was still running, with its separate
compile preflight already successful. Fresh platform CI is required on the
follow-up commit. These fixture results do
not prove configured gameplay, audio/visual acceptance or a full local build.
The severe Black Market slowdown remains unresolved. Existing cumulative
integrations, original recovery checkpoints, saves, external packs and the
accepted master are preserved.
