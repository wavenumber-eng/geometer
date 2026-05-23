set(GEOMETER_LIB_SOURCES
    c_api.cpp
    clipper2_bytes.cpp
    hlr_projection.cpp
    planar_contours.cpp
    planar_solve.cpp
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
    set(GEOMETER_ABI_VERSION "20260523" CACHE STRING "Geometer C ABI generation date")
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
    TKBRep
    TKGeomBase
    TKG3d
    TKG2d
    TKMath
    TKernel
)

function(configure_geometer_native_target target_name)
    target_include_directories(${target_name} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/generated
        ${OpenCASCADE_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/third_party/clipper2/include
    )

    target_link_libraries(${target_name} PUBLIC ${GEOMETER_OCCT_LIBS})

    if(WIN32)
        target_link_libraries(${target_name} PUBLIC ws2_32)
    endif()

    if(MSVC)
        target_compile_options(${target_name} PUBLIC /EHsc)
    endif()
endfunction()

configure_geometer_native_target(geometer_lib)

if(NOT EMSCRIPTEN)
    option(GEOMETER_COPY_OCCT_RUNTIME "Copy OCCT runtime DLLs next to geometer_shared" ON)

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
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/dist"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:geometer_browser>"
            "${CMAKE_SOURCE_DIR}/dist/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE_DIR:geometer_browser>/geometer.wasm"
            "${CMAKE_SOURCE_DIR}/dist/"
        COMMENT "Copying browser WASM outputs to dist/"
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
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/dist"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:geometer_planar_browser>"
            "${CMAKE_SOURCE_DIR}/dist/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE_DIR:geometer_planar_browser>/geometer-planar-browser.wasm"
            "${CMAKE_SOURCE_DIR}/dist/"
        COMMENT "Copying planar browser WASM outputs to dist/"
    )
endif()

if(NOT EMSCRIPTEN)
    if(WIN32 AND GEOMETER_COPY_OCCT_RUNTIME AND OpenCASCADE_BINARY_DIR)
        file(GLOB GEOMETER_OCCT_RUNTIME_DLLS "${OpenCASCADE_BINARY_DIR}/*.dll")
    endif()

    # Copy native static library to dist/ after build.
    add_custom_command(TARGET geometer_lib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/dist"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:geometer_lib>"
            "${CMAKE_SOURCE_DIR}/dist/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:geometer_lib>"
            "${CMAKE_SOURCE_DIR}/dist/libgeometer.a"
        COMMENT "Copying geometer_lib to dist/"
    )

    # Copy native shared library to dist/ for Python ctypes loading.
    add_custom_command(TARGET geometer_shared POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/dist"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:geometer_shared>"
            "${CMAKE_SOURCE_DIR}/dist/"
        COMMENT "Copying geometer_shared to dist/"
    )

    if(GEOMETER_OCCT_RUNTIME_DLLS)
        add_custom_command(TARGET geometer_shared POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${GEOMETER_OCCT_RUNTIME_DLLS}
                "${CMAKE_SOURCE_DIR}/dist/"
            COMMENT "Copying OCCT runtime DLLs to dist/"
        )
    endif()
endif()
