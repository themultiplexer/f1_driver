cmake_minimum_required(VERSION 3.20)
project(f1_driver VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Find HIDAPI - more robust approach for macOS
if(APPLE)
    # Try to find via Homebrew paths
    find_path(HIDAPI_INCLUDE_DIR 
        NAMES hidapi/hidapi.h
        PATHS 
            /opt/homebrew/include
            /usr/local/include
        NO_DEFAULT_PATH
    )
    
    find_library(HIDAPI_LIBRARY 
        NAMES hidapi
        PATHS 
            /opt/homebrew/lib
            /usr/local/lib
        NO_DEFAULT_PATH
    )
else()
    # Linux approach
    find_path(HIDAPI_INCLUDE_DIR
            NAMES hidapi.h
            PATH_SUFFIXES
            hidapi)
    find_library(HIDAPI_LIBRARY NAMES hidapi-libusb hidapi)
endif()

# Check if we found everything
if(NOT HIDAPI_INCLUDE_DIR OR NOT HIDAPI_LIBRARY)
    message(FATAL_ERROR "HIDAPI not found. On macOS: brew install hidapi^")
endif()

# Add executable
add_library(f1_driver
    src/input_reader_base.cpp
    src/input_reader_fader.cpp
    src/input_reader_knob.cpp
    src/input_reader_wheel.cpp
    src/led_controller_base.cpp
    src/led_controller_display.cpp
    src/startup_sequence.cpp
    src/controller_handler.cpp
    include/controller_handler.h
    include/input_reader_base.h
    include/input_reader_fader.h
    include/input_reader_knob.h
    include/input_reader_wheel.h
    include/led_controller_base.h
    include/led_controller_display.h
    include/startup_sequence.h
)

# Include directories
target_include_directories(f1_driver PRIVATE 
    include/
    ${HIDAPI_INCLUDE_DIR}
    ${RTMIDI_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link the HIDAPI library
target_link_libraries(f1_driver PUBLIC ${HIDAPI_LIBRARY} ${RTMIDI_LIBRARY})
