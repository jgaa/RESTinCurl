
macro (ADD_AND_RUN_UNITTEST testname targetname)
SET_CPP_STANDARD(${targetname})
add_test(NAME ${testname} COMMAND ${targetname})
if (AUTORUN_UNIT_TESTS)
    add_custom_command(
        TARGET ${targetname}
        POST_BUILD
        COMMENT "running ${testname}"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION> -R "${testname}"  --output-on-failure
    )
endif()
endmacro()


add_executable(general_tests general_tests.cpp)
target_link_libraries(general_tests
    ${DEFAULT_LIBRARIES}
)
add_dependencies(general_tests externalLest)
ADD_AND_RUN_UNITTEST(GENERAL_TESTS general_tests)

add_executable(queue_tests queue_tests.cpp)
target_link_libraries(queue_tests
    ${DEFAULT_LIBRARIES}
)
add_dependencies(queue_tests externalLest)
ADD_AND_RUN_UNITTEST(QUEUE_TESTS queue_tests)

add_executable(app_test app_test.cpp)
target_link_libraries(app_test
    ${DEFAULT_LIBRARIES}
)
ADD_AND_RUN_UNITTEST(APP_TEST app_test)

add_executable(upload_test upload_test.cpp)
add_dependencies(upload_test externalJson externalLog)
target_link_libraries(upload_test ${DEFAULT_LIBRARIES})
ADD_AND_RUN_UNITTEST(UPLOAD_TEST upload_test)


add_executable(example1 example1.cpp)
add_dependencies(example1 externalJson externalLog)
target_link_libraries(example1
    ${DEFAULT_LIBRARIES}
)
ADD_AND_RUN_UNITTEST(EXAMPLE1 example1)

add_executable(example2 example2.cpp)
add_dependencies(example2 externalJson externalLog)
target_link_libraries(example2
    ${DEFAULT_LIBRARIES}
)
ADD_AND_RUN_UNITTEST(EXAMPLE2 example2)

add_executable(example3 example3.cpp)
add_dependencies(example3 externalJson externalLog)
target_link_libraries(example3
    ${DEFAULT_LIBRARIES}
)
ADD_AND_RUN_UNITTEST(EXAMPLE3 example3)

add_executable(example4 example4.cpp)
add_dependencies(example4 externalJson externalLog)
target_link_libraries(example4
    ${DEFAULT_LIBRARIES}
)
ADD_AND_RUN_UNITTEST(EXAMPLE4 example4)

add_executable(example5 example5.cpp)
add_dependencies(example5 externalJson externalLog)
target_link_libraries(example5
    ${DEFAULT_LIBRARIES}
)
ADD_AND_RUN_UNITTEST(EXAMPLE5 example5)

add_executable(example6 example6.cpp)
add_dependencies(example6 externalJson externalLog)
target_link_libraries(example6
    ${DEFAULT_LIBRARIES}
)
ADD_AND_RUN_UNITTEST(EXAMPLE6 example6)

add_executable(example7 example7.cpp)
add_dependencies(example7 externalJson externalLog)
target_link_libraries(example7
    ${DEFAULT_LIBRARIES}
)
ADD_AND_RUN_UNITTEST(EXAMPLE7 example7)
