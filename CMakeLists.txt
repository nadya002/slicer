cmake_minimum_required(VERSION 3.0)

project(libcam)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=undefined -g")
# set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address -fsanitize=undefined -g")

add_compile_definitions(GLOG_USE_GLOG_EXPORT)

# Зависимости
add_subdirectory(contrib/googletest)
add_subdirectory(contrib/cpp-httplib)
add_subdirectory(contrib/json)
add_subdirectory(contrib/spdlog)

add_subdirectory(internal/balancer)
add_subdirectory(cmd)

# Стартовая точка
# add_subdirectory(src/pkg)
# add_subdirectory(src/cmd)

# Тесты
# enable_testing()
# add_subdirectory(tests)