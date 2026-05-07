add_library(geometer_lib STATIC
    c_api.cpp
    hlr_projection.cpp
    planar_contours.cpp
    planar_solve.cpp
    projection.cpp
    projection_options_json.cpp
    step_to_glb.cpp
    step_to_glb_options_json.cpp
    version.cpp
    ${CMAKE_SOURCE_DIR}/third_party/clipper2/src/clipper.engine.cpp
    ${CMAKE_SOURCE_DIR}/third_party/clipper2/src/clipper.offset.cpp
    ${CMAKE_SOURCE_DIR}/third_party/clipper2/src/clipper.rectclip.cpp
    ${CMAKE_SOURCE_DIR}/third_party/clipper2/src/clipper.triangulation.cpp
)
set_target_properties(geometer_lib PROPERTIES OUTPUT_NAME geometer)

set(GEOMETER_ABI_VERSION 3 CACHE STRING "Geometer C ABI version" FORCE)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/geometer/version_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/geometer/version_config.h"
    @ONLY
)

target_include_directories(geometer_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/generated
    ${OpenCASCADE_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party/clipper2/include
)

# OCCT 7.8 target names.
# TKV3d/TKService are pulled in because TKXCAF references Graphic3d material
# types for PBR mapping, even though we do no rendering.
target_link_libraries(geometer_lib PUBLIC
    TKDESTEP
    TKDEGLTF
    TKRWMesh
    TKMesh
    TKXCAF
    TKVCAF
    TKCAF
    TKCDF
    TKLCAF
    TKV3d
    TKService
    TKDE
    TKXSBase
    TKShHealing
    TKGeomAlgo
    TKTopAlgo
    TKHLR
    TKBO
    TKBRep
    TKGeomBase
    TKG3d
    TKG2d
    TKMath
    TKernel
)

if(WIN32)
    target_link_libraries(geometer_lib PUBLIC ws2_32)
endif()

if(EMSCRIPTEN)
    add_executable(geometer_browser browser_exports.cpp)
    target_link_libraries(geometer_browser PRIVATE geometer_lib)
    target_link_options(geometer_browser PRIVATE
        --no-entry
        -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_MEMORY=67108864
        -sSTACK_SIZE=8388608
        -sENVIRONMENT=web,worker
        -sMODULARIZE=1
        -sEXPORT_NAME=createGeometerModule
        -sNO_EXIT_RUNTIME=1
        "-sEXPORTED_FUNCTIONS=['_malloc','_free','_geometer_version_string','_geometer_version_major','_geometer_version_minor','_geometer_version_patch','_geometer_abi_version','_geometer_step_hlr_projection_json','_geometer_step_hlr_projection_json_bytes','_geometer_step_to_glb','_geometer_step_to_glb_bytes','_geometer_planar_batch_solve','_geometer_planar_batch_solve_bytes','_geometer_free_string','_geometer_free_bytes']"
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','stringToUTF8','lengthBytesUTF8','getValue']"
    )
    set_target_properties(geometer_browser PROPERTIES
        OUTPUT_NAME "geometer-browser"
        SUFFIX ".js"
    )

    add_custom_command(TARGET geometer_browser POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/dist"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:geometer_browser>"
            "${CMAKE_SOURCE_DIR}/dist/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE_DIR:geometer_browser>/geometer-browser.wasm"
            "${CMAKE_SOURCE_DIR}/dist/"
        COMMENT "Copying browser WASM outputs to dist/"
    )
endif()

if(NOT EMSCRIPTEN)
    # Copy native static library to dist/ after build.
    add_custom_command(TARGET geometer_lib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/dist"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:geometer_lib>"
            "${CMAKE_SOURCE_DIR}/dist/"
        COMMENT "Copying geometer_lib to dist/"
    )
endif()
