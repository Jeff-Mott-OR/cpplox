FROM ubuntu:22.04

RUN apt update && apt install -y git ninja-build cmake build-essential

WORKDIR /project/build
COPY CMakeLists.txt /project
RUN cmake .. -G Ninja -DDEPS_ONLY=TRUE && cmake --build .

COPY . /project
RUN cmake .. -G Ninja -DDEPS_ONLY=FALSE && cmake --build .

CMD ./project
