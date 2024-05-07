cmake_minimum_required(VERSION 3.10) # Проверка версии CMake.
									# Если версия установленой программы
									# старее указаной, произайдёт аварийный выход.


add_subdirectory(lib)				# Добавление подпроекта, указывается имя дирректории
add_subdirectory(cmd)

#add_executable(main ${SOURCE_EXE})	# Создает исполняемый файл с именем main

#target_link_libraries(main PRIVATE Balancer)		# Линковка программы с библиотекой