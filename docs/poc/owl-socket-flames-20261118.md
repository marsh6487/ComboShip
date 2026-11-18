# Owl socket flames: accepted POC4 implementation candidate

Visual direction accepted: translucent blue flames filling the eye sockets,
calm book illumination, and the existing neutral NEI get-item shimmer. Preview
acceptance is not in-game validation of this implementation.

Baseline: POC8 rectangular pedestal/upright book archive, SHA256
`c8922c8397cdd5de61196cbbc2268ecc75b7dfbee99dedb2a3fcbd727cbc9d04`.
Candidate: `zzzzzzzzzz_MM_Owl_SocketFlames_POC4.o2r`, SHA256
`dbdfafa43a210ee149ff03ed6bb9f0ae64e87d47015e513ded7db5bc57a14f73`.

The candidate archive replaces the native/Alt opened-model roots only. It
removes the old eye lobes and page-surface glow, retaining all original closed
geometry, pedestal/book/writing, and page-border glow. Runtime draws those
layers in XLU using bounded frame allocations. Flame textures reproduce the
accepted field at 32x32, 60 frames per eye, maximum alpha 166/255. Native tick
120 repeats the six-second field; atlas samples advance every two ticks. Book
writing UVs remain fixed beneath a low-contrast broad illumination sweep.

The world actor and all three GI routes call one shared renderer: native MM,
direct imported MM GI in OoT, and MM foreign OPS recipes on OoT checks. The
marker is queried in the selected model owner's resource manager. No new
dynamic segment dependency is added to a shared display list. Mesh positions
are normalized before fixed-point packing; temporary matrix scales and donor
owners are balanced and XLU colors/matrix/setup are restored. The closed pose
never draws these layers. With the asset absent, the ordinary owl retains its
existing model; GI shimmer remains controlled by NEI Item Effects.

The new archive **requires this source candidate or a later build containing
these hooks**. An older executable will have no replacement flame/page layer.
Put it in the MM mods directory in Combo or standalone 2Ship, replacing older
owl test archives. Both native and Alt aliases are included. OoT imports these
resources from MM; do not install this MM object replacement as an OoT pack.

Verified: deterministic archive/reference/native-Alt checks; all 119 untouched
original resources byte-identical; page vertex data exact; bounded normalized
policy coordinates, cycle/UV/opacity ownership checks; sanitized policy test
with leak detection disabled for this environment. The production renderer
fixture passed with both native MM and OoT headers, including absent/present
marker, page/flame vertices and colors, donor ownership, frame/loop selection,
matrix restoration and allocation failure. Existing mask/bottle/fairy/season
and bounded-shop checks passed. See tools/owl_effects for portable rebuilding
from the baseline.

Pending: full executable build and gameplay evidence. Test activated/closed
world owls, opening animation, native MM receipts, imported/foreign OoT checks,
Item Effects on/off, Alt on/off, scene re-entry and game switching, plus an
unrelated GI afterward for leaked render state. Preserve the accepted POC8
archive for rollback until this configuration passes.
