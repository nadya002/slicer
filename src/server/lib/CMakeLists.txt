
cmake_minimum_required(VERSION 3.10) # Проверка версии CMake.
									# Если версия установленой программы
									# старее указаной, произайдёт аварийный выход.

add_library(Balancer balancer.cpp)# Создание статической библиотеки

target_include_directories(Balancer INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
