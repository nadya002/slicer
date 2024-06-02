cmake_minimum_required(VERSION 3.0)

project(libcam)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=undefined -g")
set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address -fsanitize=undefined -g")


enable_testing()

add_subdirectory(contrib/googletest)
add_subdirectory(contrib/cpp-httplib)
add_subdirectory(contrib/json)
add_subdirectory(contrib/spdlog)

add_subdirectory(tests)

add_subdirectory(src/server/lib)
add_subdirectory(src/server/http)
add_subdirectory(src/server/cmd)