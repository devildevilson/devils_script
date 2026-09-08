# Builds the library and a numeric-contract check with DEVILS_SCRIPT_32BIT=1, so the 32-bit
# configuration is compiled and executed by the normal test run instead of only being believed.
cmake_minimum_required(VERSION 3.30)
project(devils_script_bits32 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB DS_SOURCES "${DS_SOURCE_DIR}/src/*.cpp")
add_executable(devils_script_bits32 "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp" ${DS_SOURCES})
target_include_directories(devils_script_bits32 PRIVATE
  "${DS_SOURCE_DIR}/include" "${TAVL_SOURCE_DIR}/include" "${REFLECT_SOURCE_DIR}")
target_compile_definitions(devils_script_bits32 PRIVATE DEVILS_SCRIPT_32BIT=1)
target_link_libraries(devils_script_bits32 PRIVATE "${TAVL_LIBRARY}")
