# Combined integration recovery — 2026-10-28

This is one in-progress ComboShip candidate containing Rich cel shading, Unbound 9.3 and growable OoT AT/AC/OC queues. It is not an accepted master or a release. The user authorized continuing source checkpoints on the candidate branch; protected branches and releases remain excluded.

## Restore

Clone `marsh6487/ComboShip` and check out `integration/cel-unbound-collision-20261028`. Use the published Git tree, not a remembered scratch directory. The preserved host is `badc4e09a5176b5d423926eed21d5d19a67ee87e`, tree `7cf77119f87361102e9de1464c2c20adab925205`. The first published milestone is `7c62ffd49f09ffe904d96f5cf5e03487ce23bfae`, tree `30bf3cd1b86de024c79ed80d0b8e4512d644bed5` (local equivalent `6db302b7b`). Later source checkpoints are cumulative. GitHub API commits may have different commit IDs from the local commits; tree equality is checked before advancing the remote ref.

The exact donor references, preservation contract, work ownership and design decisions are in `docs/superpowers/plans/2026-10-28-cel-unbound-collision.md`. Torch already matches the donor's complete tree; its vanilla v0 ROM exporters remain intact. Original accepted scene/archive inputs are not modified by this source integration.

## Current proof and next work

Native production-backed tests passed for growable queue admission/contact/order/teardown/OOM, Skulltula pickup gates, MM legacy and float/s32 writers, custom dynamic collision consumers, cel policy/sky/native hooks, all 65,536 shader IDs, command transport, backend vertex layouts and real generated GLES shader compilation. Actual widened-header syntax checks cover the central interpreter and 21 cel/native translation units. The integrated CMake configuration passed and applies both ABI definitions to every engine/game/tool command. The Linux shared engine linked successfully; subsequent capture edits require the normal final rebuild.

Central Unbound archive routing/save/custom-actor wiring and expanded Render Flight resource output are now integrated. The five new integration groups passed through the checked-in CI runner, including transactional scene state, native collision consumers, mixed archive vertex formats, cel backend state and shader lifetime, compiler ABI adapters and deterministic archive packaging. Frame timing/capture output and analyzer tests passed again after formatting. The original 51 equipment groups passed across retained segments; this is not a claim of one unified exit-zero equipment run. The mask shimmer fixture now compares every vertex field without reading padding and checks the original vertex-count budget against the widened native arena capacity.

Windows compile preflight run `40797872741` passed for source checkpoint `f8e79d4d5b4023eb6ae614a7e729afab59f1c71c` (tree `67c9ab175c0daab3291e0470ef0af7f25503529c`): shared engine, complete UI, launchers and Windows SDK geometry checks. The first Build Artifacts preflight stopped on formatting; clang-format 14.0.6 has been applied to the affected tracked files for the next run. The initial Linux full build reached its last link steps with four already-corrected compile commands outstanding. A fresh full build is running; its complete exit status and matching port archive generation remain pending.

This remains WIP: a successful full SoH/MM executable build, matching candidate package and configured game runtime are not yet proven. Do not infer a scene performance improvement from the native checks. Required runtime checks are the user's Kakariko scene/packs: late pickups, bomb and melee pot hits, bomb grottos/custom entrance collisions, night/camera/Keaton stability, then MM inventory/transition preservation. Preserve those distinctions when resuming from a source checkpoint.

## Build and compatibility

Use one complete matching engine + SoH + MM build: `GBI_FLOAT_MTX=1` and `GBI_S32_VTX=1` are a shared ABI. Do not swap a single game DLL into an older runtime. Vanilla v0 resources are widened by compatible readers; authored wide Vertex/Array resources use version 1. Existing scene mods retain explicit legacy routing and archive priority. Unbound's converted base is prepared before supplemental mods, preserving the original inputs.

Cel opcodes are 0x4c–0x4f; Unbound float fog remains 0x41 and existing ComboShip commands remain reserved. Toon uses shader bit 33; Prism IDs remain bits 17–32. Explicit custom Prism materials retain their prior shade inputs. Queue capacities grow from their original reservations without changing pickup/damage policies.

Normal Linux setup uses CMake/Ninja, SDL2/SDL_net, OpenGL/GLEW, libzip, spdlog/fmt, JSON and the repository-pinned dependencies. During this session `/tmp/combined-build-env/REPORT.txt`, `sources.json` and `dependencies.cmake` record the prepared environment; these paths are temporary conveniences, not required source recovery inputs. Configure a fresh environment from the checked-in dependency pins when resuming elsewhere.
