ARG CC=clang

FROM ubuntu:22.04 AS base

    RUN apt update && apt install -y cmake git ninja-build

FROM base AS base-clang

    RUN apt install -y clang

FROM base AS base-gcc

    RUN apt install -y build-essential

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

    RUN apt install -y cppcheck valgrind
    COPY test /project/src/test
    RUN cmake ../src -GNinja -DDEPS_ONLY=FALSE -DENABLE_TESTING=TRUE
    RUN cmake --build .
    RUN ctest --verbose --output-on-failure

FROM test as debug

    RUN apt install -y gdb vim
    RUN cmake ../src -GNinja -DDEPS_ONLY=FALSE -DENABLE_TESTING=TRUE -DCMAKE_BUILD_TYPE=Debug
    RUN cmake --build .
    RUN ctest --verbose --output-on-failure
