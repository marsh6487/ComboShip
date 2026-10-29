# Render Flight resource capture

The existing native capture checks run with:

```sh
python3 -B scripts/diagnostics/run_frame_timing_probe_tests.py
python3 -B scripts/diagnostics/run_performance_trace_tests.py
python3 -B scripts/diagnostics/test_analyze_render_flight.py
```

Renderer capture retains up to 4096 resource identities and 4096 texture
identities per detailed frame, including each table's reserved unknown bucket.
Framebuffer attribution retains 128 identities. Additional identities accumulate
into separate overflow rows with explicit overflow counts. Every retained table
row is emitted; output has no top-12 or top-64 truncation. Trace resource-stage
groups retain 4096 identities, actor groups 2048, and general aggregates 4096,
with their existing explicit overflow and identity-length limits.

Native cases retain 1000 distinct resource and texture identities, verify their
individual paths/counters, and parse all 1000 rows from the real asynchronous
dedicated capture file. Separate cases exceed the 4096 renderer collection
budgets and verify the overflow counts and costs. The logger test reads the
dedicated `OoT-RenderFlight-*.log`, since large tables can rotate the ordinary
game log. Existing files from earlier runs are preserved.

Capture remains bounded at approximately 256 MiB of serialized payload or
15 minutes, with an explicit capture-limit record. Framebuffer limits, capture
bounds, diagnostic defaults and the disabled path are unchanged. These checks
establish native collection/output behavior; configured in-game capture and
user acceptance remain separate.
