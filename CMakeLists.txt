cmake_minimum_required(VERSION 3.30)
Include(FetchContent)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(devils_script VERSION 1.2.0)

enable_language(CXX)

option(DS_BUILD_TESTS "Build tests for devils_script" ON)
option(DS_BUILD_EXAMPLES "Build examples for devils_script" ON)
# ISA for auto-vectorizing devils_script's own sources. Applied PRIVATE; not forced on
# consumers (they pick their own arch). No SIMD intrinsics in headers, so no ABI impact.
include(cmake/Architecture.cmake)
# Width of the language's own numbers. OFF: int64/double. ON: int32/float. This changes the
# layout of stack values and the meaning of the public accessors, so it propagates PUBLIC - a
# consumer has to see the same setting the library was built with.
option(DS_32BIT "Use 32-bit script numbers (int32_t / float)" OFF)

# MSVC runtime: default to the DLL runtime (replaces the hand-written /MD /MDd). Guarded so a
# consumer embedding devils_script can pick the static runtime instead.
if (NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# LTO needs a per-compiler value: GCC wants -flto=auto (parallel jobs), Clang wants -flto=thin.
set(DS_LTO_OPTION "-flto")
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(DS_LTO_OPTION "-flto=auto")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(DS_LTO_OPTION "-flto=thin")
endif()

set(DS_ARCH_GNU "")
set(DS_ARCH_MSVC "")
if (DS_ARCH STREQUAL "AVX")
  set(DS_ARCH_GNU "-mavx")
  set(DS_ARCH_MSVC "/arch:AVX")
elseif (DS_ARCH STREQUAL "AVX2")
  set(DS_ARCH_GNU "-mavx2")
  set(DS_ARCH_MSVC "/arch:AVX2")
elseif (DS_ARCH STREQUAL "NATIVE")
  set(DS_ARCH_GNU "-march=native")
endif()

add_library(devils_script_options INTERFACE)
target_compile_options(devils_script_options INTERFACE
  $<$<CONFIG:Debug>:$<$<CXX_COMPILER_ID:MSVC>:${DS_ARCH_MSVC} /Zi /Wall /wd5045 /wd4820 /wd4514 /GR- /permissive- /Zc:preprocessor>>
  $<$<CONFIG:Release>:$<$<CXX_COMPILER_ID:MSVC>:${DS_ARCH_MSVC} /Wall /wd5045 /wd4820 /O2 /GL /GR- /permissive- /Zc:preprocessor>>
  $<$<CONFIG:MinSizeRel>:$<$<CXX_COMPILER_ID:MSVC>:${DS_ARCH_MSVC} /Wall /wd5045 /wd4820 /O2 /GL /GR- /permissive- /Zc:preprocessor>>
  $<$<CONFIG:RelWithDebInfo>:$<$<CXX_COMPILER_ID:MSVC>:${DS_ARCH_MSVC} /Zi /Wall /wd5045 /wd4820 /O2 /GL /GR- /permissive- /Zc:preprocessor>>

  $<$<CONFIG:Debug>:$<$<CXX_COMPILER_ID:GNU,Clang>:${DS_ARCH_GNU} -g -Wall -Wextra -fno-rtti -fno-inline>>
  $<$<CONFIG:Release>:$<$<CXX_COMPILER_ID:GNU,Clang>:${DS_ARCH_GNU} -O3 -Wall -Wextra -fno-rtti -pipe ${DS_LTO_OPTION}>>
  $<$<CONFIG:MinSizeRel>:$<$<CXX_COMPILER_ID:GNU,Clang>:${DS_ARCH_GNU} -O3 -Wall -Wextra -fno-rtti -pipe ${DS_LTO_OPTION} -s>>
  $<$<CONFIG:RelWithDebInfo>:$<$<CXX_COMPILER_ID:GNU,Clang>:${DS_ARCH_GNU} -O3 -g -Wall -Wextra -fno-rtti -pipe ${DS_LTO_OPTION}>>

  # Fat LTO objects: the static archive carries both real machine code and LTO bitcode, so it links
  # whether or not the consumer enables LTO. GCC only — Clang's fat-object support is unstable.
  $<$<AND:$<CONFIG:Release,MinSizeRel,RelWithDebInfo>,$<CXX_COMPILER_ID:GNU>>:-ffat-lto-objects>
)
# LTO must also be passed at link time (where the actual whole-program optimization runs), with the
# same value as at compile time so GCC/Clang don't warn about a mismatch. MSVC pairs /GL with /LTCG.
target_link_options(devils_script_options INTERFACE
  $<$<CONFIG:Release,MinSizeRel,RelWithDebInfo>:$<$<CXX_COMPILER_ID:GNU,Clang>:${DS_LTO_OPTION}>>
  $<$<CONFIG:Release,MinSizeRel,RelWithDebInfo>:$<$<CXX_COMPILER_ID:MSVC>:/LTCG>>
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
  GIT_TAG        v1.0.3
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
  src/common.cpp
  src/context.cpp
  src/container.cpp
  src/script_ast.cpp
  src/system_ast.cpp
  src/system_builtins.cpp
  src/system.cpp
  src/optimizer.cpp
  src/basic_functions.cpp
  src/prng.cpp
)

add_library(devils_script STATIC ${SRC_LIST} ${HDR_LIST})
# Options are PRIVATE so the build flags (arch, warnings, RTTI, LTO, MSVC runtime) stay internal and
# don't leak onto consumers. Only the C++20 requirement and tavl propagate via PUBLIC.
target_link_libraries(devils_script PRIVATE devils_script_options)
target_link_libraries(devils_script PUBLIC tavl)
target_compile_features(devils_script PUBLIC cxx_std_20)
target_compile_definitions(devils_script PUBLIC DEVILS_SCRIPT_32BIT=$<BOOL:${DS_32BIT}>)
# MSVC: the /GL object code requires /LTCG when the archive is created.
set_target_properties(devils_script PROPERTIES
  STATIC_LIBRARY_OPTIONS "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release,MinSizeRel,RelWithDebInfo>>:/LTCG>"
)
target_include_directories(devils_script PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (DS_BUILD_EXAMPLES)
  add_executable (devils_script_example_desc examples/desc.cpp)
  target_link_libraries(devils_script_example_desc devils_script devils_script_options)

  add_executable (devils_script_example_arithmetic_types examples/arithmetic_types.cpp)
  target_link_libraries(devils_script_example_arithmetic_types devils_script devils_script_options)
endif()

if (DS_BUILD_TESTS)
  enable_testing()
  FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.5.2
  )

  FetchContent_MakeAvailable(doctest)

  add_executable (devils_script_tests
    tests/audit.cpp
    tests/optimizer.cpp
    tests/numbers.cpp
    tests/main.cpp
    tests/basics.cpp
    tests/statements.cpp
    tests/typecheck.cpp
    tests/description.cpp
    tests/context_lists.cpp
    tests/builtins.cpp
    tests/case1.cpp
    tests/introspection.cpp
    tests/disasm.cpp
    tests/execute.cpp)
  target_link_libraries(devils_script_tests devils_script devils_script_options doctest::doctest)
  add_test(NAME devils_script_tests COMMAND devils_script_tests)
  FetchContent_GetProperties(reflect)
  add_test(NAME devils_script_compile_fail COMMAND ${CMAKE_COMMAND}
    -S ${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail
    -B ${CMAKE_CURRENT_BINARY_DIR}/compile_fail
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DDS_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
    -DTAVL_SOURCE_DIR=${tavl_SOURCE_DIR}
    -DREFLECT_SOURCE_DIR=${reflect_SOURCE_DIR})
  add_test(NAME devils_script_architecture COMMAND ${CMAKE_COMMAND}
    -DDS_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/architecture.cmake)

  # The 32-bit configuration is compiled and run as its own executable: the main suite pins 64-bit
  # values, so it cannot double as the check that the narrow build still holds its numeric contract.
  add_test(NAME devils_script_bits32_configure COMMAND ${CMAKE_COMMAND}
    -S ${CMAKE_CURRENT_SOURCE_DIR}/tests/bits32
    -B ${CMAKE_CURRENT_BINARY_DIR}/bits32
    -DCMAKE_BUILD_TYPE=$<IF:$<CONFIG:Debug>,Debug,Release>
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DDS_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
    -DTAVL_SOURCE_DIR=${tavl_SOURCE_DIR}
    -DREFLECT_SOURCE_DIR=${reflect_SOURCE_DIR}
    -DTAVL_LIBRARY=$<TARGET_FILE:tavl>)
  add_test(NAME devils_script_bits32_build COMMAND ${CMAKE_COMMAND}
    --build ${CMAKE_CURRENT_BINARY_DIR}/bits32)
  add_test(NAME devils_script_bits32 COMMAND ${CMAKE_CURRENT_BINARY_DIR}/bits32/devils_script_bits32)
  set_tests_properties(devils_script_bits32_build PROPERTIES DEPENDS devils_script_bits32_configure)
  set_tests_properties(devils_script_bits32 PROPERTIES DEPENDS devils_script_bits32_build)


  add_executable (devils_script_benchs tests/case1_bench.cpp)
  target_link_libraries(devils_script_benchs devils_script devils_script_options)

  add_executable (devils_script_execute_bench tests/execute_bench.cpp)
  target_link_libraries(devils_script_execute_bench devils_script devils_script_options)

  add_executable(devils_script_process_bench tests/process_bench.cpp)
  target_link_libraries(devils_script_process_bench devils_script devils_script_options)
endif()
