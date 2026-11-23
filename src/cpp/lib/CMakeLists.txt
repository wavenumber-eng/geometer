set(GEOMETER_LIB_SOURCES
    c_api.cpp
    clipper2_bytes.cpp
    fast_hlr.cpp
    fast_hlr_occt.cpp
    fast_hlr_reconstruct.cpp
    fast_hlr_seams.cpp
    fast_mesh_shadow_outline.cpp
    hlr_projection.cpp
    indexed_mesh_packet.cpp
    mesh_shadow_outline.cpp
    mesh_hlr_projection.cpp
    mesh_illustration.cpp
    mesh_illustration_geometry.cpp
    mesh_illustration_paint.cpp
    mesh_illustration_hlr.cpp
    mesh_illustration_fusion.cpp
    mesh_illustration_fusion_geometry.cpp
    mesh_illustration_grid.cpp
    mesh_illustration_layers.cpp
    mesh_illustration_math.cpp
    mesh_illustration_occlusion.cpp
    mesh_illustration_operation.cpp
    illustration_clipping.cpp
    mesh_illustration_prepare.cpp
    mesh_illustration_style.cpp
    mesh_illustration_svg.cpp
    mesh_illustration_svg_lines.cpp
    mesh_illustration_visibility.cpp
    model_bounds.cpp
    analytic_illustration_lowering.cpp
    model_illustration_operation.cpp
    model_illustration_source_cache.cpp
    model_illustration_source_cache_size.cpp
    model_illustration_transform.cpp
    model_tessellation.cpp
    model_tessellation_operation.cpp
    model_bounds_options_json.cpp
    operation_c_api.cpp
    operation_registry.cpp
    operation_transport.cpp
    planar_contours.cpp
    planar_solve.cpp
    planar_step.cpp
    planar_triangulate.cpp
    projection.cpp
    projection_options_json.cpp
    sha256.cpp
    step_media_type.cpp
    step_to_glb.cpp
    step_to_glb_options_json.cpp
    step_xcaf_root_frame.cpp
    version.cpp
    geometer/generated/contracts/contracts_json.cpp
    geometer/generated/contracts/operation_catalog.cpp
    ${CMAKE_SOURCE_DIR}/third_party/clipper2/src/clipper.engine.cpp
    ${CMAKE_SOURCE_DIR}/third_party/clipper2/src/clipper.offset.cpp
    ${CMAKE_SOURCE_DIR}/third_party/clipper2/src/clipper.rectclip.cpp
    ${CMAKE_SOURCE_DIR}/third_party/clipper2/src/clipper.triangulation.cpp
)

if(NOT EMSCRIPTEN)
    list(APPEND GEOMETER_LIB_SOURCES
        ipc_a0_frame.cpp
        ipc_a0_server.cpp
        native_operation_execution_gate.cpp
        native_topology_operation_registry.cpp
        step_topology_edit_journal.cpp
        step_topology_inspection.cpp
        step_topology_glb.cpp
        step_topology_hierarchy.cpp
        step_topology_logical_groups.cpp
        step_topology_metadata_probes.cpp
        step_topology_recovery.cpp
        step_topology_render_binding.cpp
        step_topology_resident_accounting.cpp
        step_topology_session.cpp
        step_topology_session_store.cpp
    )
endif()

add_library(geometer_lib STATIC ${GEOMETER_LIB_SOURCES})
set_target_properties(geometer_lib PROPERTIES OUTPUT_NAME geometer)

if(NOT DEFINED GEOMETER_ABI_VERSION)
    message(FATAL_ERROR
        "GEOMETER_ABI_VERSION must be defined by the top-level Geometer configuration"
    )
endif()
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/geometer/version_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/geometer/version_config.h"
    @ONLY
)

# OCCT toolkit target names.
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
    TKBO
    TKPrim
    TKBRep
    TKGeomBase
    TKG3d
    TKG2d
    TKMath
    TKernel
)

# Standard OCAF document persistence is a native research/cache surface. Keep
# these drivers out of the browser WASM link closure until a browser use case is
# intentionally designed and measured.
if(NOT EMSCRIPTEN)
    list(PREPEND GEOMETER_OCCT_LIBS TKBinXCAF TKXmlXCAF)
