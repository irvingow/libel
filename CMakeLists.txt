cmake_minimum_required(VERSION 3.15)
project(libel)

set(CMAKE_CXX_STANDARD 14)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

SET(CXX_FLAGS
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
    -Wreturn-type
    -march=native
    -std=c++11
    -rdynamic)

string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH  ${PROJECT_BINARY_DIR}/lib)
add_compile_options(-Wreturn-type)

find_package(Protobuf)
find_package(ZLIB)

if (PROTOBUF_FOUND)
    message(STATUS "found protobuf")
else()
    message(STATUS "protobuf not found")
endif()
if (ZLIB_FOUND)
    message(STATUS "found zlib")
else()
    message(STATUS "zlib not found")
endif()

include_directories(${PROJECT_SOURCE_DIR})

string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

add_subdirectory(libel/base)
add_subdirectory(libel/net)
