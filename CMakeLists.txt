cmake_minimum_required(VERSION 3.0)

project(libcam)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_subdirectory(contrib/googletest)
add_subdirectory(contrib/cpp-httplib)
add_subdirectory(contrib/json)
add_subdirectory(contrib/spdlog)

add_subdirectory(tests)

add_subdirectory(src/server/lib)
add_subdirectory(src/server/http)
add_subdirectory(src/server/cmd)