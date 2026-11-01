# OoT scene initialization: valid ELF message Blob rejected as a Scene

## Candidate and preservation

This is a bounded source repair on `fix/startup-crash-20261031-poc1`, based on
`1514491385626d2ac06b463eede4c4dadf4b299b` (tree
`f7e611d2b96b2126842583e2185484c36f6bcac6`). It changes the OoT loader/consumer
boundary and its regression checks. No scene files, actor placements, rendering,
audio, progression, save bytes, MM shop code, or accepted master is changed.

The failing executable identifies itself as
`628e891c683f5cd5b02a5570b1572ad62d42dd37`, tree
`31d6341a9135b1d3180760118a45b4715282a0e2`. GitHub's commit/compare responses
identify it as the merge of `7995f7b6354d993850fa7eb4ab43a1932d355f08` into
`1514491385626d2ac06b463eede4c4dadf4b299b`, eight commits ahead. None of the
119 changed paths overlaps this repair or the accompanying four MM shop camera
paths. Apply this bounded repair to the combined current checkpoint; the morning
parent alone does not contain those eight newer commits.

Both failing source files were fetched at the exact `628e891` ref and compared
byte for byte with the unmodified local parent:

| File | SHA-256, identical at both refs |
| --- | --- |
| `soh/soh/z_play_otr.cpp` | `0bc3a0e47401eb71b2fb2b50ca8d12ef6ee6565d4b5ca640c03c805f1b312718` |
| `soh/soh/z_scene_otr.cpp` | `80091133caf12e728f876a4f42530a1d118932ed56935289b38792bcac9f0d1c` |

## Supplied runtime evidence

`Fleet of Harkinian (4).log`, 66,876 lines, SHA-256
`9e9a9e808da8132beee23bdb3275f9056bdd32b22d7f8bf4169c9ff5042fbf66`, contains
many sessions. Its two final sessions are the new build. Each loads its OoT
scene successfully, then immediately stops after attempting to load the same
valid optional Navi hint resource:

| Launch line/time | Last line/time | Configuration and last boundary |
| --- | --- | --- |
| 64584 / 00:45:54.871 | 65465 / 00:46:13.634 | Commit `628e891`; scene 81, spawn 5, entrance `0x18d`, layer 0, respawn 0; Alt 1 |
| 65995 / 00:46:31.154 | 66876 / 00:46:52.075 | Commit `628e891`; same arrival; Alt 0 |

The terminal message is `text/elf_message_field/elf_message_field`, present in
the OoT owner with no Alt override, `result=wrong-type`. Neither new session
contains a textual exception record. The loaded MM owl-save metadata before
these requests is not itself evidence of corruption.

The additional `Fleet of Harkinian(1).log`, 1,547 lines, SHA-256
`54804966bcb49b19efdcd3d0f7821d2795c6ba3389b5f75ae16045ccbb277962`, reproduces
the same boundary during an MM-to-OoT handoff. Build `628e891` is identified at
line 613 / 18:46:39.184. MM returns at line 1529 / 18:49:49.721. OoT requests
scene 91, spawn 0, entrance `0x11e`, layer 0, respawn 2 at line 1541. The
`scenes/spot10/scene.json` resource succeeds at line 1544; line 1547 /
18:49:50.104 again ends on the field ELF message with `result=wrong-type`, Alt 1,
OoT owner active. This log also has no textual exception traceback.

The `field` name is the shared overworld Navi hint bank. Lost Woods's scene
header selects ELF-message table 1, which maps to that resource in
`sNaviMsgFiles`; Hyrule Field uses the same selection. The personal log's
scene 91 (`spot10`) and entrance `0x11e` identify Lost Woods correctly. Loading
this hint bank is expected during that Lost Woods arrival.

### Matching minidump

`Fleet of Harkinian-crash(5).dmp`, 398,131 bytes, SHA-256
`b3cedc00f4a9060af10f204b1aa30f93238d900e590d2c89e0bccc4f459259d0`, supplies
the missing machine-level fault. Its header timestamp is
`2026-10-31T23:49:50Z`, the same second as the handoff failure (18:49:50 in the
log). Exception and module streams report:

