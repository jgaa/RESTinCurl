cmake_minimum_required(VERSION 3.05...3.30 FATAL_ERROR)
project(restincurl VERSION 0.5 LANGUAGES CXX)


option(RESTINCURL_BUILD_TESTS "Build tests" OFF)
option(RESTINCURL_WITH_APIDOC "Generate Doxygen documentation" OFF)

include(cmake_scripts/external-projects.cmake)

message(STATUS "Using ${CMAKE_CXX_COMPILER}")

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

message(STATUS "Curl library: ${CURL_LIBRARIES}")

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
    CURL::libcurl
    Threads::Threads
)

# Create the namespaced alias for easy consumption
add_library(RESTinCurl::RESTinCurl ALIAS restincurl)

include(CheckCXXSourceCompiles)

# A little snippet that errors-out if __cplusplus < 201402L
set(CXX14_TEST_SOURCE [=[
  #if __cplusplus < 201402L
  # error "C++14 or newer is required"
  #endif
  int main() { return 0; }
]=])

check_cxx_source_compiles("${CXX14_TEST_SOURCE}" CXX_SUPPORTS_CPP14)

if(NOT CXX_SUPPORTS_CPP14)
    # Set CXX standard requirement
    target_compile_features(restincurl INTERFACE cxx_std_14)
    set(CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    unset(CXX_SUPPORTS_CPP14)

    check_cxx_source_compiles("${CXX14_TEST_SOURCE}" CXX_SUPPORTS_CPP14_2)
    if(NOT CXX_SUPPORTS_CPP14_2)
        message(FATAL_ERROR
            "Your compiler does not support C++14 (i.e. __cplusplus < 201402L).\n"
            "Please pass a -std=c++14 (or newer) flag, or use a newer compiler.")
    endif()
endif()

if(RESTINCURL_BUILD_TESTS)
    #find_package(GTest REQUIRED)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

if(RESTINCURL_WITH_APIDOC)
  include(cmake_scripts/doxygen.cmake)
endif()

install(TARGETS restincurl
  EXPORT restincurlTargets
  INCLUDES DESTINATION include
)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT restincurlTargets
  NAMESPACE RESTinCurl::
  DESTINATION lib/cmake/restincurl
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/restincurlConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/restincurlConfigVersion.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_scripts/restincurlConfig.cmake"  # if you provide one
  DESTINATION lib/cmake/restincurl
)
