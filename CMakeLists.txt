cmake_minimum_required(VERSION 3.15)

project(isodrive LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -O2)

include_directories(src/include)

set(LIB_SOURCES
    src/util.cpp
    src/logger.cpp
    src/configfsisomanager.cpp
    src/androidusbisomanager.cpp
)

add_library(isodrive_lib STATIC ${LIB_SOURCES})

add_executable(isodrive src/main.cpp)
target_link_libraries(isodrive PRIVATE isodrive_lib)

include(GNUInstallDirs)
install(TARGETS isodrive DESTINATION ${CMAKE_INSTALL_BINDIR})

# Testing
enable_testing()

# Mock library for tests
add_library(mock_sysfs STATIC tests/mock_sysfs.cpp)
target_include_directories(mock_sysfs PUBLIC tests)

# Test: util functions
add_executable(test_util tests/test_util.cpp)
target_link_libraries(test_util PRIVATE isodrive_lib)
target_include_directories(test_util PRIVATE tests)
add_test(NAME test_util COMMAND test_util)

# Test: configfs module
add_executable(test_configfs tests/test_configfs.cpp)
target_link_libraries(test_configfs PRIVATE isodrive_lib mock_sysfs)
target_include_directories(test_configfs PRIVATE tests)
add_test(NAME test_configfs COMMAND test_configfs)

# Test: android module
add_executable(test_android tests/test_android.cpp)
target_link_libraries(test_android PRIVATE isodrive_lib mock_sysfs)
target_include_directories(test_android PRIVATE tests)
add_test(NAME test_android COMMAND test_android)

# Magisk Target
add_custom_target(magisk
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/build_magisk.sh $<TARGET_FILE:isodrive> ${CMAKE_SOURCE_DIR}/isodrive-magisk.zip
    DEPENDS isodrive
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Building Magisk module"
)
