# Custom collision adapters under Unbound

Run `python tests/custom_unbound_collision/run_tests.py` from the repository root.
Optional cases: `shared sm64 water grapple actors bridges motion timectl`.

The native GCC tests compile current production functions and game headers with
`GBI_FLOAT_MTX`, `GBI_S32_VTX`, AddressSanitizer and UndefinedBehaviorSanitizer.
They cover:

- OoT and MM door/shelf authored geometry, neutral cameras, camera exclusion and
  player collision flags; OoT table growth from 64 to 128; MM fixed-table failure;
  fixture re-registration after the collision context releases its slot.
- The real OoT OC admission/reset path with 96 cylinders, complete SM64 collision
  snapshots, allocation failure/retry, teardown, vanish floors and wide exit walls,
  dynamic slot 95, vertex index 70000, ignored faces and invalid vertex bounds.
- Full-width seasonal water backup/restore, including repeated cleanup. The
  accepted suppression level remains -32000; suppression in custom worlds below
  that level is outside this preservation change.
- Grappling dimensions using wide coordinates/indices and the current baked Dyna
  polygon range rather than model-space source polygons.
- Stasis/Pacci/magnet/wand shape consumers at slot 95, inactive/deleting slots,
  full-width Stasis dimensions and Pacci impact positions, exact original tree
  profile/camera/climb flags, and protection against deleting a reused slot.
- Authored Valley/Snowhead/Lake bridge vertices, polygon bounds/planes, neutral
  camera/water fields and wide-index duplicate suppression.
- SM64's refresh signature at dynamic slot 95 and precise wide/fractional Switch
  Hook movement for cylinders, spheres, quad midpoints and triangle planes.
- TimeCtl AC cache growth through 160 real native registrations, exemption and
  duplicate policies, hard-stop and partial-slow hit-frame timing, allocator
  failure/retry, original clock restoration and owned-cache scene/reset cleanup.

Rendering is not executed. The extracted native units replace logging, return the
native decision from `GameInteractor_Should`, and disable second-update/frame-
advance hooks. They do not prove those hook configurations, native Dyna transform
baking, full game linking, MSVC/platform builds, asset/Alt combinations, randomizer
progression or gameplay. MM's unchanged headers emit existing button-macro
redefinition warnings in this standalone test setup. Runtime proof and user
acceptance remain separate.
