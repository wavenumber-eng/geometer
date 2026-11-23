add_executable(geometer_preview_depth_buffer_test preview_depth_buffer_test.cpp)
add_test(NAME geometer_preview_depth_buffer_test COMMAND geometer_preview_depth_buffer_test)

add_executable(geometer_planar_contours_test
    planar_contours_test.cpp
    ${CMAKE_SOURCE_DIR}/src/cpp/lib/planar_contours.cpp
)

target_include_directories(geometer_planar_contours_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/cpp/lib
)

add_test(NAME geometer_planar_contours_test COMMAND geometer_planar_contours_test)

add_executable(geometer_fast_hlr_test
    fast_hlr_test.cpp
)

target_link_libraries(geometer_fast_hlr_test PRIVATE geometer_lib)

add_test(NAME geometer_fast_hlr_test COMMAND geometer_fast_hlr_test)

add_executable(geometer_indexed_mesh_packet_test
    indexed_mesh_packet_test.cpp
)

target_link_libraries(geometer_indexed_mesh_packet_test PRIVATE geometer_lib)

add_test(NAME geometer_indexed_mesh_packet_test COMMAND geometer_indexed_mesh_packet_test)

add_executable(geometer_planar_solve_test
    planar_solve_test.cpp
)

target_link_libraries(geometer_planar_solve_test PRIVATE geometer_lib)

add_test(NAME geometer_planar_solve_test COMMAND geometer_planar_solve_test)

add_executable(geometer_planar_step_test
    planar_step_test.cpp
)

target_link_libraries(geometer_planar_step_test PRIVATE geometer_lib)

add_test(NAME geometer_planar_step_test COMMAND geometer_planar_step_test)

function(configure_geometer_fixture_test target_name)
    if(EMSCRIPTEN)
        target_compile_definitions(${target_name} PRIVATE
            GEOMETER_TEST_SOURCE_DIR="/repo"
        )
        target_compile_options(${target_name} PRIVATE -fexceptions)
        target_link_options(${target_name} PRIVATE
            -fexceptions
            -sALLOW_MEMORY_GROWTH=1
            -sINITIAL_MEMORY=67108864
            -sSTACK_SIZE=8388608
        )
        foreach(fixture_path IN LISTS ARGN)
            target_link_options(${target_name} PRIVATE
                "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/${fixture_path}@/repo/${fixture_path}"
            )
        endforeach()
    else()
        target_compile_definitions(${target_name} PRIVATE
            GEOMETER_TEST_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
        )
    endif()
endfunction()

add_executable(geometer_model_bounds_test
    model_bounds_test.cpp
)

target_link_libraries(geometer_model_bounds_test PRIVATE geometer_lib)

configure_geometer_fixture_test(geometer_model_bounds_test
    tests/fixtures/step/embedded_models/SOT-23.STEP
)

add_test(NAME geometer_model_bounds_test COMMAND geometer_model_bounds_test)

add_executable(geometer_model_tessellation_test model_tessellation_test.cpp)
target_link_libraries(geometer_model_tessellation_test PRIVATE geometer_lib)
configure_geometer_fixture_test(geometer_model_tessellation_test
    tests/fixtures/step/embedded_models/SOT-23.STEP)
add_test(NAME geometer_model_tessellation_test COMMAND geometer_model_tessellation_test)

