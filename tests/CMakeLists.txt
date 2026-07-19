cmake_minimum_required(VERSION 3.22)
project(BoSP_Tests)

add_executable(test_waveshaper tests_WaveShaper.cpp)

# Include shared headers
target_include_directories(test_waveshaper PRIVATE "${CMAKE_SOURCE_DIR}/../shared/DSP")

# No external libraries required

enable_testing()
add_test(NAME WaveShaperTests COMMAND test_waveshaper)
