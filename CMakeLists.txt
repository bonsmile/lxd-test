cmake_minimum_required(VERSION 3.4)

project(lxd-test CXX)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $<1:${PROJECT_SOURCE_DIR}/bin/$<CONFIG>>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${PROJECT_SOURCE_DIR}/bin/$<CONFIG>>)
set(CMAKE_CXX_STANDARD 17)

# fmtlib
add_subdirectory(fmt EXCLUDE_FROM_ALL)
# lxd
add_subdirectory(./lxd lxd)
#
find_package(GTest CONFIG REQUIRED)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE lxd GTest::gmock GTest::gtest GTest::gmock_main GTest::gtest_main)

set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})