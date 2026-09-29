#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
python3 -B scripts/diagnostics/run_hint_item_name_tests.py
python3 -B scripts/diagnostics/run_nei_gi_tests.py --held --combo
python3 -B scripts/diagnostics/run_mm_item_presentation_tests.py
python3 -B tests/nei_held/run_hand_fit_tests.py
python3 -B tests/nei_held/run_articulated_tests.py
python3 -B tests/nei_used_fx/run_tests.py
python3 -B tests/nei_leaf/run_tests.py
python3 -B tests/nei_whip/run_tests.py
python3 -B tests/nei_lantern_grip/run_tests.py
python3 -B scripts/diagnostics/run_switch_hook_instant_tests.py
python3 -B scripts/diagnostics/run_time_gate_visibility_tests.py
python3 -B tests/nei_item_stow/run_ballchain_tests.py
python3 -B tests/nei_item_stow/run_lantern_rod_tests.py
python3 -B tests/nei_item_stow/run_stow_tests.py
python3 -B tools/nei_held/verify_assets.py
