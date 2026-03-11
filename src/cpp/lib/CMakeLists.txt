add_library(geometer_lib STATIC
    geometer.cpp
)

target_include_directories(geometer_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(geometer_lib PUBLIC
    TKXDEStep
    TKRWMesh
    TKMesh
    TKXCAF
    TKLCAF
    TKSTEP
    TKSTEPBase
    TKSTEPAttr
    TKSTEP209
    TKXSBase
    TKShHealing
    TKGeomAlgo
    TKTopAlgo
    TKBRep
    TKGeomBase
    TKG3d
    TKG2d
    TKMath
    TKernel
)
