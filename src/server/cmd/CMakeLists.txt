cmake_minimum_required(VERSION 3.10) # Проверка версии CMake.
									# Если версия установленой программы
									# старее указаной, произайдёт аварийный выход.
project(ServerSlicer VERSION 1.0)


#add_subdirectory(lib)				# Добавление подпроекта, указывается имя дирректории

add_executable(ServerSlicer cmd_server.cpp)

target_link_libraries(
		ServerSlicer
        httplib
		nlohmann_json
		Balancer
)
#add_executable(server ${SOURCE_EXE})	# Создает исполняемый файл с именем main

#target_link_libraries(server PRIVATE Balancer)		# Линковка программы с библиотекой