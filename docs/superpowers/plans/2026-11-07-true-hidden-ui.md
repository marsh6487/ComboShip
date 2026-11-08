# True Hidden UI and 2Ship Minimal UI

Baseline: `d13069d22d03e3aae7ab488b5a1f4e07532a8af5`, preserved in the previous
independently restored cumulative checkpoint. Candidate:
`candidate/true-hidden-ui-both-games-20261107`.

User requirements: a True Hidden UI checkbox hiding every UI/HUD element,
What’s New accent, and both Minimal UI and True Hidden UI available in 2Ship.
L must toggle hidden/full presentation. The user explicitly chose to keep the
application settings menu available. Both checkboxes may remain enabled:
True Hidden UI takes priority; disabling it restores Minimal UI behavior.

Use `gEnhancements.TrueHiddenUI` (default off) in both ports, adjacent to
Minimal UI in the existing enhancement UI category. One nonpersistent runtime
hidden bit lives in the shared library, initially hidden when armed; L flips
it once per sampled press. It remains independent of the minimap register and
does not rewrite either checkbox. Existing L/debug conflict policies remain.
Native game-state input handling allows recovery while native dialogue/pause
or file selection is hidden; old minimap toggles do not also process L while
True Hidden UI is armed. Normal behavior is preserved with both options off.

Do not simply skip native UI draw functions: several advance text, timer,
clock, pause or pictograph state. Suppress submitted UI commands while retaining
those state transitions and tail allocations. Preserve scene rendering,
actors/equipment, camera transitions and world fades. Cover all native UI draw
routes, including menus/messages, title cards, targeting, timers and game-over
presentation, and passive ImGui overlays. Application menu/menubar recovery
remains available without clearing saved overlay visibility intent.

- [x] Trace native and ImGui boundaries and draw-driven state in both hosts.
- [x] Add shared runtime gate, toggle and passive GUI suppression with recovery.
- [x] Add OoT true-hidden rendering/input, preserving existing Minimal UI.
- [x] Port Minimal UI and true-hidden rendering/input to MM.
- [x] Verify state progression, one-toggle-per-press, restoration, precedence,
  settings access and both build guards with focused native tests and review.
- [x] Save an independently restored cumulative source candidate; update the
  requested combined publication authorization. No public upload or master merge.

Ownership: root owns shared forwarding header, accent registry, plans/records,
CI registration and recovery. OoT and MM agents own their native code, menu
checkboxes and host fixtures. A third agent owns the shared visibility bridge
and GUI rendering. Full builds and configured in-game appearance remain separate.
