# Renderer performance preservation contract

The current deliverable is the source investigation and concrete renderer proposals in `docs/poc/renderer-investigation-20261029.md`, using the existing RenderFlight evidence. Preserve full scene geometry, model replacements, texture resolution, material passes, effects, animation, collision and gameplay.

Do not add or ship a new diagnostic build. The experimental texture-only probe has been withdrawn and all of its source hooks, tests, CVar and CI entry removed.

The main structural recommendation is indexed triangle submission. Smaller proposals address repeated identical matrix work and avoidable triangle/DX11 submission work. The report identifies exact files, mechanisms, invalidation risks and meaningful native verification. These proposals are not implemented speedups. Keep the separately verified loader ABI repair distinct from performance claims. No protected merge or release.
