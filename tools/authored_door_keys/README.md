# Original authored door-key get-items

Source handoffs exclude GIFs, videos and animated previews at cor's request.
The reviewed key rotation was delivered separately; it is not required by
the game, the asset build, or the regression checks. Animation generation
remains optional for local visual review and must use an output directory
outside the source handoff.

`build.py` authors both production models from mathematical contours and deterministic noise. It does not read any existing model, texture, ROM, mod or private donor asset. The production files are native XML display lists/vertices, original static weathering textures and original reflection maps. The preview loads those serialized production files, including both material passes; it never renders an authoring-only mesh.

From the repository root, with Python 3, NumPy and Pillow installed:

```sh
python tools/authored_door_keys/build.py
python3 -B tests/authored_door_keys/asset_test.py
python tools/authored_door_keys/build.py --check
python tools/authored_door_keys/preview.py
```

`build.py --check` compares every regenerated resource byte and the manifest without changing files. Normal generation writes only the two dedicated `objects/combo_door_keys` asset directories and `manifest.json` here. Current previews default to `tools/authored_door_keys/previews/sheen/`; `--output PATH` chooses another destination. Labels use the installed DejaVu Sans font.

The optional `--animation` flag generates a local rotating GIF. That preview is excluded from repository publication and is not required by either game's resources, build, or tests.

- MM owns `objects/combo_door_keys/RustyKeyDL`: an imperfect forged oval bow, substantial cylindrical shaft, stepped collar and beveled warded bit. Original procedural rust/pit detail stays fixed on the surface while exposed iron picks up a broad native reflection.
- OoT owns `objects/combo_door_keys/OverworldKeyDL`: an octagonal brass bow with a true open hole, small geometric maker's escutcheon, beveled blade, asymmetrical cut teeth and narrow blade channel. Tarnish/patina stays matte while exposed brass picks up the reflection.
- Draw both roots at **0.25**, with shared native draw code applying **local Z rotation −45° before inherited turn/Y spin**. The upright source geometry and all original UVs/normals remain untouched. Upright authored heights remain 70.75 and 70 world units.
- Names remain distinct between owners, including every child display list, vertex list and texture. All dependencies are bundled beneath the owner's resource root.

The meshes contain closed intersecting components, an intentional economical construction for GI rendering. Each component is closed, has outward winding, and has positive volume after integer quantization. No boolean union or collision mesh is needed. Physical pitting is represented by texture, rather than microscopic polygons.

## Native material passes

1. An opaque metallic base uses the existing native `G_TEXTURE_GEN` / `G_TEXTURE_GEN_LINEAR` pipeline with an original soft 16×16 RGBA16 reflection map, its own primitive/environment metal colors and native lighting. Generated coordinates come from the view-relative vertex normals; there is no time-scrolling texture.
2. The exact same geometry is reloaded with static UVs and drawn as `G_RM_AA_ZB_XLU_DECAL2`. Its 32×32 RGBA32 weathering coat retains the original RGB values exactly as decoded by the native RGBA16 loader. Orange oxide, green patina and dark pits are opaque; exposed metal has restrained translucency. The resulting broad highlight moves beneath fixed weathering.

Each pass uses one texture at a time. The coat is exactly 4 KiB and the reflection tile is 512 bytes, so no simultaneous two-texture TMEM allocation or unsupported independent-UV trick is required. Root exit disables texturing/texgen, restores white environment color and restores opaque render mode. Each invocation establishes its own material colors, independent of inherited CSMC tint.

`manifest.json` distinguishes unique mesh triangles from submitted triangles: 972/1,944 for iron and 744/1,488 for brass. Vertex resources are shared by the two passes.

## Preservation and preview limits

The reviewed upright matte baseline is preserved in git checkpoint `b0854044` and `baseline/`: original resource ZIP, manifest, report and contact sheet. Focused tests verify all approved vertex resource bytes and native-decoded RGB values remain unchanged. The original `previews/` images remain available; updated sheen/diagonal previews are under `previews/sheen/`.

`offline_render.py` evaluates the serialized two native passes, including per-vertex texgen, primitive/environment interpolation, static texture alpha and coplanar decal composition. It uses fixed approximate diffuse lighting and canonical look-at axes; it adds **no PBR/specular highlight, environment art, geometry, normal map or preview-only texture**. The moving highlight comes exclusively from the production reflection map. The optional local animation shows a 3.24-second turn. Native camera/look-at setup, lights, depth bias, filtering and actual GI framing still require configured runtime inspection. See `REPORT.md` for commands, hashes and implementation evidence.
