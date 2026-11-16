cmake_minimum_required(VERSION 3.13)
project(libbgcode CXX)

# Prusa libbgcode 0.2.0 (https://github.com/prusa3d/libbgcode, AGPL-3.0) at commit
# 5041c093b33e2748e76d6b326f2251310823f3df, the commit PrusaSlicer 2.9.4 pins.
# Only the core, binarize and convert components are vendored; the export.h headers
# replace the ones upstream generates. OrcaSlicer changes are marked "OrcaSlicer:".
add_library(libbgcode STATIC
    src/LibBGCode/core/core.cpp
    src/LibBGCode/binarize/binarize.cpp
    src/LibBGCode/binarize/meatpack.cpp
    src/LibBGCode/convert/convert.cpp
)

target_include_directories(libbgcode SYSTEM PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LibBGCode
)
target_compile_features(libbgcode PUBLIC cxx_std_17)
target_compile_definitions(libbgcode PRIVATE LibBGCode_VERSION="0.2.0")
target_link_libraries(libbgcode PRIVATE heatshrink ZLIB::ZLIB boost_headeronly)
set_target_properties(libbgcode PROPERTIES POSITION_INDEPENDENT_CODE ON)
