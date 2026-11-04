# Graveyard and soul repair plan

Baseline: 2e28b239cb4aa0f7f31da683cd15b1bcc8874ac1, matching the supplied crash build. User approved the collision direction and requested the Majora GI fixes. Accepted archives remain unchanged.

1. Reproduce the 39-surface Graveyard overflow and custom-polygon preservation failure with production patch code. Replace fixed storage with owned dynamic storage, restrict vanilla polygon patches to matching canonical data, and reset pinned resources before scene teardown. Guard scene exits using the actual loaded list length.
2. Trace Majora through name resolution, static/animated exports, and host rendering. Remove an incorrect stand-in only with a faithful native recipe; preserve Gyorg and other boss models. Test exported recipes and host state handling.
3. Correct only the HD soul icon's metadata in a separate deterministic candidate. Continue Alt flame resource tracing; make only evidence-supported changes, preserving artwork and native behavior.
4. Run focused production regressions and sanitizers, review changes, commit bounded source changes, and save a recovery artifact including patch, refs, recipes, verification and remaining runtime checks. Do not merge or promote masters; runtime remains unverified until tested in the user's configuration.
