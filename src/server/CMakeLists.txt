cmake_minimum_required(VERSION 3.10) # Проверка версии CMake.
									# Если версия установленой программы
									# старее указаной, произайдёт аварийный выход.

project(main)				# Название проекта

set(CMAKE_CXX_STANDARD 14)

set(SOURCE_EXE main.cpp)			# Установка переменной со списком исходников

#include_directories(lib)			# Расположение заголовочных файлов

add_subdirectory(lib)				# Добавление подпроекта, указывается имя дирректории

add_executable(main ${SOURCE_EXE})	# Создает исполняемый файл с именем main

target_link_libraries(main PRIVATE Balancer)		# Линковка программы с библиотекой