add_executable(geometer_planar_contours_test
    planar_contours_test.cpp
    ${CMAKE_SOURCE_DIR}/src/cpp/lib/planar_contours.cpp
)

target_include_directories(geometer_planar_contours_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/cpp/lib
)

add_test(NAME geometer_planar_contours_test COMMAND geometer_planar_contours_test)

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

add_executable(geometer_analytic_planar_boolean_feasibility_test
    analytic_planar_boolean_feasibility_test.cpp
)

target_link_libraries(geometer_analytic_planar_boolean_feasibility_test PRIVATE geometer_lib)

add_test(
    NAME geometer_analytic_planar_boolean_feasibility_test
    COMMAND geometer_analytic_planar_boolean_feasibility_test
)

add_executable(geometer_analytic_filtered_core_test
    analytic_filtered_core_test.cpp
)

target_link_libraries(geometer_analytic_filtered_core_test PRIVATE geometer_lib)

add_test(NAME geometer_analytic_filtered_core_test COMMAND geometer_analytic_filtered_core_test)

add_executable(geometer_analytic_filtered_lowering_test
    analytic_filtered_lowering_test.cpp
)

target_link_libraries(geometer_analytic_filtered_lowering_test PRIVATE geometer_lib)

add_test(NAME geometer_analytic_filtered_lowering_test
         COMMAND geometer_analytic_filtered_lowering_test)

add_executable(geometer_analytic_filtered_overlay_test
    analytic_filtered_overlay_test.cpp
)

target_link_libraries(geometer_analytic_filtered_overlay_test PRIVATE geometer_lib)

add_test(NAME geometer_analytic_filtered_overlay_test
         COMMAND geometer_analytic_filtered_overlay_test)

add_executable(geometer_analytic_filtered_arrangement_test
    analytic_filtered_arrangement_test.cpp
)

target_link_libraries(geometer_analytic_filtered_arrangement_test PRIVATE geometer_lib)

add_test(NAME geometer_analytic_filtered_arrangement_test
         COMMAND geometer_analytic_filtered_arrangement_test)

add_executable(geometer_analytic_filtered_boolean_selection_test
    analytic_filtered_boolean_selection_test.cpp
)

target_link_libraries(geometer_analytic_filtered_boolean_selection_test PRIVATE geometer_lib)

add_test(NAME geometer_analytic_filtered_boolean_selection_test
         COMMAND geometer_analytic_filtered_boolean_selection_test)

add_executable(geometer_exact_rational_test
    exact_rational_test.cpp
)

if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_rational_test PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_rational_test PRIVATE geometer_lib)
endif()

add_test(NAME geometer_exact_rational_test COMMAND geometer_exact_rational_test)

add_executable(geometer_exact_polynomial_test
    exact_polynomial_test.cpp
)

if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_polynomial_test PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_polynomial_test PRIVATE geometer_lib)
endif()

add_test(NAME geometer_exact_polynomial_test COMMAND geometer_exact_polynomial_test)

add_executable(geometer_exact_resultant_test
    exact_resultant_test.cpp
)

if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_resultant_test PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_resultant_test PRIVATE geometer_lib)
endif()

add_test(NAME geometer_exact_resultant_test COMMAND geometer_exact_resultant_test)

add_executable(geometer_exact_factorization_test exact_factorization_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_factorization_test PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_factorization_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_exact_factorization_test COMMAND geometer_exact_factorization_test)

add_executable(geometer_exact_value_test exact_value_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_value_test PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_value_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_exact_value_test COMMAND geometer_exact_value_test)

add_executable(geometer_exact_value_codec_test exact_value_codec_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_value_codec_test PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_value_codec_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_exact_value_codec_test COMMAND geometer_exact_value_codec_test)

add_executable(geometer_exact_expression_test exact_expression_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_expression_test PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_expression_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_exact_expression_test COMMAND geometer_exact_expression_test)

add_executable(geometer_exact_construction_test exact_construction_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_construction_test PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_construction_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_exact_construction_test COMMAND geometer_exact_construction_test)

add_executable(geometer_exact_artifact_test exact_artifact_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_artifact_test PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_artifact_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_exact_artifact_test COMMAND geometer_exact_artifact_test)

add_executable(geometer_exact_geometry_test exact_geometry_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_geometry_test PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_geometry_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_exact_geometry_test COMMAND geometer_exact_geometry_test)

