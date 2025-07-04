cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(restincurl VERSION 0.5 LANGUAGES CXX)

include(cmake_scripts/external-projects.cmake)

option(RESTINCURL_BUILD_TESTS "Build tests" OFF)

message(STATUS "Using ${CMAKE_CXX_COMPILER}")

if (NOT DEFINED WITH_APIDOC)
    option(WITH_APIDOC "Generate Doxygen documentation")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(CURL REQUIRED)

message(STATUS "Curl library: ${CURL_LIBRARIES}")

set(DEFAULT_LIBRARIES ${CURL_LIBRARIES} pthread)

include_directories(
    ${CURL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
    )

link_directories( ${CURL_LIBRARIES} )

if(RESTINCURL_BUILD_TESTS)
    #find_package(GTest REQUIRED)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

include(cmake_scripts/doxygen.cmake)
