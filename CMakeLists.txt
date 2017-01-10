cmake_minimum_required(VERSION 2.6)

project(Vnet C CXX)

enable_testing()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

set(CXX_FLAGS
 -static-libstdc++
 -g
 -DCHECK_PTHREAD_RETURN_VALUE
 -Wall
 -Wextra
 -Werror
 -Wconversion
 -Wno-unused-parameter
 -Wold-style-cast
 -Woverloaded-virtual
 -Wpointer-arith
 -Wshadow
 -Wwrite-strings
 -march=native
 -rdynamic
 )
if(CMAKE_BUILD_BITS EQUAL 32)
  list(APPEND CXX_FLAGS "-m32")
endif()
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_COMPILER "g++-6.1")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -finline-limit=1000 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(VNET_LIBRARIES_DIR ${PROJECT_BINARY_DIR}/lib)

include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/src")

add_library (event STATIC IMPORTED)
set_target_properties(event PROPERTIES IMPORTED_LOCATION "${VNET_LIBRARIES_DIR}/libevent.a")

add_library (FastUdx STATIC IMPORTED)
set_target_properties(FastUdx PROPERTIES IMPORTED_LOCATION "${VNET_LIBRARIES_DIR}/libFastUdx.a")

add_library (event_pthreads STATIC IMPORTED)
set_target_properties(event_pthreads PROPERTIES IMPORTED_LOCATION "${VNET_LIBRARIES_DIR}/libevent_pthreads.a")

string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

add_subdirectory(src)
add_subdirectory(test)