| Field | Value |
| --- | --- |
| Exception | `0xc0000005`, read from `0x20`, thread 23016 |
| Fault module | `E:\!SoH UNBOUND\soh.dll` |
| Image base / size | `0x7ff812690000` / `0x8d06000` |
| RIP / RVA | `0x7ff81323f1ea` / **`0xbaf1ea`** |
| PE timestamp | `0x6ae675e6` (`2026-10-31T23:12:38Z`) |
| CodeView identity | PDB GUID `e48cda41-2274-4955-ba28-6eebca934b2b`, age 1 |
| PDB path | `D:\a\ComboShip\ComboShip\x64\Release\soh.pdb` |
| Relevant registers | `RAX=0`, `RDI=0x2754e7f0670`, `RBX=0x2740ec6ed60` |

The memory stream includes the complete small function surrounding the fault.
It checks the special command's global-object field, conditionally spawns that
object, checks its ELF-message index, obtains a file-table entry, calls the
loader, then executes:

```asm
; immediately following the resource-loader call
0x7ff81323f1e5: call 0x7ff81323d9f0
0x7ff81323f1ea: mov  rcx, [rax + 0x20]       ; fault: RAX is null
0x7ff81323f1ee: mov  [rdi + 0x3bcc0], rcx
```

This matches `Scene_CommandSpecialFiles` reading `Blob::Data.data()` after the
generic loader has returned null, immediately before assigning
`PlayState::cUpElfMsgs`. The function identification is from the captured
instructions plus the exact source and last logged call, not PDB symbolication.
The dump establishes a new-build RVA; it does not reuse an older crash offset.

### Older crash records kept separate

All nine textual exceptions in the long log identify commit `9699f8e`, built
October 30 2026 at 00:39:40. All are `0xc0000005` invalid reads reported from
`0xffffffffffffffff`. They are separate evidence from the new null read above:

| Log line | Time | Scene | Top `soh.dll` RVA |
| --- | --- | --- | --- |
| 20877 | 18:32:44.100 | Kakariko Village | `0xb20daf` |
| 27260 | 21:29:23.854 | Kakariko Potion Shop | `0xb20daf` |
| 30374 | 21:57:37.339 | Kakariko Village | `0xb20daf` |
| 37505 | 23:11:46.297 | Kakariko Village | `0xef53a0` |
| 39564 | 23:13:28.016 | Kakariko Village | `0xef53a0` |
| 42527 | 23:14:36.195 | Kakariko Village | `0xef53a0` |
| 44510 | 23:15:25.281 | Kakariko Village | `0xef53a0` |
| 46493 | 23:16:33.402 | Kakariko Village | `0xef53a0` |
| 56976 | 00:01:04.521 | Kakariko Village | `0xef53a0` |

Those older actor/resource failures are outside this repair and receive no new
runtime-success claim here.

### Companion/enemy integration excluded for this fault

An independent causal review compared the morning parent `151449138` with the
failing companion/enemy merge `628e891`. The failing loader, scene-command
consumer, ResourceManager, ResourceLoader, binary factory, cross-game registry
and resource helpers are byte-identical across that merge. The original null
Blob-read reproduces with no companion or enemy implementation linked.

The fault also precedes their execution. `Play_Init` calls `Play_SpawnScene`
before `Actor_InitContext`. The SpecialFiles command fails before the
after-scene-command hooks and `OnSceneInit`. Partner frame callbacks run after
state initialization; enemy spawn recording runs later in `Actor_SpawnEntry`.
The companion resource sets contain no scene or ELF-message overrides.

These independent source and ordering checks exclude the new companion/enemy
integration as the cause of this particular dump. They do not certify every
companion/enemy gameplay path. Keep all eight newer integration commits in the
cumulative candidate; this repair belongs at the generic loader/type boundary.

## Root cause and repair

The exact failing source has this call chain:

