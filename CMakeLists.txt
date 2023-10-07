cmake_minimum_required(VERSION 3.1)
project(devils_script)

option(BUILD_SHARED_LIBS "compile as shared lib" OFF)

if (EXISTS DEVILS_SCRIPT_OUTER_NAMESPACE)
  add_definitions(-DDEVILS_SCRIPT_OUTER_NAMESPACE=${DEVILS_SCRIPT_OUTER_NAMESPACE})
endif (EXISTS DEVILS_SCRIPT_OUTER_NAMESPACE)

if (EXISTS DEVILS_SCRIPT_INNER_NAMESPACE)
  add_definitions(-DDEVILS_SCRIPT_INNER_NAMESPACE=${DEVILS_SCRIPT_INNER_NAMESPACE})
endif (EXISTS DEVILS_SCRIPT_INNER_NAMESPACE)

add_definitions()

if (CMAKE_BUILD_TYPE MATCHES "Debug")
  if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /ZI /MTd /GR- /permissive-")
  else()
    #-fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=undefined -fno-omit-frame-pointer
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2 -std=c++17 -Wall -Wextra -g -fno-rtti")
  endif()
else()
  add_definitions(-D_NDEBUG)
  add_definitions(-DNDEBUG)
  if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /Ox /MT /GR- /permissive- /GL")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2 -std=c++17 -Wall -Wextra -O3 -s -fno-rtti")
  endif()
endif (CMAKE_BUILD_TYPE MATCHES "Debug")

if (MSVC)
  if (CMAKE_BUILD_TYPE MATCHES "Debug")
    set_property(TARGET ${TARGET_NAME} APPEND PROPERTY LINK_FLAGS /DEBUG)
    set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set_target_properties(${TARGET_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
    set_target_properties(${TARGET_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
  else()
    set_property(TARGET ${TARGET_NAME} APPEND PROPERTY LINK_FLAGS "/LTCG /OPT:REF /OPT:ICF")
    set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set_target_properties(${TARGET_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
    set_target_properties(${TARGET_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
  endif()
endif()

include_directories(
  src/
  include/span/include/
  include/parallel-hashmap/
  include/sol2/include/
)

# old files located to exclude/
set(SRC_LIST
  src/core.cpp
  src/container.cpp
  src/context.cpp
  src/numeric_functions.cpp
  src/logic_commands.cpp
  src/common_commands.cpp
  src/object.cpp
  src/system.cpp
  src/common.cpp
  src/linear_rng.cpp
  src/core_interface.cpp
)

set(HDR_LIST
  src/numeric_commands_macro.h
  src/numeric_functions.h
  src/logic_commands_macro.h
  src/logic_commands.h
  src/common_commands.h
  src/core.h
  src/header.h
  src/container.h
  src/context.h
  src/interface.h
  src/object.h
  src/system.h
  src/type_info.h
  src/type_traits.h
  src/templates.h
  src/common.h
  src/memory_pool.h
  src/linear_rng.h
  src/core_interface.h
)

add_library(${PROJECT_NAME} ${SRC_LIST} ${HDR_LIST})
target_link_libraries(${PROJECT_NAME} lua)
