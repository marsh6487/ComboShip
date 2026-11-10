# Midna native syntax fixture include closure

Actual public source 3eb1649e (local c9fdb3a7, tree900fd9e2) systems job129311424145 failed the registered `Contents-based chests and opt-in Midna` row. Its real native OoT HUD translation unit now includes `combo/rando/SongEventsBridge.h`; the existing Midna audio syntax command omitted the repository root include path. Direct reproduction failed at the identical missing header.

The sole runner change appends the real repository root to the existing include search list, preserving native header precedence, assertions, source bodies and compiler diagnostics. No production source or warning policy changes.

The exact five-command manifest row was rerun through scripts/ci/run_regressions.py with the established GCC14/native dependency environment. All commands passed in13.25 seconds, including complete OoT fairy/HUD and MM fairy/HUD syntax checks, native chest behavior, draw/routing controls, PCM/resource discovery, fallback and manager isolation. The full systems CI job itself remains failed at the prior checkpoint; this repair does not claim later rows passing or pristine native runtime/gameplay proof.
