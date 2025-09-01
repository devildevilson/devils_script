cmake_minimum_required(VERSION 3.30)
Include(FetchContent)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(devils_script)

enable_language(CXX)

option(DS_BUILD_TESTS "Build tests for devils_script" ON)

include_directories(
  include/
)

set(HDR_LIST
  include/devils_script/type_traits.h
  include/devils_script/context.h 
  include/devils_script/container.h 
  include/devils_script/common.h 
  include/devils_script/system.h 
  include/devils_script/text.h 
  include/devils_script/basic_functions.h 
  include/devils_script/template_functions.h 
  include/devils_script/prng.h
)

set(SRC_LIST
  src/container.cpp 
  src/text.cpp 
  src/system.cpp 
  src/basic_functions.cpp 
  src/prng.cpp
)

#add_compile_definitions(DEVILS_SCRIPT_OUTER_NAMESPACE=devils_engine)
#add_compile_definitions(DEVILS_SCRIPT_INNER_NAMESPACE=script)
add_compile_definitions(DEVILS_SCRIPT_OUTER_NAMESPACE=devils_script)

add_library(devils_script STATIC ${SRC_LIST} ${HDR_LIST})
set_property(TARGET devils_script PROPERTY CXX_STANDARD 20)
if (MSVC)
  target_compile_options(devils_script PRIVATE
    /permissive-
    $<$<CONFIG:Debug>:/Od /Zi /GR- /Wall>
    $<$<CONFIG:Release>:/O2 /DNDEBUG /GR- /Wall>
    $<$<CONFIG:RelWithDebInfo>:/O2 /Zi /DNDEBUG /GR- /Wall>
  )
else()
  target_compile_options(devils_script PRIVATE
    $<$<CONFIG:Debug>:-Wall -Wextra -g -fno-rtti>
    $<$<CONFIG:Release>:-O3 -Wall -Wextra -fno-rtti>
    $<$<CONFIG:RelWithDebInfo>:-O3 -Wall -Wextra -g -fno-rtti>
  )
endif()

if (DS_BUILD_TESTS) 
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.10.0
  )

  FetchContent_MakeAvailable(Catch2)

  add_executable (devils_script_tests tests/basic.cpp tests/case1.cpp)
  set_property(TARGET devils_script_tests PROPERTY CXX_STANDARD 20)
  target_link_libraries(devils_script_tests devils_script Catch2::Catch2WithMain)
  if (MSVC)
    target_compile_options(devils_script_tests PRIVATE
      /permissive-
      $<$<CONFIG:Debug>:/Od /Zi /GR- /Wall>
      $<$<CONFIG:Release>:/O2 /DNDEBUG /GR- /Wall>
      $<$<CONFIG:RelWithDebInfo>:/O2 /Zi /DNDEBUG /GR- /Wall>
    )
  else()
    target_compile_options(devils_script_tests PRIVATE
      $<$<CONFIG:Debug>:-Wall -Wextra -g -fno-rtti>
      $<$<CONFIG:Release>:-O3 -Wall -Wextra -fno-rtti>
      $<$<CONFIG:RelWithDebInfo>:-O3 -Wall -Wextra -g -fno-rtti>
    )
  endif()
endif()

# optional lua converter? or another project?
#target_link_libraries(devils_script2 lua54)
#set(PROJECT_EXTERNAL_DEPENDENCY_DIR ${CMAKE_CURRENT_LIST_DIR}/libs)
if (WIN32)
  #target_link_directories(devils_script2 PRIVATE ${PROJECT_EXTERNAL_DEPENDENCY_DIR}/lua54)
  #target_include_directories(devils_script2 PRIVATE ${PROJECT_EXTERNAL_DEPENDENCY_DIR}/lua54/include)
endif()
