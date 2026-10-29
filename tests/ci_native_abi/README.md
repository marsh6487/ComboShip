# Native regression compiler ABI

The combined build uses `GBI_FLOAT_MTX` and `GBI_S32_VTX`. The regression runner
gives each task temporary compiler adapters for both named commands and `CC`/`CXX`,
so standalone fixtures inherit that same ABI. Original compiler commands and
flags, including compiler paths containing spaces, are resolved before changing
`PATH`; argument boundaries and `CPATH` remain intact, and temporary adapters are
removed when the invocation finishes.

Tasks that deliberately compile multiple ABIs set `"native_abi": "explicit"` in
the regression manifest. A nested script can also set
`COMBO_NATIVE_TEST_ABI=explicit` before selecting its own compiler flags.

Run `python3 -B tests/ci_native_abi/run_tests.py`. The tests execute the production
runner with controlled fake compilers and cover named/configured commands,
arguments containing spaces, caller flags, both opt-out mechanisms, NEI include
policy, success/failure cleanup, and list mode without compiler dependencies.
