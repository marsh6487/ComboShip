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
only real host allocation calls. Queue growth is forced to move its allocation in
savestate cases; the fixture tracks actual allocated addresses and sizes. Each
owning context releases all tracked allocations.
LeakSanitizer is disabled in the managed host because its process tracing prevents
LeakSanitizer from inspecting `/proc`; AddressSanitizer and UBSan remain enabled.

Storage belongs to an external owner record keyed by `CollisionCheckContext`'s
address. Owner records, live queue pointers and epochs are not in the game heap,
so restoring heap bytes cannot overwrite authoritative ownership or registry links.
`ClearContext` preserves allocations,
zeros all reserved entries and counts, and respects SAC. Repeated `InitContext`
clears/reuses owned storage. `DestroyContext` performs DinFireSword teardown before
unregistering and freeing authoritative live buffers. Every initialized context
must be destroyed before its enclosing storage is released. Queue buffers may move when registration
grows them; consumers must read the context pointer at use time. On allocation
failure admission returns `-1` without changing the accepted entries/count, and a
later registration retries. `colATCapacity`, `colACCapacity` and `colOCCapacity`
report actual reserved entries. `COLLISION_CHECK_*_MAX` names remain compatibility
aliases for initial reservations and no longer impose admission limits.

Savestates use the C APIs declared in `z64collision_check.h`:

1. `CollisionCheck_CaptureQueueSnapshot` copies all three reserved queue arrays,
   their counts/capacities, SAC flags and fixed OC-line entries into an immutable
   snapshot. Null signals failure; no live queue changes.
2. `CollisionCheck_PrepareQueueRestore` checks context address and lifetime epoch,
   then stages independent buffers. Every allocation must succeed before restoring
   any game-heap bytes. A snapshot may prepare repeated loads during the same epoch.
3. Validate all external restore plans with `CollisionCheck_ValidateQueueRestore`
   before the heap copy. With no intervening queue calls,
   `CollisionCheck_CommitQueueRestore` immediately after that copy cannot allocate
   or fail. It frees authoritative current buffers and rebinds the restored context
   to the staged buffers and saved payload. The plan is consumed once.
4. Free abandoned or consumed plans with `CollisionCheck_FreeQueueRestore`, and
   release snapshots with `CollisionCheck_FreeQueueSnapshot`. Freeing a plan never
   frees live transferred buffers, even after its original context was destroyed.

Every Init, including repeated Init at a live address, begins a different epoch;
Destroy unregisters it. Wrong addresses, destroyed/reused addresses and old Init
epochs are refused before allocation or writes. Successful queue/SAC/line mutations
invalidate already prepared plans; they do not invalidate snapshots. Changes to a
different context do not invalidate a plan. The caller also validates scene/room
lifetimes before queue preparation; the queue epoch alone describes Init/Destroy.
Raw heap copy by itself does not restore queue contents and must not be followed
by collision dispatch before the prepared queue commit.

Additional native savestate cases exercise save, forced moving growth, raw heap
rewind, restore, Clear/register, late native AT/AC and OC contacts, and teardown;
original pointer order, saved capacities, SAC bits and OC lines; repeated loads;
three contexts with changed registry order; stale plans; every snapshot/preparation
allocation failure without heap mutation; zero-capacity queues after failed initial
reservations; and abandoned/consumed handle destruction after owner teardown. A
separate teardown repro rejects freeing saved stale allocations even if the raw
heap was rewound without committing its queue plan.

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

The current savestate revision passes all eleven queue cases normally and with
ASan/UBSan, SkullPickup normally and with ASan/UBSan, all nine Goron collision
cases with ASan/UBSan, and the SwitchHook instant-swap runner. A complete
`z_collision_check.c` syntax check passes using the combined candidate's real
CMake compile database. The full executable build and configured in-game
savestate proof remain integration checks, separate from this native evidence.
