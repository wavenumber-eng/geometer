add_library(geometer_lib STATIC
    geometer.cpp
)

target_include_directories(geometer_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OpenCASCADE_INCLUDE_DIR}
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
    TKBO
    TKBRep
    TKGeomBase
    TKG3d
    TKG2d
    TKMath
    TKernel
)
