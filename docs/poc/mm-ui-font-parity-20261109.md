# MM settings UI font parity

## Scope and baseline

cor requested a narrow checkout for PR #89 and clarified that the UI font,
not the in-game font, should match SoH.

- Candidate branch: `candidate/soh-ui-font-parity-20261109`.
- Local parent: `767f1541eff077c8d8929052ea71adf22ac52fa0`.
- Equivalent published PR #89 parent: `d9c1b39bc6eb5f911979bc397ae49141ff1f221f`.
- Both parents have tree `743aead685ac075206cede15ad41cc7d26e07c78`.
- All existing cumulative integrations remain in the parent. This change is
  limited to MM menu font selection and explanatory comments.

## Design and implementation

Both games already load byte-identical Montserrat Regular for standard UI text
at 16, 20 and 24 px. Both set the 20 px standard face as ImGui's default. MM's
settings paths instead explicitly selected the 20 px Inconsolata monospace face.

Use MM's existing `fontStandardLarger` at all three settings content entrypoints:

1. `Ship::Menu::DrawElement` in `mm/2s2h/BenGui/Menu.cpp`: standalone/solo MM
   settings, search results and popout content.
2. `Ship::Menu::DrawContent` in that file: the retained shared content entrypoint,
   kept consistent if called. The current combined menu delegates individual widgets.
3. `BenGui::BenMenu::DrawWidgetByIndex` in `mm/2s2h/BenGui/BenMenu.cpp`: individual
   MM widgets delegated by combined settings and search.

Update the shared draw helper's comments to describe its optional content font.
Keep the existing font stack, null guards, 24 px headings, menu scaling, icon
merging, resource loading and standard/mono font-pack override paths. Selecting
the local standard pointer also honors an explicitly installed MM standard UI
font pack; removing the push would inherit whichever font the caller selected.
Monospace developer views and all in-game text remain outside this change.

No font files, archive changes, new options or configuration migration are needed.
The shared Montserrat file SHA-256 is
`c3fb0280e4339f6c70cea42b8b432c6fb17fde130fbb12e9209c7cdfa79d976f`.

## Verification record

- PASS: both games' bundled Montserrat files have the identical hash above.
- PASS: diff inspection confirms exactly three production font selections change;
  the shared draw helper has comment-only changes.
- PASS: existing `scripts/diagnostics/run_mm_mod_font_tests.py` checks all four
  standard/mono override configurations and post-archive initialization order.
  Font loading code is unchanged from the verified parent.
- PASS: `git diff --check` and pinned clang-format 14.0.6 validation of all three
  changed C++ source/header files.
- PASS: GCC 13 `-fsyntax-only` checks for both affected MM translation units with
  the production `COMBO_BUILD` defines/options/sysroot and fresh textual PCH.
  Include traces confirm the candidate source and shared menu header were used.
  Existing warnings outside the changed selections remain; neither check reported
  an error. This is native compilation checking, not a complete link.
- PASS: independent review of menu coverage, dormant MM fallback, font-stack
  balance, size/scaling, icons and font-pack behavior found no blocking issue.
  Its documentation correction distinguishing the retained content helper from
  active combined-menu widget delegation is incorporated above.
- Unproven: full application link and configured visual acceptance for this patch.

The decisive visual check is one MM settings page with long labels, search, and
Font Awesome controls in solo MM and combined mode, including a delegated MM
widget, before/after an OoT-to-MM transition and at 150% menu scale. Body text
should use Montserrat at the existing scale without clipping or font-stack errors.
Publishing to the draft candidate does not promote or merge the accepted master.
