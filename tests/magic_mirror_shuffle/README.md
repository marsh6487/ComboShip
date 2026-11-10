Magic Mirror shuffle POC3

The native `Shuffle Magic Mirror` option defaults OFF. OFF uses the existing fixed stump reward. ON adds one Mirror and one shuffled stump check; the assigned reward, native receipt, ownership grant and collection state are handled by the receiving engine. Mirror ownership never removes an uncollected shuffled stump. Plentiful does not duplicate the Mirror.

In solo OoT, the check follows the native Lost Woods region and its room-1 fairy tunnel. In solo MM, it follows the native Woods of Mystery passage. The combined pool owns one physical MM check. Its two genuine routes are native MM Woods of Mystery (which returns through OoT Lost Woods) and the OoT Lost Woods fairy tunnel. The direct OoT oracle reports an access-only probe after its actual inventory-specific native region search. The shared fill, playthrough, requiredness and cross-dungeon fixed point union that probe into the same MM check, including before the main interworld portal opens. The probe never becomes a pool location or a reward, and reaching it does not open MM's main overworld.

In Combo, the existing controls in both game tabs edit one authoritative `gCombo.Rando.ShuffleMagicMirror` preference. Either control can turn it ON or OFF, independently of the retained solo preferences. Native MM cards, flat OoT widgets, presets, bulk controls, config import and live pool previews use the same value. The launcher invokes `SOH_NormalizeComboGraceFromMM` before native dumps to freeze that preference into `gRando.Generation.ShuffleMagicMirror`; both native seed policies explicitly apply zero as well as one. Replay normalizes both native snapshots before restore, while temporary save creation preserves the user's newer UI preference. Only old configurations or snapshots without a canonical value migrate by OR-ing the old native settings. Solo modes retain their native controls, generation and settings. Old MM check arrays load with the appended check uncollected.

`CW_DRAW_KIND_MAGIC_MIRROR = 42` describes the procedural Mirror through the portable item ABI. Both foreign renderers call the receiving host's authored Mirror draw function, so they retain its mesh/reflection/leaves and native scale without copying donor display-list pointers or regenerating art.

Run the focused checks with project nlohmann and spdlog include directories in `CPATH`:

- `python -B tests/magic_mirror_shuffle/registry_test.py`
- `python -B tests/magic_mirror_shuffle/run_shared_settings_tests.py --sanitize`
- `python -B tests/magic_mirror_shuffle/run_option_tests.py`
- `python -B tests/magic_mirror_shuffle/run_grant_tests.py`
- `python -B tests/magic_mirror_shuffle/run_pool_tests.py`
- `python -B tests/magic_mirror_shuffle/run_save_tests.py`
- `python -B tests/magic_mirror_shuffle/run_foreign_tests.py`
- `python -B tests/magic_mirror_shuffle/run_cancel_tests.py`
- `python -B tests/magic_mirror_shuffle/run_route_tests.py`

The route fixture executes the production active combined fill, final playthrough, requiredness replay and cross-dungeon fixed point over 32 deterministic small fixtures. Native oracle graph responses are boundary inputs. Native grant/cancel/save tests use actual game/save/event structures; cancellation also runs under ASan/UBSan. The MM pool fixture executes complete native `GeneratePools`. The option fixture uses the verbatim active normalization/snapshot/restore/save-policy bodies with config and option-enumeration boundaries. These checks and translation-unit syntax verification do not constitute a full game executable build or configured gameplay runtime acceptance.
