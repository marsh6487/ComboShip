# Probe-only hook for Kenix3/libultraship#1236 (Jameriquiah e3cd6591).
# ComboShip includes this file optionally before add_subdirectory(libultraship),
# so the historical patch is applied to the vendored 7cb10226e LUS baseline
# before any LUS targets are configured or compiled.

find_program(_COMBO_SCROLL_GIT git REQUIRED)
set(_COMBO_SCROLL_PATCH
    "${CMAKE_CURRENT_SOURCE_DIR}/patches/libultraship-custom-tex-scroll-e3cd6591.patch")

execute_process(
    COMMAND "${_COMBO_SCROLL_GIT}" apply --check --directory=libultraship "${_COMBO_SCROLL_PATCH}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE _COMBO_SCROLL_CHECK_RESULT
    OUTPUT_QUIET
    ERROR_VARIABLE _COMBO_SCROLL_CHECK_ERROR)

if(_COMBO_SCROLL_CHECK_RESULT EQUAL 0)
    execute_process(
        COMMAND "${_COMBO_SCROLL_GIT}" apply --directory=libultraship "${_COMBO_SCROLL_PATCH}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE _COMBO_SCROLL_APPLY_RESULT
        OUTPUT_QUIET
        ERROR_VARIABLE _COMBO_SCROLL_APPLY_ERROR)

    if(NOT _COMBO_SCROLL_APPLY_RESULT EQUAL 0)
        message(FATAL_ERROR
            "Failed to apply libultraship custom texture scrolling probe (e3cd6591):\n"
            "${_COMBO_SCROLL_APPLY_ERROR}")
    endif()

    message(STATUS "Applied libultraship custom texture scrolling probe e3cd6591")
else()
    # A second configure of the same checkout sees an already-patched worktree.
    # Accept that state only when every feature marker is present; otherwise fail
    # rather than silently building against a mismatched LUS revision.
    set(_COMBO_SCROLL_ALREADY_APPLIED TRUE)
    set(_COMBO_SCROLL_MARKERS
        "libultraship/include/fast/lus_gbi.h|OTR_G_SCROLL_TEXTURE"
        "libultraship/include/libultraship/libultra/gbi.h|gsSPScrollTexture"
        "libultraship/src/fast/interpreter.cpp|gfx_scroll_texture_handler_custom"
        "libultraship/src/fast/resource/factory/DisplayListFactory.cpp|ScrollTexture")

    foreach(_COMBO_SCROLL_MARKER IN LISTS _COMBO_SCROLL_MARKERS)
        string(REPLACE "|" ";" _COMBO_SCROLL_PAIR "${_COMBO_SCROLL_MARKER}")
        list(GET _COMBO_SCROLL_PAIR 0 _COMBO_SCROLL_FILE)
        list(GET _COMBO_SCROLL_PAIR 1 _COMBO_SCROLL_TOKEN)
        file(READ "${CMAKE_CURRENT_SOURCE_DIR}/${_COMBO_SCROLL_FILE}" _COMBO_SCROLL_FILE_TEXT)
        string(FIND "${_COMBO_SCROLL_FILE_TEXT}" "${_COMBO_SCROLL_TOKEN}" _COMBO_SCROLL_TOKEN_INDEX)
        if(_COMBO_SCROLL_TOKEN_INDEX EQUAL -1)
            set(_COMBO_SCROLL_ALREADY_APPLIED FALSE)
        endif()
    endforeach()

    if(_COMBO_SCROLL_ALREADY_APPLIED)
        message(STATUS "libultraship custom texture scrolling probe e3cd6591 already applied")
    else()
        message(FATAL_ERROR
            "libultraship custom texture scrolling probe e3cd6591 does not apply cleanly, "
            "and the expected feature markers are not all present. Refusing to patch an "
            "unexpected LUS tree. git apply --check reported:\n${_COMBO_SCROLL_CHECK_ERROR}")
    endif()
endif()

unset(_COMBO_SCROLL_FILE_TEXT)
unset(_COMBO_SCROLL_TOKEN_INDEX)
unset(_COMBO_SCROLL_PAIR)
unset(_COMBO_SCROLL_FILE)
unset(_COMBO_SCROLL_TOKEN)
unset(_COMBO_SCROLL_MARKER)
unset(_COMBO_SCROLL_MARKERS)
unset(_COMBO_SCROLL_ALREADY_APPLIED)
unset(_COMBO_SCROLL_APPLY_ERROR)
unset(_COMBO_SCROLL_APPLY_RESULT)
unset(_COMBO_SCROLL_CHECK_ERROR)
unset(_COMBO_SCROLL_CHECK_RESULT)
unset(_COMBO_SCROLL_PATCH)
unset(_COMBO_SCROLL_GIT)
