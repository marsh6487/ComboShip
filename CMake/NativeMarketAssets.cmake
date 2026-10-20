# Geometry sources stay compressed in git. Both ordinary port-archive packers
# consume identical resources; no supplemental market archive is installed.
get_filename_component(COMBO_NATIVE_MARKET_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
add_custom_target(
    PrepareNativeMarketAssets
    COMMAND ${Python3_EXECUTABLE} -B "${COMBO_NATIVE_MARKET_ROOT}/scripts/assets/prepare_native_market.py"
    WORKING_DIRECTORY "${COMBO_NATIVE_MARKET_ROOT}"
    DEPENDS "${COMBO_NATIVE_MARKET_ROOT}/scripts/assets/prepare_native_market.py"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/native_market.py"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/native_market/bundled/resources.zlib.b64"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/native_market/bundled/manifest.json"
    COMMENT "Preparing native Black Market geometry..."
    VERBATIM)
