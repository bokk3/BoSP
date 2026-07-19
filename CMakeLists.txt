cmake_minimum_required(VERSION 3.22)

project(BoDSP VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(external/JUCE)
add_subdirectory(plugins/Distortion)
add_subdirectory(plugins/Delay)