add_executable(geometer_exact_curve_domain_test exact_curve_domain_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_curve_domain_test PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_curve_domain_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_exact_curve_domain_test COMMAND geometer_exact_curve_domain_test)

add_executable(geometer_exact_arrangement_test exact_arrangement_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_arrangement_test PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_arrangement_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_exact_arrangement_test COMMAND geometer_exact_arrangement_test)

add_executable(geometer_exact_boolean_stages_test exact_boolean_stages_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_boolean_stages_test PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_boolean_stages_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_exact_boolean_stages_test COMMAND geometer_exact_boolean_stages_test)

add_executable(geometer_exact_boolean_regions_test exact_boolean_regions_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_boolean_regions_test PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_boolean_regions_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_exact_boolean_regions_test COMMAND geometer_exact_boolean_regions_test)

add_executable(geometer_exact_boolean_provenance_test exact_boolean_provenance_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_boolean_provenance_test PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_boolean_provenance_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_exact_boolean_provenance_test COMMAND geometer_exact_boolean_provenance_test)

add_executable(geometer_exact_boolean_outcomes_test exact_boolean_outcomes_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_boolean_outcomes_test PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_boolean_outcomes_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_exact_boolean_outcomes_test COMMAND geometer_exact_boolean_outcomes_test)

add_executable(geometer_exact_boolean_lineage_matrix_test
               exact_boolean_lineage_matrix_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_boolean_lineage_matrix_test
                          PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_boolean_lineage_matrix_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_exact_boolean_lineage_matrix_test
         COMMAND geometer_exact_boolean_lineage_matrix_test)

add_executable(geometer_exact_result_normalization_test exact_result_normalization_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_result_normalization_test PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_result_normalization_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_exact_result_normalization_test
         COMMAND geometer_exact_result_normalization_test)

add_executable(geometer_exact_source_sets_test exact_source_sets_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_source_sets_test PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_source_sets_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_exact_source_sets_test COMMAND geometer_exact_source_sets_test)

add_executable(geometer_analytic_request_packet_test
               analytic_request_packet_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_analytic_request_packet_test
                          PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_analytic_request_packet_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_analytic_request_packet_test
         COMMAND geometer_analytic_request_packet_test)

add_executable(geometer_analytic_operand_lowering_test
               analytic_operand_lowering_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_analytic_operand_lowering_test
                          PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_analytic_operand_lowering_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_analytic_operand_lowering_test
         COMMAND geometer_analytic_operand_lowering_test)

add_executable(geometer_analytic_result_packet_layout_test
               analytic_result_packet_layout_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_analytic_result_packet_layout_test
                          PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_analytic_result_packet_layout_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_analytic_result_packet_layout_test
         COMMAND geometer_analytic_result_packet_layout_test)

add_executable(geometer_analytic_result_packet_records_test
               analytic_result_packet_records_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_analytic_result_packet_records_test
                          PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_analytic_result_packet_records_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_analytic_result_packet_records_test
         COMMAND geometer_analytic_result_packet_records_test)

add_executable(geometer_analytic_result_packet_topology_test
               analytic_result_packet_topology_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_analytic_result_packet_topology_test
                          PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_analytic_result_packet_topology_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_analytic_result_packet_topology_test
         COMMAND geometer_analytic_result_packet_topology_test)

add_executable(geometer_analytic_closed_form_invariants_test
               analytic_closed_form_invariants_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_analytic_closed_form_invariants_test
                          PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_analytic_closed_form_invariants_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_analytic_closed_form_invariants_test
         COMMAND geometer_analytic_closed_form_invariants_test)

add_executable(geometer_exact_degeneracy_sweep_test
               exact_degeneracy_sweep_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_degeneracy_sweep_test
                          PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_degeneracy_sweep_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_exact_degeneracy_sweep_test
         COMMAND geometer_exact_degeneracy_sweep_test)

add_executable(geometer_exact_boolean_identities_test
               exact_boolean_identities_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_boolean_identities_test
                          PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_boolean_identities_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_exact_boolean_identities_test
         COMMAND geometer_exact_boolean_identities_test)

add_executable(geometer_exact_boolean_metamorphic_test
               exact_boolean_metamorphic_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_boolean_metamorphic_test
                          PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_boolean_metamorphic_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_exact_boolean_metamorphic_test
         COMMAND geometer_exact_boolean_metamorphic_test)

