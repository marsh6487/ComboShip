# True Hidden UI in both games, 2026-11-07

| Field | Record |
|---|---|
| Baseline | `d13069d22d03e3aae7ab488b5a1f4e07532a8af5`; all preceding cumulative fixes and catalogue additions retained. |
| Shared milestone | `e7c25caba23e14c39f034129e18e95ff2565e183`, independently restored and saved before completing the native ports. |
| Candidate | `candidate/true-hidden-ui-both-games-20261107`; final ref/tree and file hashes are in the recovery manifest. |
| Scope | True Hidden UI in OoT and MM, L visibility toggle, MM Minimal UI port, What’s New accents, and required draw/update separation. |
| Preservation | Game progression/input, dialogue/photo/clock state, world images and effects, fades, equipment/actors, minimap preferences, saved window intent and prior integrations. No scene/asset/save rewrite or master promotion. |
| Verdict | Implemented and source/native-fixture verified. Full executable builds, rendered configuration proof and user acceptance remain separate. |

True Hidden UI uses `gEnhancements.TrueHiddenUI`, off by default, adjacent to
Minimal UI in OoT's existing Graphics/UI section and in MM's new
Enhancements → UI sidebar. Its control receives the new-release accent;
Minimal UI receives the updated accent. Both checkboxes may stay enabled.

L flips a single atomic, nonpersistent hidden bit in libultraship, shared by
the native hosts and GUI. Native input handles each sampled press once before
the current game-state main function, including pause, dialogue and file
selection. Held L does not repeat the toggle; existing developer/cheat L conflict
rules remain. While armed, True Hidden UI overrides Minimal UI and bypasses
only the minimap's hidden register for normal presentation. It does not rewrite
that register or either checkbox. Turning it off restores the saved Minimal UI
and minimap behavior. The mode's last L state persists for the session and
across game/scene transitions; a new process starts the armed mode hidden.

The user explicitly chose application settings access while game UI is hidden.
The main application menu/menubar, recovery modals/file picker and explicit
configuration panels remain usable. Passive trackers, notifications, timers,
custom-form HUDs and other game overlays are hidden without clearing their
open/visibility preferences. The legacy guest-game framebuffer is retained.

Native UI drawing is not treated as pure rendering: text glyph/mode changes,
ocarina, clock, photo, pause, title and attention state continue. Scoped command
head restoration suppresses submitted UI commands while keeping tail-side
allocations and required GPU readback/capture operations. Boundaries retain
balanced Graph open/close markers, paused world snapshots, scene/pause/game-over
fades, dawn white transitions, physical end-title Triforce, lens and mask/warp
effects, and gameplay blindness. Coverage includes HUD, messages, pause and
notebook, shops, reticles/attention, title/dawn cards, autosave and diagnostic
overlays. Pure visual leaf functions use early gates only where their state
updates occur elsewhere.

MM Minimal UI follows OoT's actual policy: conditional damaged hearts and active
magic, fewer counters/button backgrounds, retained equipped-item icons/action
labels and contextual minigame feedback, and full HUD on pause. It is not the
all-UI suppression mode. With True Hidden UI armed and revealed, normal UI
takes precedence over this minimal presentation.

The GUI filter continues every registered Update. Nonvisual work formerly in
Draw moves to Update: combined tracker/settings reconciliation, timer overlay
suppression, notes flush/slot/debounce, health-meter notices, Harpoon GO flash
aging and both map-selection input/vote handlers. Existing connection/phase/
gamemode/input gates and entry semantics remain; rendering does not process
those operations a second time. Existing preexisting entry behavior was not
rewritten as an unrelated networking fix.

Verification:

- `tests/ui_visibility/run_tests.py` passes the real shared-module C/C++ API
  test, headless ImGui routing/notification lifetimes, four Combo lifecycle
  cases, OoT both-guard draw/input/Graph-marker/fade cases and HUD clocks, MM
  native interface/buttons/input/shops/registration/autosave/title/dawn/debug
  cases, MM health-meter lifetime and both Harpoon vote handlers. Native
  Interface_Draw and the late autosave callback are executed, not just policy
  predicates. Fixtures use ASan/UBSan where applicable.
- Native OoT changes compile into 36 real-header C objects; MM's runner checks
  23 complete native C translation units in both original and Combo guards
  (46 syntax checks). Negative controls reproduce the prior marker/fade and
  hidden-update/input/lifetime failures before their corrections.
- Existing season-shop fixtures (six sanitized cases), native market title
  fixtures in both hosts, routed Lost Woods title-card sanitizer tests and
  Magic Mirror integration tests pass with their assertions preserved. The
  new shared ABI/default-off bindings were added where extracted production
  bodies acquired that dependency.
- The What’s New native/real-ImGui suite and all 25 CI policy checks pass.
  Production/new-fixture formatting uses clang-format 14.0.6; inherited styling
  in the two preexisting title fixtures is retained. `git diff --check` passes.
- An independent read-only review completed the native texture/text/overlay
  route scan, checked preservation boundaries and reran the entire common UI
  suite successfully. No remaining confirmed source blocker was reported.

Limits: the shared GUI translation units compile at the existing fixture SDL
dependency boundary. Full host C++ checks were attempted, but actual SDL2,
SDL_net or libzip SDK headers are absent for several menu/custom-HUD units;
no fake SDK substitution or full-link claim is made. Full Windows/platform
builds and configured rendered behavior remain unverified. Rebuild matching
libultraship and both native game modules: the new shared API is required.

Configured acceptance should check L hidden/revealed and checkbox-off recovery
with both options on/off; dialogue/ocarina, pause/notebook, shop prompts,
autosave, dawn/title cards, targeting, pictograph capture, overlay timers,
notes, network voting and scene/cross-game transitions. Verify the world image,
fades and effects remain present, then compare the normal UI after restoration.
Publication of this expanded cumulative candidate awaits the combined
authorization requested by cor. The severe Black Market FPS regression remains
unresolved; UI suppression does not establish its cause or recovery.
