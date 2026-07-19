cmake_minimum_required(VERSION 3.22)
project(BoSP_Tests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(test_waveshaper tests_WaveShaper.cpp)
add_executable(test_reverb tests_Reverb.cpp)
add_executable(test_filter tests_Filter.cpp)

# Include shared headers
target_include_directories(test_waveshaper PRIVATE "${CMAKE_SOURCE_DIR}/../shared/DSP")
target_include_directories(test_reverb PRIVATE "${CMAKE_SOURCE_DIR}/../shared/DSP")
target_include_directories(test_filter PRIVATE "${CMAKE_SOURCE_DIR}/../shared/DSP")

# No external libraries required

enable_testing()
add_test(NAME WaveShaperTests COMMAND test_waveshaper)
add_test(NAME ReverbTests COMMAND test_reverb)
add_test(NAME FilterTests COMMAND test_filter)
