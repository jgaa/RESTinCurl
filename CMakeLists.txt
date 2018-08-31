cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(restincurl VERSION 0.1 LANGUAGES CXX)

include(cmake_scripts/external-projects.cmake)

message(STATUS "Using ${CMAKE_CXX_COMPILER}")

macro(SET_CPP_STANDARD target)
    message(STATUS "Using C++ 14 for ${target}")
endmacro(SET_CPP_STANDARD)

find_package(CURL REQUIRED)

message(STATUS "Curl library: ${CURL_LIBRARIES}")

set(DEFAULT_LIBRARIES ${CURL_LIBRARIES})

include_directories(
    ${CURL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
    )

link_directories( ${CURL_LIBRARIES} )

add_subdirectory(tests)

