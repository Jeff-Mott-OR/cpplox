ARG CC=gcc

FROM ubuntu:22.04 AS base

    RUN apt update && apt install -y cmake git ninja-build

FROM base AS base-clang

    RUN apt update && apt install -y clang

FROM base AS base-gcc

    RUN apt update && apt install -y build-essential

FROM base-$CC AS deps

    WORKDIR /project/src
    COPY CMakeLists.txt /project/src

    WORKDIR /project/build
    RUN cmake ../src -GNinja -DDEPS_ONLY=TRUE
    RUN cmake --build .

FROM deps AS build

    COPY src /project/src/src
    RUN cmake ../src -GNinja -DDEPS_ONLY=FALSE
    RUN cmake --build .

    CMD ./cpploxbc

FROM build AS test

    RUN apt update && apt install -y clang-format cppcheck valgrind
    COPY .clang-format /project/src
    COPY test /project/src/test
    RUN cmake ../src -GNinja -DDEPS_ONLY=FALSE -DENABLE_TESTING=TRUE
    RUN cmake --build .
    RUN ctest --verbose --output-on-failure

FROM test as bench

    RUN apt update && apt install -y nodejs default-jdk
    WORKDIR /project/build/_deps/crafting_interpreters-src
    RUN make jlox clox
    WORKDIR /project/build
    RUN ./bench_test

FROM bench as debug

    RUN apt update && apt install -y gdb vim
    RUN cmake ../src -GNinja -DDEPS_ONLY=FALSE -DENABLE_TESTING=TRUE -DCMAKE_BUILD_TYPE=Debug
    RUN cmake --build .
    RUN ctest --verbose --output-on-failure
