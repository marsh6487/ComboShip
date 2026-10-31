# Native partners and custom encounters — candidate source

This candidate extends the morning bugfix tree `f7e611d2b96b2126842583e2185484c36f6bcac6`.
Published matching source: `marsh6487/ComboShip`, `fix/nei-item-regressions-20261031`,
commit `1514491385626d2ac06b463eede4c4dadf4b299b`. The local equivalent parent is
`ba37349b82ee855c2f7179cd00b9b5871891f418`. No protected branch was changed.

## Delivered source

- Native Malon and Cojiro actors in both games, optional following and companion actions.
- Owned Command Melody control using native input/camera hooks; C-Up returns to Link.
- Persistent Cojiro ownership and equipment summon/dismiss, with additive cross-game ownership.
  OoT uses the completed Grog Cojiro trade check; MM uses Grog's chick quest completion.
  Original quest rewards are not replaced. Temporary recovery clears on file load.
- Existing native Miniblin, Molmauk/Hammergeist and Scissors Beetle reused; native Lynel added.
- OoT existing Enemy Randomizer pool extension; MM deterministic outdoor ground-enemy replacement.
- Independent additive natural encounters, with bounded ownership and live collision checks.
- Nine matching What's New gold control accents in both native menus.

These are implemented candidate sources, not a linked release or accepted gameplay.
Cucui Ganon is excluded. Indexed rendering and existing cumulative gameplay defaults are unchanged.

## Controls and resources

OoT: Enhancements → Extra Modes. MM: Enhancements → Gameplay.
Malon Companion, Cojiro Companion, Natural Custom Encounters and Include Custom Enemies
are off by default. The four enemy selections default on but need one of the encounter
modes enabled. OoT custom randomization additionally requires the existing Enemy Randomizer.
MM custom randomization replaces eligible native outdoor Tektite/Deku Baba entries only.

Command Melody Control defaults on, requires the existing learned song and an enabled,
nearby companion; it does not grant or shuffle a new item. The current native sequence is
**A, C-Right, A, C-Left, A, C-Right, A, C-Left**. The unchanged donor README's six-note
sequence is stale. Following and ordinary companion actions do not require the song.
Target a companion to select it, otherwise the closest eligible partner is selected.
A jumps, B uses the companion action, C-Left/Right turn the camera, C-Up releases control.
Start remains available. Disable, damage, ground loss, world changes, actor loss and
excessive distance release camera/input ownership. Link does not follow across room boundaries.

Build packs with:

```sh
python3 scripts/mods/package_partners_enemies.py --oot-base /path/to/oot.o2r --output /path/to/candidate-assets
```

Place the generated archive under each engine's existing mod directory, keeping the
existing cumulative archive set and loader order. These use ordinary resource paths,
not Alt-only resources, and do not replace shared cucco textures. The MM pack additionally
contains the required OoT Malon/cucco resources and Cojiro icon copied from the supplied
base, plus their referenced dependencies. Output metadata records the base checksum.
Incompatible skeleton limb counts or absent resources reject companion initialization.
Do not install a competing version of the same partner/enemy resource paths alongside it.

Verified inputs generated these reproducible packs:

| Engine | Resources | SHA-256 |
| --- | ---: | --- |
| OoT | 403 | `6a0382770249cae43453c6d898abd3e4b4e6fd5f4920990d979b097e9fa23140` |
| MM | 605 | `19af9f46e141a86665bb08185e6f0d40ffd013470a9c267fe5d8d00da7178a9d` |

## Natural encounter inventory

The region table is in `combo/encounters/NativeCustomEnemies.inc.h`. Locations are
anchored to original native ground-enemy/spawner scene entries, recorded before enemy
randomization. No base scene resources or existing actors are replaced by natural mode.
The first two distinct eligible entries are candidate sites, offset 240 units on X/Z.
Actual additions require static, dry, reasonably level floor; radial floor/wall and
ceiling clearance; room match; distance from Link and his entry point; NPC/door clearance.
A rejected collision site is skipped for that visit with one bounded log message.
Some region/asset/scene configurations can therefore have fewer than two additions or none.
This is a curated candidate inventory, not runtime-approved coordinates.

