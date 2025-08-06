cmake_minimum_required(VERSION 3.15)
project(coro-unifex LANGUAGES CXX)

set(CMAKE_CXX_STANDARD      20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS    OFF)

# 1) Fetch libunifex from GitHub
include(FetchContent)
FetchContent_Declare(
  libunifex
  GIT_REPOSITORY https://github.com/facebookexperimental/libunifex.git
  GIT_TAG        main    # or pin to a release tag
)
FetchContent_MakeAvailable(libunifex)

# 2) Ensure there is a target called "unifex"
#    Some versions of the CMakeLists in libunifex may not automatically
#    declare a 'unifex' target when added as a subdirectory, so guard it.
if (NOT TARGET unifex)
  add_library(unifex INTERFACE)
endif()

# 3) Point that target at the headers you just fetched
target_include_directories(
  unifex
  INTERFACE
    $<BUILD_INTERFACE:${libunifex_SOURCE_DIR}/include>
)

# 4) Build your test executable
add_executable(coro-unifex coro-unifex.cpp)

# 5) Link against the INTERFACE target "unifex"
target_link_libraries(coro-unifex
  PRIVATE
    RESTinCurl::RESTinCurl
    unifex
)

# 6) Hook it into your existing test machinery
add_dependencies(coro-unifex externalLest)
ADD_AND_RUN_UNITTEST(GENERAL_TESTS coro-unifex)