endif()

set(GEOMETER_OCCT_LINK_LIBS ${GEOMETER_OCCT_LIBS})
if(UNIX AND NOT EMSCRIPTEN AND GEOMETER_OCCT_LIBRARY_TYPE STREQUAL "Static" AND OpenCASCADE_LIBRARIES)
    set(GEOMETER_OCCT_LINK_LIBS ${OpenCASCADE_LIBRARIES})
endif()
list(REMOVE_ITEM GEOMETER_OCCT_LINK_LIBS TKHLR)

# The static OCCT visualization and data exchange exports also advertise
# TKHLR. Geometer uses their STEP/material code without their HLR entry points.
if(GEOMETER_OCCT_LIBRARY_TYPE STREQUAL "Static")
    foreach(toolkit IN ITEMS TKV3d TKDEVRML)
        if(TARGET ${toolkit})
            get_target_property(dependencies ${toolkit} INTERFACE_LINK_LIBRARIES)
            list(REMOVE_ITEM dependencies TKHLR)
            set_property(TARGET ${toolkit} PROPERTY INTERFACE_LINK_LIBRARIES "${dependencies}")
        endif()
    endforeach()
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

    # The XCAF persistence tests instantiate OCCT presentation classes directly.
    set(toolkits ${GEOMETER_OCCT_LINK_LIBS}
        "$<$<BOOL:$<TARGET_PROPERTY:GEOMETER_OCCT_PRESENTATION_TEST>>:TKHLR>")
    if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN AND GEOMETER_OCCT_LIBRARY_TYPE STREQUAL "Static")
        string(REPLACE ";" "," _geometer_occt_link_group "${toolkits}")
        target_link_libraries(${target_name} PUBLIC "$<LINK_GROUP:RESCAN,${_geometer_occt_link_group}>")
    else()
        target_link_libraries(${target_name} PUBLIC ${toolkits})
    endif()

    if(UNIX AND NOT EMSCRIPTEN)
        target_link_libraries(${target_name} PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
    endif()

    if(WIN32)
        target_link_libraries(${target_name} PUBLIC ws2_32 bcrypt)
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
        "-sEXPORTED_FUNCTIONS=['_malloc','_free','_geometer_version_string','_geometer_version_major','_geometer_version_minor','_geometer_version_patch','_geometer_abi_version','_geometer_step_hlr_projection_json','_geometer_step_hlr_projection_json_bytes','_geometer_step_to_glb','_geometer_step_to_glb_bytes','_geometer_planar_batch_solve','_geometer_planar_batch_solve_bytes','_geometer_planar_batch_solve_json','_geometer_planar_batch_solve_json_bytes','_geometer_planar_triangulate','_geometer_planar_triangulate_bytes','_geometer_clipper2_boolean','_geometer_clipper2_boolean_bytes','_geometer_clipper2_inflate_open','_geometer_clipper2_inflate_open_bytes','_geometer_operation_catalog_json','_geometer_operation_execute','_geometer_operation_result_json_data','_geometer_operation_result_json_size','_geometer_operation_result_attachment_count','_geometer_operation_result_attachment_name','_geometer_operation_result_attachment_media_type','_geometer_operation_result_attachment_data','_geometer_operation_result_free','_geometer_free_string','_geometer_free_bytes']"
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
        "-sEXPORTED_FUNCTIONS=['_malloc','_free','_geometer_version_string','_geometer_version_major','_geometer_version_minor','_geometer_version_patch','_geometer_abi_version','_geometer_planar_batch_solve','_geometer_planar_batch_solve_bytes','_geometer_planar_batch_solve_json','_geometer_planar_batch_solve_json_bytes','_geometer_planar_triangulate','_geometer_planar_triangulate_bytes','_geometer_clipper2_boolean','_geometer_clipper2_boolean_bytes','_geometer_clipper2_inflate_open','_geometer_clipper2_inflate_open_bytes','_geometer_free_string','_geometer_free_bytes']"
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
