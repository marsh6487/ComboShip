#!/usr/bin/env bash
# Production entry/decoder/render regressions run before full platform builds.
set -euo pipefail
cd "$(dirname "$0")/../.."
export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}"

python3 -B tests/custom_messages/run_decoder_tests.py
python3 -B scripts/diagnostics/run_forest_gi_particles_tests.py
python3 -B tests/black_market/run_entry_access_tests.py
python3 -B tests/black_market/run_resource_entry_tests.py --bundled-only
python3 -B tests/native_market/run_legacy_cache_tests.py
python3 -B scripts/diagnostics/run_shield_particle_lifetime_tests.py
python3 -B scripts/diagnostics/run_saria_palette_tests.py
python3 -B -m unittest discover -s tests/native_forest -p 'test_*.py'
