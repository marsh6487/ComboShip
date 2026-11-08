# Shared UI visibility milestone, 2026-11-07

Parent: `d13069d22d03e3aae7ab488b5a1f4e07532a8af5` (preceding cumulative source).
This bounded milestone contains the shared exported True Hidden UI runtime bit,
passive GUI filtering, settings/modal/framebuffer recovery, notification aging,
and the What’s New inventory. Native checkbox/input/draw wiring and the MM
Minimal UI port are still in progress and excluded from this milestone.

The saved checkbox defaults off. L changes a single nonpersistent atomic bit
in libultraship shared by the two native hosts and GUI. It does not change
minimap, Minimal UI, tracker/window intent or save data. Registered GUI updates
continue while their passive rendering is hidden. Application menus, explicit
settings recovery, modal/file-picker requests and the legacy guest framebuffer
remain available. Hidden notifications continue aging without replay on reveal.

Verified: actual compiled shared-module API with independent C/C++ callers;
ASan/UBSan headless ImGui routing and notification lifetime fixtures; complete
Gui/GameOverlay translation-unit compilation at the fixture SDL dependency
boundary; What’s New native/ImGui suite and all 25 CI policy checks. These are
source/CPU tests, not a complete executable or configured game demonstration.

The checkpoint is separately committed, bundled and independently restored
against public baseline `2054eef2f12aeedfb2d2d0f5519cc23272227e4e`; the recovery
manifest records exact refs/tree/file hashes. The in-progress native hosts and
remaining GUI lifecycle adapters are retained in the candidate worktree for
the following milestone. No publication or master promotion.