| Game | Region (scene ID) | Eligible species |
| --- | --- | --- |
| OoT | Hyrule Field (0x51) | All four |
| OoT | Zora's River (0x54), Lost Woods (0x5B) | Miniblin, Beetle |
| OoT | Lake Hylia (0x57) | Beetle, Lynel |
| OoT | Gerudo Valley (0x5A), Haunted Wasteland (0x5E) | Miniblin, Lynel |
| OoT | Desert Colossus (0x5C) | Molmauk, Lynel |
| OoT | Death Mountain Trail (0x60) | Molmauk, Beetle |
| MM | Termina Field (0x2D) | All four |
| MM | Southern Swamp clear/poison (0x00/0x45), Road to Swamp (0x40), Path to Mountain Village (0x1C) | Miniblin, Beetle |
| MM | Ikana Canyon (0x13) | Molmauk, Lynel |
| MM | Great Bay Coast (0x37) | Beetle, Lynel |
| MM | Road to Ikana (0x53) | Miniblin, Lynel |
| MM | Mountain Village winter/spring (0x50/0x5A), Path to Snowhead (0x5B), Path to Goron Village winter/spring (0x5D/0x5E) | Molmauk, Beetle |

At most two additions exist per region visit. Defeats persist through toggles/room changes;
scene re-entry starts a fresh visit. Actor destruction clears identities before allocation
reuse. Disable removes only this feature's surviving additions. Natural and replacement
modes can run together; custom actors are never replacement source entries. Large OoT
replacements are restricted to wilderness, and both adapters validate floor/clearance.
Clock Town, Market, ranch/quest hubs, authored interiors and progression rooms have no
natural additions.

## Verification and remaining runtime work

The new executable tests exercise production policy/native function bodies and the shared
native encounter adapter at engine boundaries. Actual translation units were syntax-checked
against game headers in Solo and Combo configurations. The source recovery delta was
applied to an independent index and its complete resulting tree compared.

```sh
python3 -B -m unittest discover -s tests/partners -v
python3 -B -m unittest discover -s tests/custom_encounters -v
python3 scripts/diagnostics/run_combo_save_compat_tests.py --json-include /path/to/nlohmann/include
python3 tests/custom_encounters/compile_native.py --dependencies /path/to/test-deps --json-include /path/to/nlohmann/include [native-source-paths]
```

`NLOHMANN_JSON_INCLUDE` supplies the JSON include root for the save test. The native compiler
fixture expects imgui, native-headers, spdlog/include, fmt/include, stb, thread-pool-v4/include
and sdl2/include beneath its dependency root. Add `--solo` for Solo. Full `z_actor.c` TUs need
`--legacy-pointer-warnings` for existing resource-path/Gfx signature mismatches; new actor
TUs use strict implicit-function and incompatible-pointer errors. The full player TU is
blocked in the available baseline by missing `expansions/ssbb/ssbb_spawn.h` and existing
animation pointer mismatches. This work does not claim a full rebuild or link.

| Runtime acceptance case | OoT Solo/Combo | MM Solo/Combo |
| --- | --- | --- |
| Default-off world, cumulative NPC/scene/item fixes | Untested | Untested |
| Follow, carry/throw, bow, recall, Epona seat | Untested | Untested |
| Cojiro unlock, pause summon/dismiss, three-hit recovery, slow fall | Untested | Untested |
| Owned-song control, C-Up, pause, damage, disable and camera restoration | Untested | Untested |
| Save/reload and dormant-world switch while controlling/carrying/riding | Untested | Untested |
| All four enemy combat/death/projectiles; Lynel mount/buck interruption | Untested | Untested |
| Every candidate region/site, both modes together, absent/Alt assets | Untested | Untested |
| Room transitions, age change or Song of Time reset | Untested | Untested |
| Performance A/B against the morning source | Untested | Untested |

No game runtime was available. Host fixtures and compilation do not establish rendering,
placement feel, progression safety in every installed scene variation, or user acceptance.
The existing time-pedestal regression runner also fails at its baseline struct-extraction
fixture before execution; it was not modified to force a pass.

## Provenance

All donor additions are pinned to `skijer/Shipwright` commit
`abf41181d690643f74ab72926840918b15fbba3e`, `mod-sdk/mods/` folders named after these
integrations. Imported serialized blobs were checked against Git's blob SHA1s. The original
three enemies already existed in the morning source and were retained after native checks.
Their preserved manifests credit trueffel, syeo501 and Skijer; Lynel credits Skijer (its source
also describes the generated/rigged mesh). Malon's original reference material and CC0
license remain under `malon_companion/reference`, pinned to Luv4os's Romani Partner commit
`cced2388ea8c8a9082346ce70e227d805162a8ee`. Unchanged donor READMEs are reference documents,
not instructions to install a second external mod loader for this native integration.
