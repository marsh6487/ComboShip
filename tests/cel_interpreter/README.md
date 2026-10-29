# Cel interpreter integration fixture

Run from the repository root:

```sh
python -B tests/cel_interpreter/run_tests.py --sanitize
```

The runner extracts the actual four custom command handlers, their actual
command registrations, `ShaderIdMask`/`ShaderIdUnmask`, `ShaderOpts`,
`CCFeatures`, and `gfx_cc_get_features` from the current engine. The native
fixture compiles those bodies directly and emits packets through the actual
GBI producer macros. No handler command behavior is reproduced in the fixture.

The minimal interpreter supplies observable geometry submission, shadow build,
shadow render, and backend stencil boundaries. Event snapshots prove that old
geometry is flushed before a key changes, toon boundaries flush geometry then
the old shadow before disarming/resetting its key, and stencil changes occur
after the pending geometry flush. Shadow checks include the saved object key,
signed feet clamp, no-clamp marker, disarm, and flush-sentinel delegation. A
different singleton interpreter remains untouched by all four explicit-instance
handlers.

All 65,536 signed 16-bit Prism shader IDs are packed and decoded through the
production functions. Their payload stays in bits 17–32, bit 33 remains free,
and toon features activate only with the explicit toon option. The fixture also
checks every declared OTR/RDP opcode for collisions and verifies real producer
packets agree with interpreter constants for float fog `0x41` and cel commands
`0x4c`–`0x4f`.

`--engine /path/to/libultraship` selects another engine source/include tree
without changing this checkout. This also supports testing temporary source
mutations: deleting the production key handler's `Flush()` must make the native
fixture fail before accepting the new key.

The build uses C++20, the shared float/s32 GBI flags, `-Wall -Wextra -Werror`, and
optional AddressSanitizer/UndefinedBehaviorSanitizer. The existing combiner
decoder's intentional shift/add precedence is exempted from the parentheses
warning. LeakSanitizer scanning is disabled for the managed runner. These checks
cover transport, ownership and state boundaries; they do not rasterize shadows,
compile GPU shaders, or replace a full renderer build or configured game run.
