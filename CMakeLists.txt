cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(restincurl VERSION 0.5 LANGUAGES CXX)

option(RESTINCURL_BUILD_TESTS "Build tests" OFF)
include(cmake_scripts/external-projects.cmake)

message(STATUS "Using ${CMAKE_CXX_COMPILER}")

if (NOT DEFINED WITH_APIDOC)
    option(WITH_APIDOC "Generate Doxygen documentation")
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(CURL REQUIRED)

message(STATUS "Curl library: ${CURL_LIBRARIES}")

set(DEFAULT_LIBRARIES ${CURL_LIBRARIES} pthread)

# Create the main library target (header-only INTERFACE library)
add_library(restincurl INTERFACE)

# Set include directories for the target
target_include_directories(restincurl INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${CURL_INCLUDE_DIRS}
)

# Link required libraries
target_link_libraries(restincurl INTERFACE
    ${CURL_LIBRARIES}
    pthread
)

# Create the namespaced alias for easy consumption
add_library(RESTinCurl::RESTinCurl ALIAS restincurl)

# Set CXX standard requirement
target_compile_features(restincurl INTERFACE cxx_std_14)

# Legacy support for existing tests
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