1. `OTRPlay_SpawnScene` loads a valid `SOH::Scene` and enters `OTRPlay_InitScene`.
2. `OTRScene_ExecuteCommands` reaches command `0x07`, `Scene_CommandSpecialFiles`.
3. That command requests a `Ship::Blob` from `sNaviMsgFiles`, using the shared
   `OTRPlay_LoadFile` API.
4. `OTRPlay_LoadFile` incorrectly casts every resource to `SOH::Scene`, logs a
   valid non-scene resource as `wrong-type`, and returns the null cast result
   (`z_play_otr.cpp`, original lines 55-60).
5. `Scene_CommandSpecialFiles` casts that null result to `Ship::Blob*` and reads
   `res->Data` without checking it (`z_scene_otr.cpp`, original lines 125-128).
   This is before after-scene-command hooks, environment initialization, and
   scene-init callbacks.

The repair restores the shared loader's generic `Ship::IResource*` result.
`OTRPlay_SpawnScene` validates `SOH::Scene` at its initial load and native-header
retry, retaining the existing bounded recovery and rejecting wrong scene types.
`Scene_CommandSpecialFiles` validates `Ship::Blob` and leaves `cUpElfMsgs` null
with a targeted diagnostic if its optional hint resource is missing, throws,
or has the wrong type. The existing `ElfMessage_GetCUpText` handles that null
state. Successfully loaded hint bytes, OoT resource ownership, Alt selection,
scene/MQ choice, and arrival state are preserved.

## Verification and limits

The new `tests/spawn_recovery/test_special_files.py` compiles the actual loader
and special-file command bodies with production `PlayState`, `Blob`, `Scene`,
`IResource`, and `SetSpecialObjects` layouts and resource implementations. Archive
IO/cache selection, active context, object allocation, and diagnostic logging
are adapters. Both Combo and standalone are built with ASan/UBSan. This is not
a renderer, Windows executable, or full resource-manager integration test.

Before the repair, the field and dungeon Blob cases reproduce the null-member
access in both builds and both Alt states. The wrong-type case additionally
exposes the unchecked C-style downcast. With the repair:

- All 24 special-file cases pass: Combo/standalone, Alt 0/1, and field, dungeon,
  missing, wrong-type, throwing-load, and no-message commands. Valid bytes reach
  `cUpElfMsgs`; Combo restores the prior MM owner after loading from OoT.
- All 26 existing/extended scene-spawn, room-request, entrance-routing, and
  scene-path checks pass. A non-scene generic result is accepted by the shared
  loader and rejected at the scene consumer, including bounded terminal failure.
- Both complete touched translation units, `z_play_otr.cpp` and
  `z_scene_otr.cpp`, compile to native Linux objects in Combo and standalone
  configurations, with real engine and cached third-party headers.
- `clang-format` 14.0.6 reports no differences for the two production files;
  `git diff --check` passes.

Commands for the source regressions:

```sh
python3 -B tests/spawn_recovery/test_special_files.py
python3 -B tests/spawn_recovery/test_spawn.py
python3 -B tests/spawn_recovery/test_room.py
python3 -B tests/spawn_recovery/test_entrance.py
python3 -B tests/spawn_recovery/test_scene_paths.py
```

The new check is added to the existing fast spawn/entrance CI step. Complete
translation-unit checks needed nlohmann, spdlog/fmt, ImGui, thread-pool v4, STB,
SDL2, and libzip headers. Initial missing-libzip/SDL includes were resolved from
the existing runtime/cache; no replacement engine headers were invented. The
standalone source regression itself needs no downloaded third-party headers;
its sole logger shim disables `Resource.cpp`'s trace output.

Full platform linking, the whole project's unrelated regression groups, and the
user's Windows game/mod configuration have not been run here. Candidate state:
implemented, focused sanitizer/static/native-compile verified; runtime acceptance
and master promotion remain unproven. The next decisive runtime check is the
same saved slot's Hyrule Field load with Alt 0 and 1, followed by the supplied
MM-to-OoT scene-91 handoff, confirming the scene header/room and later callbacks
continue past ELF message loading without altering arrival or hint behavior.
