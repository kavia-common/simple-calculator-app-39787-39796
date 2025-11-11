cmake_minimum_required(VERSION 3.16)

# Use a simple project name without spaces/special characters to avoid any quoting issues in shell wrappers.
project(MainApp VERSION 1.0.0 LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Qt6 components. Keep components explicit to avoid accidental shell expansion if wrappers parse logs.
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

# Enable Qt's automoc, autorcc and autouic just in case future UI/resources are added.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Include headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Sources and headers
set(SOURCES
    src/mainApp.cpp
)

set(HEADERS
    include/mainApp.h
)

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link against required Qt libraries
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui)

# Output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# Convenience target to run the app from build tree
add_custom_target(run
    COMMAND $<TARGET_FILE:${PROJECT_NAME}>
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Running ${PROJECT_NAME}"
)

# Tests
enable_testing()
add_test(NAME ${PROJECT_NAME}Test COMMAND ${PROJECT_NAME})
