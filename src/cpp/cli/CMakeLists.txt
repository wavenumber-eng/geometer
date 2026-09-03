add_executable(geometer main.cpp mesh_hlr_command.cpp)
target_link_libraries(geometer PRIVATE geometer_lib)
target_include_directories(geometer PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party/rapidjson/include
)

if(EMSCRIPTEN)
    # Build as a Node.js CLI that reads/writes the real filesystem.
    target_link_options(geometer PRIVATE
        -sNODERAWFS=1
        -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_MEMORY=67108864
        -sSTACK_SIZE=8388608
        -sENVIRONMENT=node
        -sMODULARIZE=0
        -sEXIT_RUNTIME=1
    )
    set_target_properties(geometer PROPERTIES
        OUTPUT_NAME "geometer-node-test"
        SUFFIX ".js"
    )
endif()

if(EMSCRIPTEN)
    # Copy Node CLI parity/test outputs to the grouped WASM dist folder.
    add_custom_command(TARGET geometer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GEOMETER_WASM_NODE_TEST_DIST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:geometer>"
            "${GEOMETER_WASM_NODE_TEST_DIST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE_DIR:geometer>/geometer-node-test.wasm"
            "${GEOMETER_WASM_NODE_TEST_DIST_DIR}/"
        COMMENT "Copying geometer-node-test outputs to dist/wasm/node-test/"
    )
else()
    # This phony target always refreshes source identity, even when the executable
    # itself did not relink (for example after a documentation-only revision).
    add_custom_target(geometer_native_distribution ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GEOMETER_NATIVE_DIST_DIR}"
        COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_SOURCE_DIR}/scripts/native_build_attestation.py" write
            --executable "$<TARGET_FILE:geometer>"
            --output "$<TARGET_FILE_DIR:geometer>/geometer.build-attestation.json"
            --geometer-version "${PROJECT_VERSION}"
            --c-abi-version "${GEOMETER_ABI_VERSION}"
            --compiler-id "${CMAKE_CXX_COMPILER_ID}"
            --compiler-version "${CMAKE_CXX_COMPILER_VERSION}"
            --platform "${_geometer_dist_os}"
            --arch "${_geometer_dist_arch}"
            --build-type "$<CONFIG>"
            --occt-profile "${OpenCASCADE_INSTALL_PREFIX}/.geometer-occt-profile.json"
            --occt-version "${OpenCASCADE_VERSION}"
            --generator "${CMAKE_GENERATOR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:geometer>"
            "${GEOMETER_NATIVE_DIST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE_DIR:geometer>/geometer.build-attestation.json"
            "${GEOMETER_NATIVE_DIST_DIR}/geometer.build-attestation.json"
        DEPENDS geometer "${CMAKE_SOURCE_DIR}/scripts/native_build_attestation.py"
        COMMENT "Copying geometer to dist/native/${GEOMETER_NATIVE_DIST_PLATFORM}/"
    )
    geometer_copy_occt_runtime_to_native_dist(geometer)
endif()
