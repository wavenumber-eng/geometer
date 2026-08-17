set(GEOMETER_LIB_SOURCES
    analytic_curve_broad_phase.cpp
    analytic_curve_narrow_phase.cpp
    analytic_filtered_batch.cpp
    analytic_filtered_boolean_selection.cpp
    analytic_filtered_boolean_selection_admission.cpp
    analytic_filtered_outcome_tracker.cpp
    analytic_filtered_outcomes.cpp
    analytic_filtered_packet.cpp
    analytic_filtered_packet_sequences.cpp
    analytic_filtered_normalization.cpp
    analytic_filtered_normalization_replay.cpp
    analytic_filtered_arrangement.cpp
    analytic_filtered_lowering.cpp
    analytic_filtered_overlay.cpp
    analytic_filtered_regions.cpp
    analytic_filtered_lineage.cpp
    analytic_interval_index.cpp
    analytic_numeric_filter.cpp
    analytic_request_packet.cpp
    analytic_result_packet_canonical.cpp
    analytic_result_packet_layout.cpp
    analytic_result_packet_records.cpp
    analytic_result_packet_standalone.cpp
    analytic_solver_limits.cpp
    c_api.cpp
    clipper2_bytes.cpp
    hlr_projection.cpp
    mesh_shadow_outline.cpp
    model_bounds.cpp
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
    step_to_glb.cpp
    step_to_glb_options_json.cpp
    version.cpp
    geometer/generated/contracts/contracts_json.cpp
    geometer/generated/contracts/operation_catalog.cpp
    ${CMAKE_SOURCE_DIR}/third_party/clipper2/src/clipper.engine.cpp
    ${CMAKE_SOURCE_DIR}/third_party/clipper2/src/clipper.offset.cpp
    ${CMAKE_SOURCE_DIR}/third_party/clipper2/src/clipper.rectclip.cpp
    ${CMAKE_SOURCE_DIR}/third_party/clipper2/src/clipper.triangulation.cpp
)

set(GEOMETER_EXACT_SOURCES
    analytic_operand_lowering.cpp
    analytic_request_packet.cpp
    analytic_result_packet_canonical.cpp
    analytic_result_packet_layout.cpp
    analytic_result_packet_records.cpp
    analytic_result_packet_standalone.cpp
    analytic_result_packet_topology.cpp
    exact_arc_distance.cpp
    exact_boolean_outcomes.cpp
    exact_boolean_provenance.cpp
    exact_boolean_regions.cpp
    exact_boolean_stages.cpp
    exact_arrangement.cpp
    exact_arrangement_classification.cpp
    exact_arrangement_faces.cpp
    exact_arrangement_order.cpp
    exact_arrangement_result.cpp
    exact_arrangement_validation.cpp
    exact_artifact.cpp
    exact_construction.cpp
    exact_construction_builder.cpp
    exact_curve_domain.cpp
    exact_expression.cpp
    exact_factorization.cpp
    exact_geometry.cpp
    exact_normalization.cpp
    exact_polynomial.cpp
    exact_rational.cpp
    exact_resultant.cpp
    exact_result_normalization.cpp
    exact_source_sets.cpp
    exact_value.cpp
    exact_value_codec.cpp
    sha256.cpp
)

# The algebraic implementation is a test/promotion oracle, never part of the
# production library. Keep this configure-time guard beside the source lists so
# a future source addition cannot silently restore Boost-backed exact code to
# the primary native target.
foreach(_geometer_source IN LISTS GEOMETER_LIB_SOURCES)
    if(_geometer_source MATCHES "^exact_" OR
       _geometer_source STREQUAL "analytic_operand_lowering.cpp" OR
       _geometer_source STREQUAL "analytic_result_packet_topology.cpp")
        message(FATAL_ERROR
            "Exact/oracle source '${_geometer_source}' is forbidden in GEOMETER_LIB_SOURCES")
    endif()
endforeach()

if(NOT EMSCRIPTEN)
    list(APPEND GEOMETER_LIB_SOURCES
        ipc_a0_frame.cpp
        ipc_a0_server.cpp
    )
endif()

add_library(geometer_lib STATIC ${GEOMETER_LIB_SOURCES})
set_target_properties(geometer_lib PROPERTIES OUTPUT_NAME geometer)

if(NOT DEFINED GEOMETER_ABI_VERSION)
    set(GEOMETER_ABI_VERSION "20260623" CACHE STRING "Geometer C ABI generation date")
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

if(BUILD_TESTING)
    add_library(geometer_exact_feasibility STATIC ${GEOMETER_EXACT_SOURCES})
    target_include_directories(geometer_exact_feasibility PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${GEOMETER_BOOST_ROOT}
    )
    if(EMSCRIPTEN)
        target_compile_options(geometer_exact_feasibility PUBLIC -fexceptions)
    elseif(MSVC)
        target_compile_options(geometer_exact_feasibility PUBLIC /EHsc)
    endif()
endif()

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
