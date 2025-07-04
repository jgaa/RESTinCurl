project(coro-asio)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)

find_package(Boost 1.75 REQUIRED COMPONENTS
    system)

add_executable(coro-asio coro-asio.cpp)
target_link_libraries(coro-asio
    ${DEFAULT_LIBRARIES}
)
add_dependencies(coro-asio externalLest)
ADD_AND_RUN_UNITTEST(GENERAL_TESTS coro-asio)

