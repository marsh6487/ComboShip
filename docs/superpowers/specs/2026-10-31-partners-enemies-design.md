# Native companions and enemies candidate

Status: native approach selected; scope expanded by cor to natural encounters
throughout BOTH games and What's New accents for every integration. No feature
code implemented yet.

## Intent and selected approach

Bring Skijer's Malon/Cojiro companion behavior and four custom enemies into
cor's cumulative ComboShip source. cor selected native source integration over
importing the full external mod API. Cucui Ganon is excluded.

Native integration adapts donor registration, hooks, equipment and UI to the
existing host. Importing the external mod loader was the alternative; it would
retain donor packages more directly but require a much broader engine change.

## Exact baseline and preservation

- Local parent: ba37349b82ee855c2f7179cd00b9b5871891f418.
- Published equivalent: marsh6487/ComboShip branch
  fix/nei-item-regressions-20261031 at
  1514491385626d2ac06b463eede4c4dadf4b299b.
- Both have tree f7e611d2b96b2126842583e2185484c36f6bcac6.
- Isolated candidate branch: candidate/partners-enemies-20261031.
- Preserve cumulative actor, scene, collision, equipment, NEI input, audio,
  chest, cross-game, boss-soul icon and renderer changes. Indexed triangles
  remain default off. This source identity is verified; runtime acceptance of
  the parent is not inferred from its published status.
- Existing Miniblin, Hammergeist/Molmauk and Scissors Beetle actor sources
  already exist for OoT and MM. Audit build inclusion, asset availability,
  registration and spawning before replacing any implementation.

## Donor and provenance

Donor is skijer/Shipwright, branch unbound-mod-nei, observed head
abf41181d690643f74ab72926840918b15fbba3e. Pin file/blob identities again at
implementation start. Relevant mod-sdk/mods folders:
malon_companion, cojiro_companion, command_melody, lynel_miniboss,
trutefel_miniblin, trutefel_molmauk and trutefel_sbeetle.

Retain author and license material. Malon's baked animation source is Luv4os's
CC0 Romani Partner, pinned by the donor to
cced2388ea8c8a9082346ce70e227d805162a8ee. The enemy ports credit trueffel,
syeo501 and Skijer. Check each donor asset's available provenance before
including it; do not describe missing assets as delivered functionality.

## Behavior to preserve

Malon follows Link, recalls safely, persists through room/scene transitions,
uses a bow against nearby hostile targets, can be carried/thrown and rides
behind Link on Epona. In OoT she matches Link's child/adult age. Companion
callbacks remain separate from original Malon quests and ranch music.

Cojiro is a friendly blue cucco. Preserve following, obstruction hops,
carry/throw, slow falling while carried, trade-check unlock and saved
summon/dismiss ownership. Three hits temporarily dismiss him. Preserve the
ordinary randomized Grog reward and existing completed-check eligibility.
Do not patch shared cucco textures or enable revenge-swarm behavior.

Command Melody supports selecting and directly controlling either companion,
camera-relative movement, their actions and returning to Link. Preserve the
donor's song sequence. Control releases on transitions, damage, invalid actor
references and excessive range. Link remains the true player; do not use the
companion to load new rooms or advance entrances.

Enemies: Red-maned Lynel (melee, shield, bow, headshot stun and mounting),
Miniblin (rupee theft), Molmauk (ice/fire hammers, slam/stomp), Scissors Beetle
(returning pincers). Preserve existing enemy behavior unless a donor update
resolves a demonstrated integration defect.

## Implementation decisions proposed by the implementer

- Individual companion toggles default off, in a Partners section of the
  existing menu; no companion updates or spawns while disabled.
- Add native registration, explicit spawn support and enemy-randomizer
  integration for all four enemies in BOTH games. The checked MM source has
  no equivalent of OoT's ExtraModes/EnemyRandomizer.cpp; implement a native MM
  replacement adapter instead of implying existing support.
