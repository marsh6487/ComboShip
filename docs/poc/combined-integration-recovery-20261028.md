# Combined integration recovery

This is one in-progress ComboShip candidate containing Rich cel shading, Unbound 9.3, growable OoT AT/AC/OC queues and expanded Render Flight resource capture. Source checkpoints are authorized on the candidate branch. Accepted masters, original authored archives, protected branches and releases remain unchanged.

## Restore

Clone `marsh6487/ComboShip` and check out `integration/cel-unbound-collision-20261028`, or open draft PR #69. Restore the published Git tree rather than relying on a scratch path. The protected host is `badc4e09a5176b5d423926eed21d5d19a67ee87e`, tree `7cf77119f87361102e9de1464c2c20adab925205`.

The latest published production repair is remote `d17956c4a1f9e4e45317edaf57f6963f3ff3321c`, local equivalent `a9d57fdcaa125ac001bb1b56028780529f2350bc`, exact tree `d617ae98ee540b8be37f6d556a31b623aa1ad5da`. It includes the wide custom-light positions and legacy-message fallback. The final native validation below adds three test fixtures and recovery evidence to this source. API and local commit IDs can differ; their complete tree equality is checked before advancing the remote ref. The branch tip and PR identify subsequent final validation commits.

Exact donor pins, design decisions and preservation requirements are in `docs/superpowers/plans/2026-10-28-cel-unbound-collision.md`. Review findings and repairs are in `docs/poc/combined-review-fixes-20261029.md`. The first recovery milestone was remote `7c62ffd49f09ffe904d96f5cf5e03487ce23bfae`, tree `30bf3cd1b86de024c79ed80d0b8e4512d644bed5`.

## Verified build and package evidence

- Mandatory fresh Linux full build: exit 0, 2791/2791 commands, shared engine, both game modules, UI and combined launcher linked.
- Explicit comborando target and final generated-provenance relink: exit 0, 157/157 follow-up commands. Selected CTest checks passed 3/3 after relink: both night-music fixtures and archive normalization.
- Actual production-module smokes: `ci-linux-smoke` and exact door-only `ci-door-interiors` workflow arguments both exit 0, one completable seed each, respectively 198 and 200 cross-game placements.
- Both new port archives passed every ZIP CRC, canonical metadata, unchanged payload hash, deployed-copy and required-resource checks. All six default shader copies match the engine; three protected custom Prism shader blobs are unchanged; all five cloud images match their source PNG pixels. Exact hashes and commands are in `docs/poc/combined-archive-validation-20261029.json`.
- Complete prior Linux bundle: 412,980,917 bytes, SHA256 `07372dc7fc3427fd37e5f535f6eda65321b333847a8be3f8402c7dc4434eb792`; all 21,701 payloads plus manifest were read back from the deterministic tar. User ROM archives, saves and configuration are excluded. SoH pack-target exit was lost during an exec transport outage; its completed archive, Done/normalization/copy log and all payloads were independently verified. The MM pack target exited 0.
- CI run `40799473977` at source `94210f74fe2fd3b210d673029c22779caf403488` passed source preflight, all 52 equipment/actor groups together, and both full Linux and Windows build/package/upload jobs. The other five suites failed old extraction assumptions and required the repairs and native reruns below; this CI run as a whole did not pass. Windows engine/UI/launcher preflight run `40799473876` passed.

Native production-backed tests additionally cover queue admission/contact/order/OOM/lifetimes and save restoration; MM legacy/wide matrix and vertex writers; immutable vertex offsets and actual hidden-visibility shared-library dispatch; JSON/legacy routing and transactional scene/save ownership; custom collision consumers; cel policy, 65,536 shader IDs, backend state and generated GLES compilation; complete retained capture tables and overflow; compiler ABI adapters and deterministic archive packaging.

## Complete native regression coverage

All 259 manifest groups have passing retained CI/native coverage: equipment 52, items 46, systems 36, entrances 10, render 69 and world 46. This combines successful prior CI groups with current native reruns; it is not one uninterrupted 259-group invocation. Exact names, commands, evidence and segment caveats are retained in `docs/poc/combined-native-coverage-20261029.json`. The final complete October 20 world group exited 0 in 480.71 seconds; its native forest set executed 44 tests and explicitly skipped 15 optional-resource tests. Local ASan/UBSan remained active; only unsupported LeakSanitizer process scanning was disabled.

