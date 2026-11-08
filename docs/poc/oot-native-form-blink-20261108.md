# OoT native FD/Deku blink candidate, 2026-11-08

| Field | Record |
|---|---|
| Baseline | Cumulative source d294398; unchanged renderer checkpoint c28791f; initial candidate238c32b retained |
| Scope | OoT transformation renderer only; FD/Deku native named eye loads plus optional companion segment 08 eye bank |
| Preservation | Original shared display lists, native geometry/UV/material commands, open-eye draw, Goron/Zora/Gerudo bindings, MM player rendering, audio |
| Configuration | Native MM form resources mounted in OoT; optional Henriko blink pack supported; native mm.o2r recovered for offline texture/DL inspection; no executable/runtime configuration available locally |
| Evidence | Host C++ helper compile and pixel tests; command/resource model tests; diff whitespace checks |
| Verdict | Implemented and helper/static verified; full native compilation and in-game appearance untested; not accepted/promoted |
| Recovery | c28791f restores renderer before this candidate; accepted input recovery ZIP unchanged |

The companion patch alone binds segment 08 but native heads use fixed texture
loads, so it cannot animate the default forms. The candidate additionally copies
the active native head list into frame memory and replaces only the exact named
native eye load. Hash commands become standard raw-image loads plus a no-op in
the old hash word. All other commands remain byte-identical. Custom skeletons
with differently named heads are excluded; segmented HD materials stay on their
existing path. Native geometry is not substituted into a custom head.

The base Half/Closed pixels are derived at runtime from the exact base RGBA16
64x32 resource; no optional archive is required for this route. Deku reduces the
actual per-column alpha opening to half height/a bottom seam, preserving RGB and
never adding opaque pixels. Fully opaque/transparent sources are rejected.
FD now uses bounds measured from the actual native 64x32 eye atlas, preserving
brows/forehead and cheek slashes. Skin and seam colors come from unchanged native
cheek/lash pixels. Offline review confirmed three distinct native frames; white
eye pixels decrease 130 → 70 → 0. Deku's actual native alpha footprint decreases
857 → 444 → 54 opaque pixels, retaining RGB and never expanding alpha. Both
native head lists contain exactly one hash load of the named eye resource.
This supersedes the donor-UV-only mask in the first candidate. In-game motion,
lighting, UV sampling, transitions and user acceptance remain untested.

The fallback refuses other dimensions/formats/flags and any active atlas whose
bytes differ from the exact base atlas. Consequently a third-party replacement
without its own blink material/bank keeps its appearance but does not receive the
procedural base blink. The supplied optional Henriko segmented bank remains
supported. Its eight named paths and inherited blink-only selection are retained;
directional expressions are outside this change.

Pixel buffers have static aligned lifetime and a separate identity per state.
Head instruction caches reset at form pinning and on Alt changes; submitted
instructions are frame allocations, never mutable shared archive lists. Failed
base derivations can retry on later draws. Base pixel buffers, once generated,
are immutable, preserving pending-render pointers through cache/Alt changes.

Verification run:

- `c++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined tests/form_blink/pixels_test.cpp -o /tmp/mm_form_blink_pixels_test`
- `ASAN_OPTIONS=detect_leaks=0 /tmp/mm_form_blink_pixels_test` passed. LeakSanitizer itself cannot run under this environment's ptrace, so its unsupported startup failure was excluded; address/undefined checks remain enabled.
- Plain `-Wall -Wextra -Werror` and sanitizer pixel builds/runs passed for the final native-mask correction.
- `python3 tests/form_blink/render_helper_test.py` passed. It compiles the exact production helper against a resource/frame model and verifies hash/filepath loads, preserved commands/shared lists, even pointers, distinct states, custom-head exclusion, changed-art exclusion, Alt invalidation, and cache reset.
- `python3 tests/form_blink/native_assets_test.py /path/to/mm.o2r /path/to/evidence-output` passed against the recovered base archive. It compiles the production pixel helper, verifies actual native resource formats/alpha/colors, locates exact named texture hashes in both head lists, and emits the inspected frame contact sheet and SHA-256 report.
- `git diff --check` passed for the owned changes.

Native input: Library `libfile_5b0bd74b2a0c81919b38bba6d5479e2b`, filename
`mm.o2r`, 36,251,870 bytes, archive SHA-256
`f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59`.
Recovered by filename search with `top_k=100`; the earlier `top_k=5` query had
omitted it. Native textures/geometry are not embedded in the source checkpoint.
Their identities and reproducible verification are recorded in
`docs/poc/oot-native-form-blink-assets-20261108.json`.

Decisive runtime checks still required: default FD and Deku with the optional pack
absent, native Alt off/on, optional Henriko bank with Alt on, repeated transforms,
pause/unpause and scene re-entry. Inspect open/half/closed eyes and face markings,
not merely that a blink counter advances. No game was launched and no full
configured application build was performed in this workspace.
