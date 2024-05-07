# Использование базового образа с установленным gcc, g++, cmake
FROM ubuntu:latest as builder

RUN apt-get update && \
    apt-get install -y build-essential cmake g++ gcc

WORKDIR /slicer

COPY . /slicer

RUN mkdir build
WORKDIR /slicer/build
RUN cmake .. && make

# FROM scratch
# COPY --from=builder /slicer/build/src/server/cmd/ServerSlicer /
# CMD ["/ServerSlicer"]

CMD ["/slicer/build/src/server/cmd/ServerSlicer"]

