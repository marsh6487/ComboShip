#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
python3 -B scripts/diagnostics/run_hint_item_name_tests.py
python3 -B scripts/diagnostics/run_nei_gi_tests.py --held --combo
python3 -B scripts/diagnostics/run_mm_item_presentation_tests.py
python3 -B scripts/diagnostics/run_mm_item_receipt_tests.py
python3 -B scripts/diagnostics/run_song_gi_tests.py
python3 -B scripts/diagnostics/run_fairy_bottle_tests.py
python3 -B tests/seasons/run_tests.py
python3 -B tests/seasons/run_rod_lifecycle_tests.py
python3 -B tests/seasons/run_oot_shop_tests.py
python3 -B scripts/diagnostics/run_mm_mask_bridge_tests.py --sanitize
python3 -B tests/mm_presentation/run_foreign_scale_tests.py
python3 -B scripts/diagnostics/run_mm_nei_tests.py
python3 -B tests/nei_held/run_hand_fit_tests.py
python3 -B tests/nei_held/run_articulated_tests.py
python3 -B tests/nei_asset_priority/run_tests.py
python3 -B tests/nei_asset_priority/run_oot_gi_tests.py
python3 -B tests/nei_held/run_equipment_tests.py
python3 -B tests/nei_quest_held/run_tests.py
python3 -B tests/nei_used_fx/run_tests.py
python3 -B tests/nei_leaf/run_tests.py
python3 -B tests/nei_whip/run_tests.py
python3 -B tests/nei_lantern_grip/run_tests.py
python3 -B scripts/diagnostics/run_switch_hook_instant_tests.py
python3 -B scripts/diagnostics/run_time_gate_visibility_tests.py
python3 -B tests/mm_nei/run_timegate_audio_tests.py
python3 -B tests/nei_item_stow/run_ballchain_tests.py
python3 -B tests/nei_item_stow/run_lantern_rod_tests.py
python3 -B tests/nei_item_stow/run_stow_tests.py
python3 -B tools/nei_held/verify_assets.py
python3 -B tools/nei_icons/build.py --verify
python3 -B tools/nei_icons/verify_routes.py
