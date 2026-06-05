set(GEOMETER_LIB_SOURCES
    c_api.cpp
    clipper2_bytes.cpp
    hlr_projection.cpp
    model_bounds.cpp
    model_bounds_options_json.cpp
    planar_contours.cpp
    planar_solve.cpp
    planar_step.cpp
    planar_triangulate.cpp
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

add_library(geometer_lib STATIC ${GEOMETER_LIB_SOURCES})
set_target_properties(geometer_lib PROPERTIES OUTPUT_NAME geometer)

if(NOT DEFINED GEOMETER_ABI_VERSION)
    set(GEOMETER_ABI_VERSION "20260604" CACHE STRING "Geometer C ABI generation date")
endif()
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/geometer/version_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/geometer/version_config.h"
    @ONLY
)

# OCCT 7.8 target names.
# TKV3d/TKService are pulled in because TKXCAF references Graphic3d material
# types for PBR mapping, even though we do no rendering.
set(GEOMETER_OCCT_LIBS
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
    TKPrim
    TKBRep
    TKGeomBase
    TKG3d
    TKG2d
    TKMath
    TKernel
)

set(GEOMETER_OCCT_LINK_LIBS ${GEOMETER_OCCT_LIBS})
if(UNIX AND NOT EMSCRIPTEN AND GEOMETER_OCCT_LIBRARY_TYPE STREQUAL "Static" AND OpenCASCADE_LIBRARIES)
    set(GEOMETER_OCCT_LINK_LIBS ${OpenCASCADE_LIBRARIES})
endif()

if(UNIX AND NOT EMSCRIPTEN)
    find_package(Threads REQUIRED)
endif()

function(configure_geometer_native_target target_name)
    target_include_directories(${target_name} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/generated
        ${OpenCASCADE_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/third_party/rapidjson/include
        ${CMAKE_SOURCE_DIR}/third_party/clipper2/include
    )

    if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN AND GEOMETER_OCCT_LIBRARY_TYPE STREQUAL "Static")
        string(REPLACE ";" "," _geometer_occt_link_group "${GEOMETER_OCCT_LINK_LIBS}")
        target_link_libraries(${target_name} PUBLIC "$<LINK_GROUP:RESCAN,${_geometer_occt_link_group}>")
    else()
        target_link_libraries(${target_name} PUBLIC ${GEOMETER_OCCT_LINK_LIBS})
    endif()

    if(UNIX AND NOT EMSCRIPTEN)
        target_link_libraries(${target_name} PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
    endif()

    if(WIN32)
        target_link_libraries(${target_name} PUBLIC ws2_32)
    endif()

    if(MSVC)
        target_compile_options(${target_name} PUBLIC /EHsc)
    endif()
endfunction()

configure_geometer_native_target(geometer_lib)

if(NOT EMSCRIPTEN)
    add_library(geometer_shared SHARED ${GEOMETER_LIB_SOURCES})
    configure_geometer_native_target(geometer_shared)
    target_compile_definitions(geometer_shared PRIVATE GEOMETER_BUILD_SHARED)
    set_target_properties(geometer_shared PROPERTIES
        OUTPUT_NAME geometer
        ARCHIVE_OUTPUT_NAME geometer_dll_import
    )
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
        "-sEXPORTED_FUNCTIONS=['_malloc','_free','_geometer_version_string','_geometer_version_major','_geometer_version_minor','_geometer_version_patch','_geometer_abi_version','_geometer_step_hlr_projection_json','_geometer_step_hlr_projection_json_bytes','_geometer_step_to_glb','_geometer_step_to_glb_bytes','_geometer_planar_batch_solve','_geometer_planar_batch_solve_bytes','_geometer_planar_triangulate','_geometer_planar_triangulate_bytes','_geometer_clipper2_boolean','_geometer_clipper2_boolean_bytes','_geometer_clipper2_inflate_open','_geometer_clipper2_inflate_open_bytes','_geometer_free_string','_geometer_free_bytes']"
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','stringToUTF8','lengthBytesUTF8','getValue']"
    )
    set_target_properties(geometer_browser PROPERTIES
        OUTPUT_NAME "geometer"
        SUFFIX ".js"
    )

    add_custom_command(TARGET geometer_browser POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GEOMETER_WASM_BROWSER_DIST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:geometer_browser>"
            "${GEOMETER_WASM_BROWSER_DIST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE_DIR:geometer_browser>/geometer.wasm"
            "${GEOMETER_WASM_BROWSER_DIST_DIR}/"
        COMMENT "Copying browser WASM outputs to dist/wasm/browser/"
    )

    add_executable(geometer_planar_browser
        browser_exports.cpp
        c_api_planar.cpp
        clipper2_bytes.cpp
        planar_solve.cpp
        planar_triangulate.cpp
        version.cpp
        ${CMAKE_SOURCE_DIR}/third_party/clipper2/src/clipper.engine.cpp
        ${CMAKE_SOURCE_DIR}/third_party/clipper2/src/clipper.offset.cpp
        ${CMAKE_SOURCE_DIR}/third_party/clipper2/src/clipper.rectclip.cpp
        ${CMAKE_SOURCE_DIR}/third_party/clipper2/src/clipper.triangulation.cpp
    )
    target_include_directories(geometer_planar_browser PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/generated
        ${CMAKE_SOURCE_DIR}/third_party/clipper2/include
    )
    target_link_options(geometer_planar_browser PRIVATE
        --no-entry
        -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_MEMORY=16777216
        -sSTACK_SIZE=4194304
        -sENVIRONMENT=web,worker
        -sMODULARIZE=1
        -sEXPORT_NAME=createGeometerPlanarModule
        -sNO_EXIT_RUNTIME=1
        "-sEXPORTED_FUNCTIONS=['_malloc','_free','_geometer_version_string','_geometer_version_major','_geometer_version_minor','_geometer_version_patch','_geometer_abi_version','_geometer_planar_batch_solve','_geometer_planar_batch_solve_bytes','_geometer_planar_triangulate','_geometer_planar_triangulate_bytes','_geometer_clipper2_boolean','_geometer_clipper2_boolean_bytes','_geometer_clipper2_inflate_open','_geometer_clipper2_inflate_open_bytes','_geometer_free_string','_geometer_free_bytes']"
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','stringToUTF8','lengthBytesUTF8','getValue']"
    )
    set_target_properties(geometer_planar_browser PROPERTIES
        OUTPUT_NAME "geometer-planar-browser"
        SUFFIX ".js"
    )

    add_custom_command(TARGET geometer_planar_browser POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GEOMETER_WASM_PLANAR_BROWSER_DIST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:geometer_planar_browser>"
            "${GEOMETER_WASM_PLANAR_BROWSER_DIST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE_DIR:geometer_planar_browser>/geometer-planar-browser.wasm"
            "${GEOMETER_WASM_PLANAR_BROWSER_DIST_DIR}/"
        COMMENT "Copying planar browser WASM outputs to dist/wasm/planar-browser/"
    )
endif()

if(NOT EMSCRIPTEN)
    # Copy native static library to the platform-aware native dist folder.
    add_custom_command(TARGET geometer_lib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GEOMETER_NATIVE_DIST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:geometer_lib>"
            "${GEOMETER_NATIVE_DIST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:geometer_lib>"
            "${GEOMETER_NATIVE_DIST_DIR}/libgeometer.a"
        COMMENT "Copying geometer_lib to dist/native/${GEOMETER_NATIVE_DIST_PLATFORM}/"
    )
endif()
