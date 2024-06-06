# Использование базового образа с установленным gcc, g++, cmake
FROM ubuntu:latest as builder

RUN apt-get update && \
    apt-get install -y build-essential cmake g++ gcc wget curl zip unzip tar ninja-build git pkg-config flex bison

RUN wget -qO vcpkg.tar.gz https://github.com/microsoft/vcpkg/archive/master.tar.gz
RUN mkdir /opt/vcpkg
RUN tar xf vcpkg.tar.gz --strip-components=1 -C /opt/vcpkg
RUN VCPKG_FORCE_SYSTEM_BINARIES=1 /opt/vcpkg/bootstrap-vcpkg.sh
RUN ln -s /opt/vcpkg/vcpkg /usr/local/bin/vcpkg
RUN VCPKG_FORCE_SYSTEM_BINARIES=1 vcpkg integrate install
RUN VCPKG_FORCE_SYSTEM_BINARIES=1 vcpkg install braft

WORKDIR /slicer

COPY . /slicer

RUN mkdir build

WORKDIR /slicer/build
RUN cmake .. -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
RUN make

EXPOSE 8080

CMD ["/badge_cli"]