if(NOT EMSCRIPTEN)
    find_package(Threads REQUIRED)
    add_executable(geometer_model_source_cache_test model_source_cache_test.cpp)
    target_include_directories(geometer_model_source_cache_test PRIVATE
                               ${PROJECT_SOURCE_DIR}/src/cpp/lib)
    target_link_libraries(geometer_model_source_cache_test PRIVATE geometer_lib Threads::Threads)
    configure_geometer_fixture_test(geometer_model_source_cache_test
        tests/fixtures/step/embedded_models/SOT-23.STEP
        tests/fixtures/step/embedded_models/GT-USB-7010C.STEP
        tests/fixtures/step/generated_topology/generated_fused_slab.step)
    add_test(NAME geometer_model_source_cache_test COMMAND geometer_model_source_cache_test)
    if(WIN32 AND OpenCASCADE_BINARY_DIR)
        set_tests_properties(geometer_model_source_cache_test PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${OpenCASCADE_BINARY_DIR}")
    endif()
endif()

add_executable(geometer_mesh_illustration_test mesh_illustration_test.cpp)
add_executable(geometer_mesh_illustration_geometry_test mesh_illustration_geometry_test.cpp)
add_executable(geometer_model_illustration_native_test model_illustration_native_test.cpp)
add_executable(geometer_illustration_clipping_test illustration_clipping_test.cpp)
target_include_directories(geometer_illustration_clipping_test PRIVATE
                           ${PROJECT_SOURCE_DIR}/src/cpp/lib)
target_link_libraries(geometer_illustration_clipping_test PRIVATE geometer_lib)
add_test(NAME geometer_illustration_clipping_test COMMAND geometer_illustration_clipping_test)
add_executable(geometer_illustration_b0_operation_test illustration_b0_operation_test.cpp)
target_link_libraries(geometer_illustration_b0_operation_test PRIVATE geometer_lib)
add_test(NAME geometer_illustration_b0_operation_test COMMAND geometer_illustration_b0_operation_test)
target_include_directories(geometer_model_illustration_native_test PRIVATE
                           ${PROJECT_SOURCE_DIR}/src/cpp/lib)
target_link_libraries(geometer_model_illustration_native_test PRIVATE geometer_lib)
add_test(NAME geometer_model_illustration_native_test COMMAND geometer_model_illustration_native_test)
target_link_libraries(geometer_mesh_illustration_geometry_test PRIVATE geometer_lib)
add_test(NAME geometer_mesh_illustration_geometry_test COMMAND geometer_mesh_illustration_geometry_test)
target_link_libraries(geometer_mesh_illustration_test PRIVATE geometer_lib)
add_test(NAME geometer_mesh_illustration_test COMMAND geometer_mesh_illustration_test)
if(NOT EMSCRIPTEN)
    find_program(GEOMETER_NODE_EXECUTABLE node)
    if(GEOMETER_NODE_EXECUTABLE)
        add_test(NAME geometer_mesh_illustration_parity
            COMMAND ${CMAKE_COMMAND} -E env "GEOMETER_PARITY_PYTHON=${Python3_EXECUTABLE}"
                ${GEOMETER_NODE_EXECUTABLE}
                ${CMAKE_SOURCE_DIR}/tests/typescript/native_illustration_parity.mjs
                $<TARGET_FILE:geometer_mesh_illustration_test>)
        set_tests_properties(geometer_mesh_illustration_parity PROPERTIES
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} TIMEOUT 90)
    endif()
endif()

add_executable(geometer_operation_contract_test
    operation_contract_topology_semantics.cpp
    operation_contract_test.cpp
    step_media_type_tests.cpp
)

target_link_libraries(geometer_operation_contract_test PRIVATE geometer_lib)

configure_geometer_fixture_test(geometer_operation_contract_test
    tests/contracts/vectors
    tests/fixtures/step_media_types.json
    tests/fixtures/step/embedded_models/SOT-23.STEP
)

add_test(NAME geometer_operation_contract_test COMMAND geometer_operation_contract_test)

if(NOT EMSCRIPTEN)
    add_executable(geometer_native_operation_execution_gate_test
        native_operation_execution_gate_test.cpp
    )

    target_link_libraries(geometer_native_operation_execution_gate_test PRIVATE geometer_lib)

    add_test(NAME geometer_native_operation_execution_gate_test
        COMMAND geometer_native_operation_execution_gate_test)
    set_tests_properties(geometer_native_operation_execution_gate_test PROPERTIES TIMEOUT 10)

    add_executable(geometer_ipc_a0_frame_test
        ipc_a0_frame_test.cpp
    )

    target_link_libraries(geometer_ipc_a0_frame_test PRIVATE geometer_lib)

    add_test(NAME geometer_ipc_a0_frame_test COMMAND geometer_ipc_a0_frame_test)

    add_executable(geometer_c_api_stdio_test_server
        c_api_stdio_test_server.cpp
    )

    target_link_libraries(geometer_c_api_stdio_test_server PRIVATE geometer_lib)

    add_executable(geometer_ipc_a0_deadline_test_server
        ipc_a0_deadline_test_server.cpp
    )

    target_link_libraries(geometer_ipc_a0_deadline_test_server PRIVATE geometer_lib)

    add_executable(geometer_ipc_a0_unexpected_exit_test_server
        ipc_a0_unexpected_exit_test_server.cpp
    )

    target_link_libraries(geometer_ipc_a0_unexpected_exit_test_server PRIVATE geometer_lib)

    add_executable(geometer_ipc_a0_writer_failure_test_server
        ipc_a0_writer_failure_test_server.cpp
    )

    target_link_libraries(geometer_ipc_a0_writer_failure_test_server PRIVATE geometer_lib)
endif()

add_executable(geometer_projection_options_json_test
    projection_options_json_test.cpp
)

target_link_libraries(geometer_projection_options_json_test PRIVATE geometer_lib)

add_test(NAME geometer_projection_options_json_test COMMAND geometer_projection_options_json_test)

add_executable(geometer_projection_model_transform_test
    projection_model_transform_test.cpp
)

target_link_libraries(geometer_projection_model_transform_test PRIVATE geometer_lib)

