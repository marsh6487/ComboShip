# Young Epona riding and cosmetics in both games

This candidate starts at ComboShip `56dcde31b355d71c7c58844c764c1ef04ce06b26`,
the latest cumulative MM rainbow-cache, persistent-drop, rain and unrestricted-item
candidate. Its parent Windows and Linux build workflow passed. It carries forward
the existing Midna POC3 and contents-based chest integration.

The riding donor is Shipwright's `integration/nei-weather-static-actors` at
`f5a992078e89ea95ba55a55aaf20d537a272538e`, including its mounted-entry,
mount-access and ranch-handoff fixes. Only the SoH/OoT engine receives the riding
port. MM retains its existing riding behavior.

## Controls

| Engine | Location | Controls |
| --- | --- | --- |
| OoT | Enhancements > Quality of Life | Ride Young Epona as Child |
| OoT | Cosmetic Editor > World & NPCs > NPCs | Adult Epona and Young Epona: Coat, Eyes, White Hair |
| MM | Cosmetic Editor > Epona | Coat, Eyes, White Hair (Mane / Forelock / Hooves) |

Riding defaults off. Learn Epona's Song, enable the checkbox and reload the area.
An ocarina and the song's notes are required, including randomized-note checks.
Native horse areas, fences and movement restrictions remain in effect. Young
Epona's saved location is independent of adult Epona's; the young riding actor
does not spawn or draw in the adult timeline. Existing saves initialize the new
young-horse data safely. ComboShip's merged-save callbacks remain intact.

The two editors use separate settings. Reset restores the original appearance.
Coat, eyes and white hair change independently; saddle and bridle controls are
outside this candidate. The ordinary young ranch actor uses the young cosmetic
settings too.

## Assets and compatibility

Put `Young_Epona_SoH_POC1_Assets.o2r` in `mods/soh`. It supplies eight privately
named riding/mount-animation resources and is unchanged from the SoH candidate:
SHA-256 `d1990518522c4b781a9b62f3b2df21e9aec5df0eae99542892093359ecab945d`.
MM's native Epona cosmetics require no additional archive.

The separate `TP_Epona_POC4_Cosmetics.o2r` is an optional replacement for the TP
adult Epona POC3 archive. Use it with this executable and Alternate Assets, and
disable the older TP Epona archive so only one version supplies the horse.
The original POC3 archive remains unchanged. POC4 retains its skeleton, skin
vertices, animations, source textures and twenty cached eye vertices. Its only
changes to existing resources are the body/head display-list path strings; new
private resources supply isolated color passes. The archive manifest records
exact checksums and preservation checks.

Native material support is restricted to verified display-list templates. HD
texture replacements using those UVs are supported; arbitrary replacement
geometry falls back to its original drawing. TP POC4 has an explicit separate
protocol. Color updates use small GPU commands with cached immutable masks and
display lists. They do not recolor HD texture pixels on the CPU or flush the
global texture/resource caches each frame.

## Other requested fixes

The latest donor's Zora shield feet anchor and animated swimming-root transform
were missing from this ComboShip parent and are included here. Its stronger
matrix fixture fails the old position and passes the corrected one. The Midna
movement, visible-time blinking, cosmetics and idle-yawn follow-ups were already
present, with ComboShip's independent opt-in gates retained. Existing chest and
Midna production diagnostics pass.

## Verification and acceptance

The build-artifacts gate includes the four riding production harnesses, young
asset bounds/dependency checks and cosmetic regressions. Local archive checks
also exercise the actual OoT and MM native horse resources. Affected full C/C++
translation units are checked against the real engine and dependency headers.
The candidate remains separate from `develop`.

Build/static checks and offline asset previews do not establish in-game
acceptance. Check the same save and mod order used by the prior cumulative
build: mount/dismount, song recall, fences and scene transitions; save/reload and
child/adult separation; each color and Reset in both editors; blinking, HD/Alt
Assets, and OoT to MM to OoT transitions. Confirm the TP POC4 eyes remain attached
through idle, gait, rear and jump animations. Check the Zora shield grounded and
swimming with the active model stack.

Rollback is the parent executable and prior TP archive, or disabling child
riding and resetting the cosmetic rows.
