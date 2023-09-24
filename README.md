# Build

    docker build --tag=cpplox --target=build .

### Options:

    --tag=<name>

The tag can be whatever name you want for your image.

    --target=<stage>

The stage can be one of: `deps`, `build`, `test`, or `debug`.

    --build-arg CC=<compiler>

The compiler can be one of:`clang` or `gcc`. Defaults to `clang`.

## Use REPL in container shell

    docker run -it cpplox
    > print "Hello, Lox!";

## Run a script outside the container

In this example, I mount `$(pwd)/test/lox` into the container as `/project/host`, and I run a Lox script from that mounted folder.

    docker run -it --mount type=bind,source=$(pwd)/test/lox,target=/project/host,readonly cpplox ./cpploxbc /project/host/hello.lox

## Disassemble the executable

    docker run -it cpplox sh -c "objdump -j .text -dC --no-addresses --no-show-raw-insn cpploxbc | less"

## Development

Docker will cache stages such as deps, build, and test, but a change to a single source file will re-run the entire build stage. To get incremental builds -- very handy during development -- we can mount our host files and run cmake from a container.

    docker run -it --mount type=bind,source=$(pwd),target=/project/src,readonly cpplox bash
    # cmake --build .
    # ctest
