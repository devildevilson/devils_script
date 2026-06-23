cmake_minimum_required(VERSION 3.30)
Include(FetchContent)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(devils_script VERSION 1.0.0)

enable_language(CXX)

option(DS_BUILD_TESTS "Build tests for devils_script" ON)
option(DS_BUILD_EXAMPLES "Build examples for devils_script" ON)

add_library(devils_script_options INTERFACE)
target_compile_features(devils_script_options INTERFACE cxx_std_20)
target_compile_options(devils_script_options INTERFACE
  $<$<CONFIG:Debug>:$<$<CXX_COMPILER_ID:MSVC>:/arch:AVX /MDd /Zi /Wall /wd5045 /wd4820 /wd4514 /GR- /permissive- /Zc:preprocessor>>
  $<$<CONFIG:Release>:$<$<CXX_COMPILER_ID:MSVC>:/arch:AVX /MD /Wall /wd5045 /wd4820 /O2 /GR- /permissive- /Zc:preprocessor>>
  $<$<CONFIG:MinSizeRel>:$<$<CXX_COMPILER_ID:MSVC>:/arch:AVX /MD /Wall /wd5045 /wd4820 /O2 /GR- /permissive- /Zc:preprocessor>>
  $<$<CONFIG:RelWithDebInfo>:$<$<CXX_COMPILER_ID:MSVC>:/arch:AVX /MDd /ZI /Wall /wd5045 /wd4820 /O2 /GR- /permissive- /Zc:preprocessor>>

  $<$<CONFIG:Debug>:$<$<CXX_COMPILER_ID:GNU,Clang>:-mavx -g -Wall -Wextra -fno-rtti -fno-inline>>
  $<$<CONFIG:Release>:$<$<CXX_COMPILER_ID:GNU,Clang>:-mavx -O3 -Wall -Wextra -fno-rtti -pipe -flto>>
  $<$<CONFIG:MinSizeRel>:$<$<CXX_COMPILER_ID:GNU,Clang>:-mavx -O3 -Wall -Wextra -fno-rtti -pipe -flto -s>>
  $<$<CONFIG:RelWithDebInfo>:$<$<CXX_COMPILER_ID:GNU,Clang>:-mavx -O3 -g -Wall -Wextra -fno-rtti -pipe -flto>>
)
target_compile_definitions(devils_script_options INTERFACE
  $<$<CONFIG:Debug>:>
  $<$<CONFIG:Release>:NDEBUG _NDEBUG>
  $<$<CONFIG:MinSizeRel>:NDEBUG _NDEBUG>
  $<$<CONFIG:RelWithDebInfo>:>
)

FetchContent_Declare(
  tavl
  GIT_REPOSITORY https://github.com/devildevilson/tavl.git
  GIT_TAG        v1.0.0
)

FetchContent_MakeAvailable(tavl)

set(HDR_LIST
  include/devils_script/type_traits.h
  include/devils_script/context.h
  include/devils_script/container.h
  include/devils_script/common.h
  include/devils_script/system.h
  include/devils_script/system_templates.h
  include/devils_script/script_ast.h
  include/devils_script/basic_functions.h
  include/devils_script/template_functions.h
  include/devils_script/prng.h
)

set(SRC_LIST
  src/container.cpp
  src/script_ast.cpp
  src/system_ast.cpp
  src/system_builtins.cpp
  src/system.cpp
  src/basic_functions.cpp
  src/prng.cpp
)

#add_compile_definitions(DEVILS_SCRIPT_OUTER_NAMESPACE=devils_engine)
#add_compile_definitions(DEVILS_SCRIPT_INNER_NAMESPACE=script)
add_compile_definitions(DEVILS_SCRIPT_OUTER_NAMESPACE=devils_script)

add_library(devils_script STATIC ${SRC_LIST} ${HDR_LIST})
target_link_libraries(devils_script PUBLIC devils_script_options tavl)
target_include_directories(devils_script PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (DS_BUILD_EXAMPLES)
  add_executable (devils_script_example_desc examples/desc.cpp)
  target_link_libraries(devils_script_example_desc devils_script devils_script_options)
endif()

if (DS_BUILD_TESTS)
  FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.5.2
  )

  FetchContent_MakeAvailable(doctest)

  add_executable (devils_script_tests tests/main.cpp tests/basic.cpp tests/case1.cpp tests/introspection.cpp tests/disasm.cpp tests/execute.cpp)
  target_link_libraries(devils_script_tests devils_script devils_script_options doctest::doctest)

  add_executable (devils_script_benchs tests/case1_bench.cpp)
  target_link_libraries(devils_script_benchs devils_script devils_script_options)
endif()
