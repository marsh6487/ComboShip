# Clock return reachability and composed entrance tracking

## Authorized outcome

Cor requested both fixes after an investigation of seed `712281` (display seed
`1401629311`). The recorded runtime baseline is
`a7cac0b273fe7a3c6608d0312afcf291c1ab556d`, tree
`239c5b5b5e69c356253efe234dc4ac9b32a1f3d0`.

With OoT Clock shuffle enabled, a night start must not pass logical MM-start
validation merely because the joint search freely roots MM. OoT must retain a
provable way to open the main return portal from its starting inventory and
reachable placements. Cross-game shuffle must retain its actual topology in
this proof. An authored bridge, Magic Mirror ownership, or an unmodeled Song of
Time reset is not a substitute for the proof.

The OoT entrance tracker must describe the final destination after native and
cross-game routing. The observed regression is physical Ice Cavern `0x0088`,
native GTG `0x0008`, final Fire Temple `0x0165`. Fire's return `0x024a` resolves
before native shuffle to the fountain outside Ice Cavern `0x03d4`.

## Design

Separate the normal portal-bootstrap gate from the logical MM-start return
requirement. Extend the existing prerequisite placement and final return check
to logical MM-start cross-shuffle generation. The closed return probe may
follow genuine foreign dungeon arrivals and collect the items actually reached
there, with existing shared-item views, but begins without an MM overworld root
or free MM pool inventory. The native portal result is distinct from overall MM
reachability. Preserve the No Logic exception and ordinary OoT-start shuffled
routes. Fail clearly if the required native portal oracle is absent.

Add a pure tracker destination resolver. A BeforeNative override of the physical
source wins. Otherwise apply native shuffle once, then AfterNative once. Never
recursively route the resulting endpoint. Return the game as well as its logical
entrance index; MM indices are not OoT metadata keys. Read-only metadata lookup
must support destination labels, filtering, grouping and sorting consistently.
Use the installed catalogue for foreign names and safe fallbacks for unknown
entries. Refresh derived data when the installed seed changes.

## Constraints

- Preserve uploaded save/seed bytes, item placements and entrance maps. Generator
  validation protects newly generated seeds; this does not repair old placement
  reachability by granting items or changing the time.
- Preserve native/solo build guards, source entrance identities and discovery
  visibility. Do not reveal undiscovered destinations or migrate discovery bits.
- Tracker queries must not mutate runtime arrivals, return checkpoints, native
  override arrays, or save data. Do not call the gameplay transition function.
- Keep the accepted baseline and masters unchanged. Work on the isolated
  candidate branch and create recoverable, verified source checkpoints.
- Report fixture/source checks, compilation, full executable runtime and user
  acceptance separately. A standalone fixture is not a gameplay runtime test.

## Acceptance evidence

Production-code regression fixtures must reject the old false return proof and
accept a usable early-Clock/daytime route, preserve real foreign arrivals without
rooting the entire destination world, and preserve OoT-start/No Logic behavior.
Tracker fixtures must cover the exact Ice-to-Fire composition, the BeforeNative
return, foreign game identity, nonrecursive routing, state purity, seed reload,
and native fallback. Run the relevant existing cross-entrance and shared-logic
tests, review both changes, and package exact refs, source, tests, evidence and
restoration instructions. Configured Windows gameplay remains a distinct final
acceptance step if no compatible executable can be built or run here.
