cmake_minimum_required(VERSION 3.13)
project(heatshrink C)

# heatshrink 0.4.1 (https://github.com/atomicobject/heatshrink, ISC license), the version
# PrusaSlicer pins. libbgcode uses it to compress the G-code blocks of binary G-code.
# Built with dynamic allocation, the heatshrink_config.h default.
add_library(heatshrink STATIC
    heatshrink/heatshrink_decoder.c
    heatshrink/heatshrink_encoder.c
)

target_include_directories(heatshrink SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(heatshrink PRIVATE c_std_99)
set_target_properties(heatshrink PROPERTIES POSITION_INDEPENDENT_CODE ON)
