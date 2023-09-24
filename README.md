# Build

## With Docker

1.

    docker build --tag=cpplox --file=Dockerfile.gcc .
        - OR -
    docker build --tag=cpplox --file=Dockerfile.clang .

2.

    docker run --interactive --tty --rm cpplox

Optionally, interact with built image in bash.

    docker run --interactive --tty --rm cpplox bash
