cmake_minimum_required(VERSION 3.15)

project(isodrive LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -O2)

include_directories(src/include)

set(LIB_SOURCES
    src/util.cpp
    src/configfsisomanager.cpp
    src/androidusbisomanager.cpp
)

add_library(isodrive_lib STATIC ${LIB_SOURCES})

add_executable(isodrive src/main.cpp)
target_link_libraries(isodrive PRIVATE isodrive_lib)

include(GNUInstallDirs)
install(TARGETS isodrive DESTINATION ${CMAKE_INSTALL_BINDIR})

enable_testing()
add_executable(test_util tests/test_util.cpp)
target_link_libraries(test_util PRIVATE isodrive_lib)
add_test(NAME test_util COMMAND test_util)

# Magisk Target
add_custom_target(magisk
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/build_magisk.sh $<TARGET_FILE:isodrive> ${CMAKE_SOURCE_DIR}/isodrive-magisk.zip
    DEPENDS isodrive
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Building Magisk module"
)
