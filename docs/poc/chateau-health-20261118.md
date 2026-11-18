# Chateau Romani health check — 2026-11-18

Chateau Romani's GI resources, native grant IDs, shared bottle projection, and drinking actions passed the source-level checks below. One native magic admission defect was reproduced and corrected in both hosts.

## Corrected native magic admission

An acquired fractional magic meter could reject a native magic request larger than its current contents even while Chateau Romani's infinite-magic effect was active. OoT checked the current amount before its infinite policy; MM's early check recognized permanent infinite magic but omitted the native Chateau week-event flag. MM's lens, Goron/Zora and Giant's Mask admission checks had the same omission at zero current magic.

Both native consume APIs now resolve their existing infinite policy after the acquired-meter guard and normalize consumption to zero before checking the amount. MM's relevant continuous-use admission checks use that same policy. OoT magic addition and both games' refill paths are unchanged. The change neither acquires a magic meter nor adds permanent magic flags, and existing action-state rejection remains in place.

The regression fixture failed against each original native function with Chateau active, an acquired 12-unit meter, and a 24-unit request. The corrected native functions pass for Chateau and permanent infinite magic, zero and 12 current magic, every native consumption request type, ordinary/expired insufficient magic, missing magic ownership, busy states, and ordinary consumption. OoT's magic-add request retains its original amount.

## Grant, bottle and drink paths

- OoT grants native Chateau bottle content `0xB6`; MM grants native content `0x25`. Both real bottle cores store the filled bottle and retain existing full-wheel handling.
- Shared bottle conversion maps `0xB6` and `0x25` in both directions. The native grant/shared-state fixture checks immediate projection, receipt merging, duplicate delivery, full inventory and echo prevention.
- OoT uses its blue-potion drinking action and activates the existing Chateau effect when Chateau is emptied. The actual native input-eligibility branch permits drinking with full health and magic. MM uses its distinct Chateau drinking action and native week-event flag.
- Both native drinking actions request health and magic refill. Fixtures exercise full and partial meters at capacities 0, 12, 48, 96 and 125. Custom-item magic helpers honor the resulting infinite policy without accepting an unowned meter. OoT buff deactivation preserves an already-owned permanent infinite-magic flag.
- The real OoT bottle-consumption hook is exercised on all three C buttons and four D-pad bindings; Chateau activates once and empties the binding once. Ordinary milk behavior is retained. Chateau's existing bottomless maximum remains three charges.

## Verification

| Check | Result |
| --- | --- |
| `python tests/chateau_health/run_tests.py` | PASS, both hosts; ASan, UBSan and float-cast-overflow |
| `python scripts/diagnostics/run_chateau_grant_tests.py --json-include /workspace/scratch/3931886f46f1/deps/include` | PASS, native grant and shared-state fixtures |
| `python scripts/diagnostics/run_combo_reward_delivery_tests.py --sanitize` | PASS, including actual bottle-consumption hook and C/D-pad bindings |
| `python scripts/diagnostics/run_chateau_3ds_draw_tests.py` | PASS, 16 resource-availability combinations and live Alt selection |
| `python tests/chateau_romani_3ds/run_bundling_tests.py` | PASS, native/imported asset bundling, idempotence and corruption checks |
| `python tests/chateau_romani_3ds/render_state_test.py` | PASS, 12 host/material copies through real Fast3D render-state rules |
| Full `z_parameter.c` syntax with native headers and Combo definitions | PASS, OoT and MM; existing warnings remain |

The native-function fixtures compile the production action bodies, enums, cost APIs, bottle cores and effect helpers, with animation, audio and engine service boundaries stubbed. They are not a linked full-game run. A release playtest should still obtain and drink Chateau in each game, cross between games with the filled bottle, and confirm the rendered GI, refill animation and effect expiration in the running build.
