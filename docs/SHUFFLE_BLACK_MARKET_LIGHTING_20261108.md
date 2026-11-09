# Black Market shelf lighting source correction

The user screenshot shows a black Chateau Romani bottle with visible shimmer.
The manual MM shelf route bypassed Actor_Draw and inherited whatever lights the
previous actor/effect left behind. The bundled bottle materials multiply texture
color by lit SHADE; the independent unlit shimmer can remain visible. This
identifies a real source omission and a plausible screenshot cause, not GPU proof.

DrawMarket now uses the exact native Actor_Draw light policy for each live stock
actor: new frame-arena lights, scene bindings at the correct legacy/positional
reference, and native Lights_Draw submission to both opaque and translucent
streams. It preserves selected-item position, native actor flags, sold-out/refill
routing, room effects and seals. There are at most eight live stock light groups.

The existing native shelf fixture failed before correction at the fresh-light
assertion. It passes after correction in both hosts under the CI combined
float-matrix/wide-vertex compiler adapters and ASan/UBSan. Added cases cover
per-item ordering, room/debug positional controls and actor lighting flags;
existing arena, seal, stock and native-position assertions remain. Light-service
boundaries are controlled; real material/color execution remains unverified.

GCC 14 exposed implicit integer-to-pointer assignments in the existing OoT arena
allocator while compiling its real C body. Explicit void* casts retain all
existing alignment arithmetic and permit the fixture to compile without
weakening compiler diagnostics. Initial fixture setup attempts also lacked the
JSON header and its forward declarations; the final run uses the complete
prepared dependency include tree.

The full BlackMarket.cpp production object compiled successfully with the actual
generated command and refreshed MM PCH; source/header/PCH hashes stayed stable.
An independent bounded source review found no actionable defect. Neither object
compilation nor the fixture establishes that the screenshot is visually fixed.
Configured native/Alt rendering and exact running resource identities remain
required gameplay evidence. No asset archive was changed or published.
