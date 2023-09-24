FROM ubuntu:22.04

RUN apt update && \
    apt install -y git && \
    apt install -y ninja-build && \
    apt install -y cmake && \
    apt install -y build-essential

WORKDIR /project
COPY . .

WORKDIR /project/build
CMD cmake .. -G Ninja && \
    cmake --build .
