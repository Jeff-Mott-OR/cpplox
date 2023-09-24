FROM ubuntu:22.04 AS base

    RUN apt update && apt install -y git ninja-build cmake cppcheck

FROM base AS gcc

    RUN apt install -y build-essential

FROM base AS clang

    RUN apt install -y clang

FROM gcc AS gcc_build

    # Fetch deps and cache
    WORKDIR /project/build
    COPY CMakeLists.txt /project
    RUN cmake .. -GNinja -DDEPS_ONLY=TRUE
    RUN cmake --build .

    # Build project
    COPY src /project/src
    RUN cmake .. -GNinja -DDEPS_ONLY=FALSE -DENABLE_TESTING=FALSE
    RUN cmake --build .

    # Build and run tests
    COPY test /project/test
    RUN cmake .. -GNinja -DDEPS_ONLY=FALSE -DENABLE_TESTING=TRUE
    RUN cmake --build .
    RUN ctest --verbose --output-on-failure

FROM clang AS clang_build

    # Fetch deps and cache
    WORKDIR /project/build
    COPY CMakeLists.txt /project
    RUN cmake .. -GNinja -DDEPS_ONLY=TRUE
    RUN cmake --build .

    # Build project
    COPY src /project/src
    RUN cmake .. -GNinja -DDEPS_ONLY=FALSE -DENABLE_TESTING=FALSE
    RUN cmake --build .

    # Build and run tests
    COPY test /project/test
    RUN cmake .. -GNinja -DDEPS_ONLY=FALSE -DENABLE_TESTING=TRUE
    RUN cmake --build .
    RUN ctest --verbose --output-on-failure

FROM gcc_build AS gcc_debug

    RUN apt install -y gdb vim
    RUN cmake .. -GNinja -DCMAKE_BUILD_TYPE=Debug -DDEPS_ONLY=FALSE -DENABLE_TESTING=TRUE
    RUN cmake --build .
    RUN ctest --verbose --output-on-failure

FROM clang_build AS clang_debug

    RUN apt install -y gdb vim
    RUN cmake .. -GNinja -DCMAKE_BUILD_TYPE=Debug -DDEPS_ONLY=FALSE -DENABLE_TESTING=TRUE
    RUN cmake --build .
    RUN ctest --verbose --output-on-failure

CMD ./project
