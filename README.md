# Дипломная работа. Балансировка нагрузки.

## Установка
Необходимо склонировать репозиторий и установить зависимоти.

```
git clone --recurse-submodules https://github.com/nadya002/slicer.git
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install
./vcpkg install braft
```

## Запуск
Возможен запуск через докер, либо локально

Пример локального запуска:
```
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/path_to_vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake
make
 ./cmd/balancer_cli
```

Так же нужно указать переменные окружения c информацией о расположение нод:

1. HOSTS - все хосты на которых работает raft, через запятую
2. DATA_DIR - путь к папке, в которой сохраняются логи рафт
3. REPLICATION_PORT и REPLICATION_HOST - хост и порт, где работает текущий инстанс рафта
4. HTTP_PORT - порт для пользовательских запросов

## Примеры

Генерация запросов находится [тут](generate_requests)