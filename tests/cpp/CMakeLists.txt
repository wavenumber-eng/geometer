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

if(EMSCRIPTEN)
    set_target_properties(
        geometer_analytic_planar_boolean_feasibility_test
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
endif()

add_executable(geometer_model_bounds_test
    model_bounds_test.cpp
)

target_link_libraries(geometer_model_bounds_test PRIVATE geometer_lib)

target_compile_definitions(geometer_model_bounds_test PRIVATE
    GEOMETER_TEST_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

add_test(NAME geometer_model_bounds_test COMMAND geometer_model_bounds_test)

add_executable(geometer_operation_contract_test
    operation_contract_test.cpp
)

target_link_libraries(geometer_operation_contract_test PRIVATE geometer_lib)

target_compile_definitions(geometer_operation_contract_test PRIVATE
    GEOMETER_TEST_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
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

target_compile_definitions(geometer_projection_model_transform_test PRIVATE
    GEOMETER_TEST_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

add_test(NAME geometer_projection_model_transform_test COMMAND geometer_projection_model_transform_test)

add_executable(geometer_projection_outline_test
    projection_outline_test.cpp
)

target_link_libraries(geometer_projection_outline_test PRIVATE geometer_lib)

target_compile_definitions(geometer_projection_outline_test PRIVATE
    GEOMETER_TEST_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

add_test(NAME geometer_projection_outline_test COMMAND geometer_projection_outline_test)

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
