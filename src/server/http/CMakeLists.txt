cmake_minimum_required(VERSION 3.10) # Проверка версии CMake.
									# Если версия установленой программы
									# старее указаной, произайдёт аварийный выход.

add_library(Http http.cpp)

#add_subdirectory(lib)				# Добавление подпроекта, указывается имя дирректории

target_link_libraries(
	Http
    httplib
	nlohmann_json
	Balancer
	spdlog
)

target_include_directories(Http INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

#add_executable(server ${SOURCE_EXE})	# Создает исполняемый файл с именем main

#target_link_libraries(server PRIVATE Balancer)		# Линковка программы с библиотекой