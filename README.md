# [Crafting Interpreters](http://www.craftinginterpreters.com/): An Implementation in C++

This is a bytecode compiler and virtual machine interpreter for the Lox programming language. Lox is dynamically typed, garbage collected, with first-class function closures, and classes and inheritance.

### What sets this apart from other implementations?

- Dockerfile for reproducible environment of compilers, tools, and builds.
- Extensive unit and functional tests using Boost Test, and a Valgrind test to verify zero memory leaks.
- Comparative benchmarks against jlox, clox, and node, using Google Benchmark.
- Perf profiling tools and setup, following from [CppCon Chandler Carruth Tuning C++](https://www.youtube.com/watch?v=nXaxk27zwlk).
- Embraced C++-isms, such as writing the token scanner as a forward iterator, which allows my compiler to use the familiar deref-increment pattern (`token = *token_iter++`).
- Simple dependency container and argument passing. No globals or singletons.
- Mark-and-sweep garbage collector is independent and relies only on the std library. It uses a smart-pointer style (`gc_ptr = gc_heap.make<string>("Hello")`), no inheritance required on collectable types, and I used function callback listeners to mark roots.

### A gentle, friendly introduction to Lox:

    print "Hello, world!";

    var imAVariable = "here is my value";
    var iAmNil;

    if (iAmNil) {
        print "yes";
    } else {
        print "no";
    }

    var a = 1;
    while (a < 10) {
        print a;
        a = a + 1;
    }

    for (var a = 1; a < 10; a = a + 1) {
        print a;
    }

    fun returnSum(a, b) {
        return a + b;
    }

    fun returnFunction() {
        var outside = "outside";

        return fun() {
            print outside;
        };
    }

    var innerFn = returnFunction();
    innerFn();

    class Breakfast {
        init(meat, bread) {
            this.meat = meat;
            this.bread = bread;
        }

        serve(who) {
            print "Enjoy your " + this.meat + " and " +
                this.bread + ", " + who + ".";
        }
    }

    class Brunch < Breakfast {
        drink() {
            print "How about a Bloody Mary?";
        }
    }

    var benedict = Brunch("ham", "English muffin");
    benedict.serve("Noble Reader");

## Build

    docker build --tag=cpploximg --target=build .

### Options:

`--tag=<name>` The tag can be whatever name you want for your image.

`--target=<stage>` The stage can be one of: `deps`, `build`, `bench`, `perf`, `test`, or `debug`.

`--build-arg CC=<compiler>` The compiler can be one of: `gcc` or `clang`. Defaults to `clang`.

## Use REPL in container shell

    docker run -it --rm cpploximg

    > print "Hello, Lox!";

## Run a script outside the container

In this example, I mount `$(pwd)/test/lox` into the container as `/host`, and I run a Lox script from that mounted folder.

    docker run -it --rm -v $(pwd)/test/lox:/host:ro cpploximg ./cpploxbc /host/hello.lox

## Development

Docker will re-run the entire build stage when a single source changes. To get incremental builds -- very handy during development -- we can mount our host files and run cmake from within a container.

    docker run -it --rm -v $(pwd):/project/src:ro cpploximg bash
    # cmake --build .
    # ctest

## Profile

The perf target will *prepare* the project and tools, then you must run in a container with privileged access.

    docker run -it --privileged --rm cpploximg bash
    # ./perf record -g ./perf_test && ./perf report -g graph,0.5,caller