configure_geometer_fixture_test(geometer_projection_model_transform_test
    tests/fixtures/step/embedded_models/SOT-23.STEP
)

add_test(NAME geometer_projection_model_transform_test COMMAND geometer_projection_model_transform_test)

add_executable(geometer_projection_outline_test
    projection_outline_test.cpp
)

target_link_libraries(geometer_projection_outline_test PRIVATE geometer_lib)

configure_geometer_fixture_test(geometer_projection_outline_test
    tests/fixtures/step/embedded_models/BGA90-8X13mm.step
    tests/fixtures/step/embedded_models/SOT-23.STEP
    tests/fixtures/step/embedded_models/TSOT-23-5.STEP
    tests/fixtures/step/embedded_models/sot223.stp
)

add_test(NAME geometer_projection_outline_test COMMAND geometer_projection_outline_test)

if(NOT EMSCRIPTEN)
    add_executable(geometer_step_topology_session_test
        step_topology_session_test.cpp
    )

    target_link_libraries(geometer_step_topology_session_test PRIVATE geometer_lib)
    configure_geometer_fixture_test(geometer_step_topology_session_test)
    add_test(NAME geometer_step_topology_session_test COMMAND geometer_step_topology_session_test)

    add_executable(geometer_step_topology_logical_groups_test
        step_topology_logical_groups_test.cpp
    )
    target_link_libraries(geometer_step_topology_logical_groups_test PRIVATE geometer_lib)
    configure_geometer_fixture_test(geometer_step_topology_logical_groups_test)
    add_test(NAME geometer_step_topology_logical_groups_test
             COMMAND geometer_step_topology_logical_groups_test)

    add_executable(geometer_step_topology_xcaf_persistence_test
        step_topology_xcaf_persistence_test.cpp
    )
    target_link_libraries(geometer_step_topology_xcaf_persistence_test PRIVATE geometer_lib)
    add_test(NAME geometer_step_topology_xcaf_persistence_test
             COMMAND geometer_step_topology_xcaf_persistence_test)

    add_executable(geometer_step_topology_xcaf_custom_driver_test
        step_topology_xcaf_custom_driver_test.cpp
    )
    target_link_libraries(geometer_step_topology_xcaf_custom_driver_test PRIVATE geometer_lib)
    add_test(NAME geometer_step_topology_xcaf_custom_driver_test
             COMMAND geometer_step_topology_xcaf_custom_driver_test)

    add_executable(geometer_step_topology_ap242_persistence_test
        step_topology_ap242_persistence_test.cpp
    )
    target_link_libraries(geometer_step_topology_ap242_persistence_test PRIVATE geometer_lib)
    add_test(NAME geometer_step_topology_ap242_persistence_test
             COMMAND geometer_step_topology_ap242_persistence_test)

    set_target_properties(
        geometer_step_topology_xcaf_persistence_test
        geometer_step_topology_xcaf_custom_driver_test
        geometer_step_topology_ap242_persistence_test
        PROPERTIES GEOMETER_OCCT_PRESENTATION_TEST TRUE
    )

    add_executable(geometer_step_topology_recovery_test
        step_topology_recovery_test.cpp
    )
    target_link_libraries(geometer_step_topology_recovery_test PRIVATE geometer_lib)
    add_test(NAME geometer_step_topology_recovery_test
             COMMAND geometer_step_topology_recovery_test)

    add_executable(geometer_step_topology_hierarchy_test
        step_topology_hierarchy_test.cpp
    )
    target_link_libraries(geometer_step_topology_hierarchy_test PRIVATE geometer_lib)
    configure_geometer_fixture_test(geometer_step_topology_hierarchy_test)
    add_test(NAME geometer_step_topology_hierarchy_test
             COMMAND geometer_step_topology_hierarchy_test)

    add_executable(geometer_step_topology_brep_graph_test
        step_topology_brep_graph_test.cpp
    )

    target_link_libraries(geometer_step_topology_brep_graph_test PRIVATE geometer_lib)
    add_test(NAME geometer_step_topology_brep_graph_test
             COMMAND geometer_step_topology_brep_graph_test)

    add_executable(geometer_step_topology_render_binding_test
        step_topology_render_binding_test.cpp
    )
    target_link_libraries(geometer_step_topology_render_binding_test PRIVATE geometer_lib)
    configure_geometer_fixture_test(geometer_step_topology_render_binding_test)
    add_test(NAME geometer_step_topology_render_binding_test
             COMMAND geometer_step_topology_render_binding_test)

    add_executable(geometer_step_topology_fixture_inventory
        step_topology_fixture_inventory.cpp
    )

    target_link_libraries(geometer_step_topology_fixture_inventory PRIVATE geometer_lib)

    add_executable(geometer_step_topology_worker_test_server
        step_topology_worker_test_server.cpp
    )

    target_link_libraries(geometer_step_topology_worker_test_server PRIVATE geometer_lib)

    add_executable(geometer_step_topology_glb_fixture_emitter
        step_topology_glb_fixture_emitter.cpp
    )
    target_link_libraries(geometer_step_topology_glb_fixture_emitter PRIVATE geometer_lib)

    add_test(
        NAME geometer_step_topology_fixture_inventory_test
        COMMAND geometer_step_topology_fixture_inventory --self-test
    )

    add_test(
        NAME geometer_step_topology_generated_fixtures_test
        COMMAND geometer_step_topology_fixture_inventory --verify-generated
                ${CMAKE_SOURCE_DIR}/tests/fixtures/step/generated_topology
    )

    if(WIN32)
        add_test(
            NAME geometer_step_topology_fixture_baseline_test
            COMMAND geometer_step_topology_fixture_inventory --check-report
                    docs/reports/step-topology-fixture-baseline.json
                    monolithic
                    tests/fixtures/step/embedded_models/miniature_test_point.stp
                    tests/fixtures/glb/embedded_models/miniature_test_point.glb
                    monolithic
                    tests/fixtures/step/embedded_models/SOT-23.STEP
                    tests/fixtures/glb/embedded_models/SOT-23.glb
                    small_assembly
                    tests/fixtures/step/embedded_models/RESC1608X06L.step
                    tests/fixtures/glb/embedded_models/RESC1608X06L.glb
                    repeated_assembly
                    tests/fixtures/step/embedded_models/SOIC-20-300.STEP
                    tests/fixtures/glb/embedded_models/SOIC-20-300.glb
                    varied_assembly
                    tests/fixtures/step/embedded_models/ABM3B.STEP
                    tests/fixtures/glb/embedded_models/ABM3B.glb
                    generated_repeated_assembly
                    tests/fixtures/step/generated_topology/generated_repeated_occurrences.step
                    -
                    generated_fused_slab
                    tests/fixtures/step/generated_topology/generated_fused_slab.step
                    -
                    generated_flat_multi_solid
                    tests/fixtures/step/generated_topology/generated_flat_multi_solid.step
                    -
        )
        set_tests_properties(
            geometer_step_topology_fixture_baseline_test
            PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    endif()
endif()

if(EMSCRIPTEN)
    # The repository package is ESM, while these executable test harnesses use
    # Emscripten's CommonJS Node runtime. Keep every registered WASM CTest on
    # the same .cjs policy as the exact/synthetic targets above.
    set_target_properties(
        geometer_planar_contours_test
        geometer_fast_hlr_test
        geometer_planar_solve_test
        geometer_planar_step_test
        geometer_model_bounds_test
        geometer_mesh_illustration_test
        geometer_mesh_illustration_geometry_test
        geometer_model_illustration_native_test
        geometer_illustration_clipping_test
        geometer_illustration_b0_operation_test
        geometer_operation_contract_test
        geometer_projection_options_json_test
        geometer_projection_model_transform_test
        geometer_projection_outline_test
        PROPERTIES SUFFIX ".cjs"
    )
endif()

if(WIN32 AND OpenCASCADE_BINARY_DIR)
    set_tests_properties(
        geometer_planar_solve_test
        geometer_planar_step_test
        geometer_model_bounds_test
        geometer_model_illustration_native_test
        geometer_operation_contract_test
        geometer_native_operation_execution_gate_test
        geometer_ipc_a0_frame_test
        geometer_projection_options_json_test
        geometer_projection_model_transform_test
        geometer_projection_outline_test
        geometer_step_topology_session_test
        geometer_step_topology_logical_groups_test
        geometer_step_topology_xcaf_persistence_test
        geometer_step_topology_xcaf_custom_driver_test
        geometer_step_topology_ap242_persistence_test
        geometer_step_topology_recovery_test
        geometer_step_topology_hierarchy_test
        geometer_step_topology_brep_graph_test
        geometer_step_topology_render_binding_test
        geometer_step_topology_fixture_inventory_test
        geometer_step_topology_generated_fixtures_test
        geometer_step_topology_fixture_baseline_test
        PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${OpenCASCADE_BINARY_DIR}"
    )
endif()

# Normal product validation builds and runs only the promoted geometry, HLR,
# illustration, transport, and packaging surface. Retained research suites stay
# registered for explicit qualification runs.
get_property(_geometer_registered_tests DIRECTORY PROPERTY TESTS)
foreach(_geometer_test IN LISTS _geometer_registered_tests)
    if(_geometer_test MATCHES
       "^geometer_step_topology_")
        set_tests_properties(${_geometer_test} PROPERTIES LABELS "experimental")
    else()
        set_tests_properties(${_geometer_test} PROPERTIES LABELS "production")
    endif()
endforeach()
