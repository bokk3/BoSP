cmake_minimum_required(VERSION 3.22)

project(BoDSP VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Helper function to copy VST3 bundle and Standalone EXE to root release_bin folder after build
function(bodsp_setup_release_copy TARGET_NAME PRODUCT_NAME)
    # Copy VST3 bundle
    if(TARGET ${TARGET_NAME}_VST3)
        add_custom_command(TARGET ${TARGET_NAME}_VST3 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/release_bin"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:${TARGET_NAME}_VST3>/../.." "${CMAKE_SOURCE_DIR}/release_bin/${PRODUCT_NAME}.vst3"
            COMMENT "Copying ${PRODUCT_NAME} VST3 bundle to root release_bin"
        )
    endif()

    # Copy Standalone executable
    if(TARGET ${TARGET_NAME}_Standalone)
        add_custom_command(TARGET ${TARGET_NAME}_Standalone POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/release_bin"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${TARGET_NAME}_Standalone>" "${CMAKE_SOURCE_DIR}/release_bin/"
            COMMENT "Copying ${PRODUCT_NAME} Standalone executable to root release_bin"
        )
    endif()
endfunction()

add_subdirectory(external/JUCE)
add_subdirectory(plugins/Distortion)
add_subdirectory(plugins/Delay)
add_subdirectory(plugins/Reverb)
add_subdirectory(plugins/Filter)
add_subdirectory(plugins/Chorus)
