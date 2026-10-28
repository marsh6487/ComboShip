# Growable OoT actor collider queues

The native harness compiles production queue ownership/admission, collider reset
functions, native cylinder intersection math, AT/AC dispatch, OC dispatch and
contact response against the production collider types and `CollisionCheckContext`.
Only external game services, rendering/audio effects and allocation fault injection
are fixture boundaries. The fixture exercises cylinders; sphere/quad/triangle
geometry remains covered by the existing native collision regressions.

Run:

```sh
python3 -B tests/actor_queue_growth/run_tests.py
ASAN_OPTIONS=detect_leaks=0 python3 -B tests/actor_queue_growth/run_tests.py --sanitize
```

Cases cover admission and native contacts beyond the former AT50/AC60/OC50
limits, exact registration order, reuse after clearing, two simultaneous contexts,
uninitialized first init, repeated scene init, teardown/address reuse, initial and
growth allocation failures, frame advance/second-update guards, disabled/dead
colliders, AT damage masks and bounce, OC masks/contact order/displacement, SAC
isolation and invalid indices, and an AT count of 40,000. Allocation faults affect
only real `realloc` calls. Each owning context releases all tracked allocations.
LeakSanitizer is disabled in the managed host because its process tracing prevents
LeakSanitizer from inspecting `/proc`; AddressSanitizer and UBSan remain enabled.

Storage belongs to `CollisionCheckContext`. `ClearContext` preserves allocations,
zeros all reserved entries and counts, and respects SAC. Repeated `InitContext`
clears/reuses owned storage. `DestroyContext` performs DinFireSword teardown before
unregistering and freeing buffers. Every initialized context must be destroyed
before its enclosing storage is released. Queue buffers may move when registration
grows them; consumers must read the context pointer at use time. On allocation
failure admission returns `-1` without changing the accepted entries/count, and a
later registration retries. `colATCapacity`, `colACCapacity` and `colOCCapacity`
report actual reserved entries. `COLLISION_CHECK_*_MAX` names remain compatibility
aliases for initial reservations and no longer impose admission limits.

The host audit found pointer/count consumers in Pacci, SwitchHook, Champion,
Keaton reflector, collision viewers and SM64 surfaces. They do not depend on
embedded array size or retain queue buffer pointers across frames. The Champion
snapshot cap is a separate consumer policy. Expansion of SM64's generated-surface
storage belongs to the combined candidate's custom-collision workstream. Native
test fixtures that wrote embedded arrays directly
now provide their own backing pointers in SwitchHook and Goron collision tests.
The SkullPickup regression uses production growing OC admission and verifies
native token gates after 75 earlier registrations. Its diagnostic uses actual
capacity and float cylinder dimensions; pickup eligibility is unchanged.

This is the queue milestone for the combined cel/Unbound integration based on host
`badc4e09a5176b5d423926eed21d5d19a67ee87e`, on branch
`integration/cel-unbound-collision-20261028`. The queue source incorporates the
exact Unbound collision-coordinate donor delta from
`ecd889c2019b87e78b0a3d942a6a3fe98b7efaed` to
`ace55888e181fabf7dfccd60f30d3f7fb2cef46b`, including float quad midpoints.
CPU and sanitizer checks do not establish configured in-game behavior or user
acceptance. The next runtime check is a crowded OoT room with a late token/prop,
then room transition/re-entry and scene teardown, within the combined candidate.

Native checks also passed the SkullPickup test (normal and ASan/UBSan), all nine
Goron field-collision cases under ASan/UBSan, and the SwitchHook instant-swap
runner. A complete `z_collision_check.c` syntax check passed, with its existing
`gSPVertex` pointer/integer warning; `z_en_si.c` passed with format warnings treated
as errors. `scripts/ci/run_regressions.py --suite equipment` passed the queue,
SkullPickup, Pictograph and Keaton melee checks before stopping at the unrelated
Blast Mask test because `spdlog/spdlog.h` was unavailable. That full equipment
group is dependency-blocked and has not been claimed passing.
