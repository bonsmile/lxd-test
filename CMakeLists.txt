cmake_minimum_required(VERSION 3.4)

project(lxd-test CXX)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $<1:${PROJECT_SOURCE_DIR}/bin/$<CONFIG>>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${PROJECT_SOURCE_DIR}/bin/$<CONFIG>>)
set(CMAKE_CXX_STANDARD 17)

# fmtlib
add_subdirectory(fmt EXCLUDE_FROM_ALL)
# lxd
add_subdirectory(./lxd lxd)

add_executable(${PROJECT_NAME} main.cpp)
#target_compile_definitions( ${PROJECT_NAME} PUBLIC -DBUILDING_DLL )

set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})