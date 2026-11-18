# Seasonal outfit routing recovery and recreation

The recovered base lacked the promised outfit/run source object
`2d89cbf63b9a666a941628abae505a8876662b3c`; its remote fetch reported "not our
ref". The inspected `README_Adult_Din_Seasonal_ModelOnly_POC3.txt` described
one fixed-alias outfit at a time and explicitly excluded automatic routing.
Those findings describe the recovery input, not the recreated candidate below.

## Recreated capability

The candidate adds **Seasonal Player Outfits** and an installed **Outfit Set**
selector in both host menus, with a What's New accent. It is generic: validated
set identifiers select private `objects/seasonal_outfits/<set>/<age>/<season>/`
body resources. Din is the first family, not a renderer limitation. See the
[public pack contract](../modding/seasonal-outfits.md).

Selection uses owned, saved `Seasons_GetSeason()` in both games. It does not use
weather eligibility or editor overrides. `SEASON_OFF`, feature Off, an absent
family/age/season, or an invalid skeleton preserves the ordinary player model
and preferences. Forced models and custom forms retain priority; remote
players do not inherit the local outfit. MM reads its actual Time Gate age.

Private `.private.o2r` discovery leaves ordinary O2R mods and shared equipment
dependencies under their normal global loader. Only canonical private body
roots bypass the existing community O2R body-swap guard. The catalog appends
outfits after old PAK/ZOBJ/sync entries so existing numeric selections retain
identity. Existing native struct self-pointers are rebound after vector growth.

SoH uses its established skeleton/face/equipment draw boundaries. Canonical
first-person and pose aliases resolve only when the active private pack
supplies them; missing aliases keep the ordinary equipment path. MM reuses its
existing private-rig `AdultLink` renderer, including hand/first-person/blink
callbacks and `Player_DrawImpl`, for native adult and future child outfit roots.
No bone translations, hierarchy, animation data or model weights are changed.
Optional weighted companions bind to that actual selected private skeleton.

Sticky mounts are retained. Unlike the old shared aliases, private body and
material addresses can coexist through spring -> winter -> spring. FrameBegin
cleanup is gated by PlayState ownership and `state.frames`, so repeated local,
reflection or cutscene draws cannot rotate/free deferred temporary-DL pools
multiple times in one graphics frame. Selection can still refresh in that frame.

## Evidence and limits

- Both production selectors compile/run under ASan/UBSan: full season cycle,
  generic families, adult/child, missing pack, Off, forced/remote priority,
  private mount retention, and same-frame draw gating. LSan is unavailable in
  this ptraced environment, so the execution uses `ASAN_OPTIONS=detect_leaks=0`.
- Full native-header syntax checks passed for both PAK TUs, the MM adult
  renderer, both host menus, and the MM Wolf form ownership boundary.
- Existing 432-frame native MM Flex/matrix fixture passed. The extended 74-frame MM
  adult/first-person fixture passes original and Combo ABIs, including a
  generic private winter set, private bow/forearm aliases and custom-form
  precedence.
- Run `ASAN_OPTIONS=detect_leaks=0 python tests/seasonal_outfits/run_tests.py`,
  `python tests/adult_mm_flex/run_tests.py`, and
  `ASAN_OPTIONS=detect_leaks=0 python tests/okayu_presentation/test_mm_adult_renderer.py --sanitize`.
- The converted Din archive validation is supplied by its asset workstream;
  accepted meshes, rig and animations must remain unchanged. Summer/young
  slots without an accepted pack use the normal model.

This is implemented/component-verified source capability, not a full executable
build or gameplay proof. Required runtime review remains a complete cycle,
Off/native-fit restoration, both games/ages, scene re-entry, Alt settings,
equipment/first-person views, forced forms, blink and weighted/rigid fallback.