add_executable(geometer_exact_boolean_seeded_property_test
               exact_boolean_seeded_property_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_boolean_seeded_property_test
                          PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_boolean_seeded_property_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_exact_boolean_seeded_property_test
         COMMAND geometer_exact_boolean_seeded_property_test)

add_executable(geometer_exact_rectangle_enumeration_test
               exact_rectangle_enumeration_test.cpp)
if(EMSCRIPTEN)
    target_link_libraries(geometer_exact_rectangle_enumeration_test
                          PRIVATE geometer_exact_feasibility)
else()
    target_link_libraries(geometer_exact_rectangle_enumeration_test PRIVATE geometer_lib)
endif()
add_test(NAME geometer_exact_rectangle_enumeration_test
         COMMAND geometer_exact_rectangle_enumeration_test)

if(EMSCRIPTEN)
    set_target_properties(
        geometer_analytic_planar_boolean_feasibility_test
        geometer_analytic_filtered_core_test
        geometer_analytic_filtered_lowering_test
        geometer_analytic_filtered_overlay_test
        geometer_analytic_filtered_arrangement_test
        geometer_analytic_filtered_boolean_selection_test
        geometer_exact_rational_test
        geometer_exact_polynomial_test
        geometer_exact_resultant_test
        geometer_exact_factorization_test
        geometer_exact_value_test
        geometer_exact_value_codec_test
        geometer_exact_expression_test
        geometer_exact_construction_test
        geometer_exact_artifact_test
        geometer_exact_geometry_test
        geometer_exact_curve_domain_test
        geometer_exact_arrangement_test
        geometer_exact_boolean_stages_test
        geometer_exact_boolean_regions_test
        geometer_exact_boolean_provenance_test
        geometer_exact_boolean_outcomes_test
        geometer_exact_boolean_lineage_matrix_test
        geometer_exact_result_normalization_test
        geometer_exact_source_sets_test
        geometer_analytic_request_packet_test
        geometer_analytic_operand_lowering_test
        geometer_analytic_result_packet_layout_test
        geometer_analytic_result_packet_records_test
        geometer_analytic_result_packet_topology_test
        geometer_analytic_closed_form_invariants_test
        geometer_exact_degeneracy_sweep_test
        geometer_exact_boolean_identities_test
        geometer_exact_boolean_metamorphic_test
        geometer_exact_boolean_seeded_property_test
        geometer_exact_rectangle_enumeration_test
        PROPERTIES SUFFIX ".cjs"
    )
    target_compile_options(geometer_exact_rational_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_rational_test PRIVATE -fexceptions)
    target_compile_options(geometer_exact_polynomial_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_polynomial_test PRIVATE -fexceptions)
    target_compile_options(geometer_exact_resultant_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_resultant_test PRIVATE -fexceptions)
    target_compile_options(geometer_exact_factorization_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_factorization_test PRIVATE -fexceptions)
    target_compile_options(geometer_exact_value_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_value_test PRIVATE -fexceptions)
    target_compile_options(geometer_exact_value_codec_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_value_codec_test PRIVATE -fexceptions)
    target_compile_options(geometer_exact_expression_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_expression_test PRIVATE -fexceptions)
    target_compile_options(geometer_exact_construction_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_construction_test PRIVATE -fexceptions)
    target_compile_options(geometer_exact_artifact_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_artifact_test PRIVATE -fexceptions)
    target_compile_options(geometer_exact_geometry_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_geometry_test PRIVATE -fexceptions)
    target_compile_options(geometer_exact_curve_domain_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_curve_domain_test PRIVATE -fexceptions)
    target_compile_options(geometer_exact_arrangement_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_arrangement_test PRIVATE -fexceptions)
    target_compile_options(geometer_exact_boolean_stages_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_boolean_stages_test PRIVATE -fexceptions)
    target_compile_options(geometer_exact_boolean_regions_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_boolean_regions_test PRIVATE -fexceptions)
    target_compile_options(geometer_exact_boolean_provenance_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_boolean_provenance_test PRIVATE -fexceptions)
    target_compile_options(geometer_exact_boolean_outcomes_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_boolean_outcomes_test PRIVATE -fexceptions)
    target_compile_options(geometer_exact_boolean_lineage_matrix_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_boolean_lineage_matrix_test PRIVATE -fexceptions)
    target_compile_options(geometer_exact_result_normalization_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_result_normalization_test PRIVATE -fexceptions)
    target_compile_options(geometer_exact_source_sets_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_source_sets_test PRIVATE -fexceptions)
    target_compile_options(geometer_analytic_request_packet_test PRIVATE -fexceptions)
    target_link_options(geometer_analytic_request_packet_test PRIVATE -fexceptions)
    target_compile_options(geometer_analytic_operand_lowering_test PRIVATE -fexceptions)
    # The occurrence-budget case intentionally lowers 131072 occurrences, which
    # outgrows the default Emscripten heap.
    target_link_options(geometer_analytic_operand_lowering_test PRIVATE
        -fexceptions
        -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_MEMORY=67108864
        -sSTACK_SIZE=8388608
    )
    target_compile_options(geometer_analytic_result_packet_layout_test PRIVATE -fexceptions)
    target_link_options(geometer_analytic_result_packet_layout_test PRIVATE -fexceptions)
    target_compile_options(geometer_analytic_result_packet_records_test PRIVATE -fexceptions)
    target_link_options(geometer_analytic_result_packet_records_test PRIVATE -fexceptions)
    target_compile_options(geometer_analytic_result_packet_topology_test PRIVATE -fexceptions)
    target_link_options(geometer_analytic_result_packet_topology_test PRIVATE -fexceptions)
    target_compile_options(geometer_analytic_closed_form_invariants_test PRIVATE -fexceptions)
    target_link_options(geometer_analytic_closed_form_invariants_test PRIVATE -fexceptions)
    target_compile_options(geometer_exact_degeneracy_sweep_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_degeneracy_sweep_test PRIVATE -fexceptions)
    target_compile_options(geometer_exact_boolean_identities_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_boolean_identities_test PRIVATE -fexceptions)
    target_compile_options(geometer_exact_boolean_metamorphic_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_boolean_metamorphic_test PRIVATE -fexceptions)
    target_compile_options(geometer_exact_boolean_seeded_property_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_boolean_seeded_property_test PRIVATE -fexceptions)
    target_compile_options(geometer_exact_rectangle_enumeration_test PRIVATE -fexceptions)
    target_link_options(geometer_exact_rectangle_enumeration_test PRIVATE -fexceptions)
