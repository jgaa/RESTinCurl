cmake_minimum_required(VERSION 3.13...3.30 FATAL_ERROR)

# Tests CMakeLists.txt for RESTinCurl

# Find threading library
find_package(Threads REQUIRED)

# Macro to add and optionally auto-run unit tests
macro(ADD_AND_RUN_UNITTEST testname targetname)
    add_test(NAME ${testname} COMMAND ${targetname})
    # Enforce C++20 for tests
    set_target_properties(${targetname} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)

    if(AUTORUN_UNIT_TESTS)
        add_custom_command(TARGET ${targetname}
            POST_BUILD
            COMMENT "Running ${testname}"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION> -R "${testname}" --output-on-failure
        )
    endif()
endmacro()

# Include any external test frameworks or helpers
add_subdirectory(coro-asio)
add_subdirectory(coro-unifex)

# General tests
add_executable(general_tests general_tests.cpp)
target_link_libraries(general_tests PRIVATE RESTinCurl::RESTinCurl)
add_dependencies(general_tests externalLest)
ADD_AND_RUN_UNITTEST(GENERAL_TESTS general_tests)

# Queue tests
add_executable(queue_tests queue_tests.cpp)
target_link_libraries(queue_tests PRIVATE RESTinCurl::RESTinCurl)
add_dependencies(queue_tests externalLest)
ADD_AND_RUN_UNITTEST(QUEUE_TESTS queue_tests)

# App test
add_executable(app_test app_test.cpp)
target_link_libraries(app_test PRIVATE RESTinCurl::RESTinCurl)
ADD_AND_RUN_UNITTEST(APP_TEST app_test)

# Upload test (requires external JSON and logging libs)
add_executable(upload_test upload_test.cpp)
target_link_libraries(upload_test PRIVATE RESTinCurl::RESTinCurl)
add_dependencies(upload_test externalJson externalLog)
ADD_AND_RUN_UNITTEST(UPLOAD_TEST upload_test)

# Examples
foreach(example RANGE 1 7)
    set(name example${example})
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE RESTinCurl::RESTinCurl)
    add_dependencies(${name} externalJson externalLog)
    string(TOUPPER ${name} UPPER_NAME)
    ADD_AND_RUN_UNITTEST(${UPPER_NAME} ${name})
endforeach()

# Target test (demonstrating alias consumption)
add_executable(target_test target_test.cpp)
target_link_libraries(target_test PRIVATE RESTinCurl::RESTinCurl)
ADD_AND_RUN_UNITTEST(TARGET_TEST target_test)