- Add a separate default-off Natural World Encounters checkbox that works
  without item randomizer or enemy randomizer. Author sparse additive
  encounters across Hyrule and Termina, using a versioned placement table.
  Miniblins suit ruins/camps, Scissors Beetles wooded/rocky areas, Molmauk
  spacious combat areas and Lynels isolated clearings. These habitat choices
  are implementer proposals, not user-selected exact locations.
- Keep entrances, NPC hubs, progression-sensitive rooms and authored bridges
  clear. Check actual available collision before spawning; skip an invalid
  placement rather than moving existing geometry or NPCs. Never auto-replace
  an additive encounter through Enemy Randomizer. Track defeat within the
  current scene visit, so room changes or enabling a toggle cannot farm a
  defeated placement instantly. Re-entry can respawn like ordinary enemies;
  MM cycle/reset and OoT age changes must have defined cleanup/reset behavior.
- No enemies or companion state updates from these features when disabled.
  Turning the natural encounter option off removes its surviving actors only.
- Use the existing combo/gui/WhatsNew.h gold New and cyan Updated control
  accents in both native menus for EVERY integration: Malon, Cojiro, Command
  Melody controls, each custom enemy selection, randomizer integration and
  natural encounters. Preserve current labels, IDs, navigation and the global
  What's New off switch. Register accents as each working control is added;
  do not advertise unavailable functionality.
- Include deterministic asset packaging and required asset identities, with
  graceful unavailable behavior when an optional asset pack is absent.
- Reuse baked rigs and animation clips; do not introduce live retargeting or
  per-frame mesh repair. Preserve existing NPC replacement resources; check
  rig compatibility rather than assuming a custom replacement matches the
  native companion rig.
- Land OoT and MM adaptations as separate milestones. Goal is both games;
  MM needs native model/animation/resource ownership and movement adapters.
  Do not expose an MM option until its required implementation and assets
  are present. OoT completion does not imply MM parity.
- Ordinary native animation blending remains available. Performance claims
  require measurements with each feature enabled and disabled.

## Boundaries and hazards

This is several actor integrations and supporting control plumbing, not a
single merge. The current OoT source does not carry the donor's SOHModApi
host. Translate the limited API calls into native ActorDB, hooks, menus,
custom equipment and save accessors instead of adding an imitation broad API.
Check existing MM API independently; it is not proof of matching OoT ABI.

Ownership and toggles persist in versioned NEI save data with defaults for
old saves. Actor pointers are transient, never serialized. Cross-game swaps,
room transitions, disabling and save reloads release control and cleanup
resources. Dormant game modules must not spawn companions or apply input.
Respect both Solo and Combo guards and preserve the existing mod load order.

## Verification and acceptance

For each milestone: inspect bounded diffs, compile affected translation units,
run meaningful native tests for save migration, registration, control release,
trade rewards and dormant-world guards. Run relevant existing regression
checks. Record missing build dependencies or unsupported test environments.

Runtime checks separately for each game: summon/dismiss, follow/recall, room
and scene changes, save/reload, pause, carry/throw, damage and control release;
Malon bow/Epona and child/adult where supported; Cojiro trade and slow falling;
each enemy spawning/combat/drop/death, Lynel stun/mount. Check native and
affected Alt Assets paths and the actual required pack/load order. Compare
frame time with companions disabled and enabled in the same scene.

For natural encounters, verify every authored placement's scene/room, age or
cycle condition, collision floor and clearance in the installed authored
scene configuration; missing proof is explicitly untested. Test options off,
natural only, randomizer only and both. Test room/re-entry, defeat, disabling
mid-fight, transitions and MM Song of Time. Verify What's New on/off styling
for each control in both menus without changing gameplay settings.

Do not claim runtime proof from a source check or successful compile. Keep
accepted masters unchanged until cor accepts the candidate. No public release
or protected-branch merge is part of this request.

## Recovery

Commit each independently verified milestone and retain its parent/donor
identities, known failures, build results and asset requirements. Save a
durable recovery artifact before broad code changes and long builds, and
verify that it restores independently. Do not rely solely on this transient
worktree. Rollback is the pinned baseline plus the selected bounded commits.