endif()

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

add_executable(geometer_operation_contract_test
    operation_contract_test.cpp
)

target_link_libraries(geometer_operation_contract_test PRIVATE geometer_lib)

configure_geometer_fixture_test(geometer_operation_contract_test
    tests/contracts/vectors
    tests/fixtures/step/embedded_models/SOT-23.STEP
)

add_test(NAME geometer_operation_contract_test COMMAND geometer_operation_contract_test)

if(NOT EMSCRIPTEN)
    add_executable(geometer_ipc_a0_frame_test
        ipc_a0_frame_test.cpp
    )

    target_link_libraries(geometer_ipc_a0_frame_test PRIVATE geometer_lib)

    add_test(NAME geometer_ipc_a0_frame_test COMMAND geometer_ipc_a0_frame_test)

    add_executable(geometer_ipc_a0_deadline_test_server
        ipc_a0_deadline_test_server.cpp
    )

    target_link_libraries(geometer_ipc_a0_deadline_test_server PRIVATE geometer_lib)

    add_executable(geometer_ipc_a0_unexpected_exit_test_server
        ipc_a0_unexpected_exit_test_server.cpp
    )

    target_link_libraries(geometer_ipc_a0_unexpected_exit_test_server PRIVATE geometer_lib)
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

if(EMSCRIPTEN)
    # The repository package is ESM, while these executable test harnesses use
    # Emscripten's CommonJS Node runtime. Keep every registered WASM CTest on
    # the same .cjs policy as the exact/synthetic targets above.
    set_target_properties(
        geometer_planar_contours_test
        geometer_planar_solve_test
        geometer_planar_step_test
        geometer_model_bounds_test
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
        geometer_analytic_planar_boolean_feasibility_test
        geometer_exact_rational_test
        geometer_exact_polynomial_test
        geometer_model_bounds_test
        geometer_operation_contract_test
        geometer_ipc_a0_frame_test
        geometer_projection_options_json_test
        geometer_projection_model_transform_test
        geometer_projection_outline_test
        PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${OpenCASCADE_BINARY_DIR}"
    )
endif()
