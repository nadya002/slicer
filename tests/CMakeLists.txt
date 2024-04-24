# Подключение Google Test
#enable_testing()
#find_package(GTest REQUIRED)

# Создание исполняемого файла тестов
add_executable(runUnitTests test_balancer_lib.cpp)

# Включение директорий с заголовочными файлами
#add_subdirectory(../contrib/googletest)

#target_include_directories(runUnitTests PRIVATE ${GTEST_INCLUDE_DIRS})
target_link_libraries(runUnitTests ${GTEST_LIBRARIES} Balancer)

# Добавление тестов
add_test(NAME balancer_lib_test COMMAND runUnitTests)
