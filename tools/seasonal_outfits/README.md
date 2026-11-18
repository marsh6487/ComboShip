# Private seasonal Din archive conversion

This Python 3 recipe routes the accepted adult Din Spring, Fall and Winter
ModelOnly POC3 archives into private seasonal paths. It does not rerig models,
alter geometry, change texture pixels, fit equipment, or add animations.

```sh
python tools/seasonal_outfits/namespace_o2r.py \
  --spring /path/to/zzzzzzzzzz_Adult_Din_Spring_ModelOnly_POC3.o2r \
  --autumn /path/to/zzzzzzzzzz_Adult_Din_Fall_ModelOnly_POC3.o2r \
  --winter /path/to/zzzzzzzzzz_Adult_Din_Winter_ModelOnly_POC3.o2r \
  --output-dir /path/to/candidate
```

The three input archive hashes are pinned in `EXPECTED`. Keep inputs outside
the Git checkout. Outputs use sorted resource paths, fixed ZIP timestamps and
DEFLATE level 6, as the accepted inputs use compressed ZIP resources.

The generic contract for outfit families is
`objects/seasonal_outfits/<set>/{adult,child}/<season>/gLink{Adult,Child}Skel`.
This first family uses set `din`, with adult canonical skeletons
`objects/seasonal_outfits/din/adult/{spring,autumn,winter}/gLinkAdultSkel`.
Original canonical body, hand, eye and mouth symbols remain as private aliases
beside the skeleton. Other private resources keep their original suffix under
`resources/`. Native and Alt aliases stay private. The original Alt-only custom
FPS hand wrapper receives one private native compatibility alias, using the
same draw commands with only its marker reference changed.

The recipe classifies the original gear resources and removes any wrapper
that calls outfit-specific arms or hands from the shared set. The resulting
500 common gear resources are byte-identical in all three accepted inputs and
are extracted once into `Adult_Din_Shared_Equipment_POC1.o2r`, with their exact
existing paths and bytes. Two new private mount markers identify that archive.
Held-hand wrappers stay private and retain calls to the shared gear.

`Seasonal_Conversion_Validation.json` records source/output hashes, every path
mapping, shared resource identities, decoded vertex/texture counts and original
external dependency hashes. It verifies that reverse reference rewriting
restores every source resource byte exactly; native vertex, texture and matrix
arrays are byte-identical. Binary skeleton/limb path fields and widened display
list reference fields are rewritten with their actual serialization schemas.
Unknown schema changes, new unresolved references or original body alias leaks
are rejected. Four pre-existing external equipment hashes remain unchanged.

The source selects an installed outfit family independently of the ordinary
player model; it supports vanilla Link or a custom model as that fallback.
Summer and young seasonal models are absent intentionally. Their routing uses
the ordinary selected model until accepted assets exist. Animations remain
separate. The output requires the matching ComboShip seasonal routing source;
archive validation does not establish in-game behavior.
