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

    docker run --interactive --tty --rm cpplox
    > print "Hello, Lox!";

## Run a script outside the container

    docker run -v /path/to/your-lox-scripts:/project/host-mount --rm cpplox ./cpploxbc /project/host-mount/your-script.lox

## Disassemble the executable

    docker run --interactive --tty --rm cpplox sh -c "objdump --section=.text --disassemble --demangle --source --line-numbers cpploxbc | less"

## Copy the executable to your host system

    docker create --name xcontainer cpplox && docker cp xcontainer:/project/build/cpploxbc . && docker rm xcontainer
