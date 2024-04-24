# Подключение GoogleTest как дополнительной библиотеки
add_subdirectory(contrib/googletest)
add_subdirectory(tests)

# включаем тестирование через CMake
enable_testing()

# Используем Google Test в проекте
# include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
