# Native companions/enemies must ship in each game's generated archive. Keep this
# merge before normalization and copy commands in GenerateSohOtr/Generate2ShipOtr.
get_filename_component(COMBO_PARTNER_RESOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(COMBO_PARTNER_RESOURCE_MERGER
    "${COMBO_PARTNER_RESOURCE_ROOT}/scripts/packaging/partner_resources.py")
