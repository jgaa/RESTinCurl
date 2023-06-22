cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(restincurl VERSION 0.2 LANGUAGES CXX)

include(cmake_scripts/external-projects.cmake)

message(STATUS "Using ${CMAKE_CXX_COMPILER}")

if (NOT DEFINED WITH_APIDOC)
    option(WITH_APIDOC "Generate Doxygen documentation")
endif()

macro(SET_CPP_STANDARD target)
    message(STATUS "Using C++ 14 for ${target}")
    set_property(TARGET ${target} PROPERTY CXX_STANDARD 14)
endmacro(SET_CPP_STANDARD)

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

message(STATUS "Curl library: ${CURL_LIBRARIES}")

set(DEFAULT_LIBRARIES ${CURL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

include_directories(
    ${CURL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
    )

link_directories( ${CURL_LIBRARIES} )

enable_testing()
add_subdirectory(tests)

include(cmake_scripts/doxygen.cmake)