Old extraction fixtures assumed fixed entrance tables, packed surfaces, inline dynamic actor tables, cached vertex-pointer packet mutation, or 16-byte runtime vertices. Their updates retain the original behavior assertions and use actual native bodies and ownership. Follow-up checks include real bootstrap registrar/schema bindings, spin lights, exhaustive regional materials, lake/snowhead/valley collision, native arrival fades, wide water-height restoration and OoT/MM wand parity. The full GI renderer preserves its original 5120-vertex shop budget while checking aligned vertex/list/matrix allocation and real arena limits. The final door fixture checks the native transition-index side channel above 63, and the portal fixture checks actual float matrix round trips alongside the unchanged legacy packed-byte oracle.

A final targeted audit found ten OoT custom light calls that still narrowed XYZ to s16 before the new f32 API. The position-only correction passed all 30 native coordinate controls and all eight original/new Zora checks under ASan/UBSan, including a post-format run. Separate RED evidence demonstrates the original insertion/orbit truncation and all 30 old-call mismatches. Colors, radii, offsets, audio, progression and MM's native s16 API are preserved. The prior bundle above remains a verified checkpoint; the final bundle must be refreshed after this tested source fix and matching rebuild. Existing verified port payloads can be reused if their hashes remain identical.

The production source and native fixtures are frozen for the final matching binary refresh. Remaining work: refresh binaries, CTest/smokes and bundle manifest; run the final platform CI gates; then hand off one identifiable candidate. Do not label prior CI harness failures as a successful whole run. Exec-server interruptions can lose process exit status; check live processes, logs and completed outputs before resuming incomplete work.

After checkpoint `40dbb7cb0f50f50fb621cf1c6bf806a24de58c8e` / remote `aa252592e1665ee3d89800525d765101790fe191` / tree `6f2f9d6d772a8fbc03935a19bb90bc805c6413b4`, the world suite exposed a real legacy-message fallback regression: unregistered tables could miss `OTRMessage_Find`, produce a null raw segment and crash in native `Message_OpenText`. The normal, Japanese and credits readers now retain the hash fast path and restore original host linear lookup/raw segment and size semantics. Null/sentinel/empty paths use static native termination streams (JPN aligned u16 `0x8170`, normal/credits byte end). Actual native Find/OpenText controls pass for owned and legacy hits/misses/empty tables, PAL fallback and all original story precedence/decoder cases in solo and Combo under ASan/UBSan; actual production C syntax also passes. A narrow independent review found no defects or material coverage gaps. Glyph drawing remains a configured-runtime check.

The refreshed 40dbb7c bundle passed build/CTest/smokes and archive checks, but is a prior build checkpoint superseded by the tested message repair. The latest source still needs the final matching rebuild/package and platform CI gates. Do not hand off that prior bundle as runtime accepted.

## Build and compatibility

Use one matching engine + SoH + MM build. `GBI_FLOAT_MTX=1` and `GBI_S32_VTX=1` are a shared ABI: runtime matrices use floats and runtime vertices occupy 24 bytes, while supported archive records remain 16-byte v0 or 22-byte v1. Do not swap an individual game DLL into an older runtime. Torch already matches the pinned donor tree and retains the vanilla v0 exporters.

Use the repository/CI dependency pins and normal CMake/Ninja Release build, then explicitly build `comborando`, `GenerateSohOtr` and `Generate2ShipOtr`. Existing `docs/BUILDING_LINUX.md` gives the normal setup. Session-only `/tmp/combined-build-env` logs and dependency caches are conveniences, not recovery prerequisites.

Unbound prepares `oot-unbound.o2r` from clean game archives before supplemental NEI/mod mounts. Original inputs remain available; legacy scene-header overrides, Alt preference and archive priority are retained. Cel opcodes use 0x4c–0x4f, float fog 0x41; existing ComboShip commands are reserved. Toon bit 33 preserves Prism IDs in bits 17–32, and explicit custom Prism materials retain their shade inputs. OoT queues grow from AT50/AC60/OC50 while preserving native policies; MM queues are unchanged.

## Runtime acceptance

Configured runtime acceptance remains separate: test the user's actual Kakariko scene/packs for late Skulltula/items, bomb and stick pot hits, bomb grottos, custom entrance collisions and night/camera/Keaton stability, then OoT/MM inventory and transitions. A comparative performance experiment is not a prerequisite, and no FPS improvement is claimed.

Render Flight includes all collected resource/texture/pass and actor/stage rows, explicit unknown/overflow, and a dedicated complete `logs/OoT-RenderFlight-*.log` capture independent of normal log level. Its default detail mode is 2. Resource timing is CPU submission/driver-wait time, not per-resource GPU timestamps or unique GPU residency. Existing DX11 aggregate GPU queries remain separate. Collection/log bounds and disabled overrides are recorded rather than silently omitted.
