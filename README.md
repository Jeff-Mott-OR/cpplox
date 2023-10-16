# [Crafting Interpreters](http://www.craftinginterpreters.com/): An Implementation in C++

This is a bytecode compiler and virtual machine interpreter for the Lox programming language. Lox is dynamically typed, garbage collected, with first-class function closures, and classes and inheritance.

A gentle, friendly introduction to Lox:

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

    docker build --tag=cpplox --target=build .

### Options:

    --tag=<name>

The tag can be whatever name you want for your image.

    --target=<stage>

The stage can be one of: `deps`, `build`, `test`, `bench`, or `debug`.

When targeting `bench`, use Docker's `--progress=plain` option to see the results.

    --build-arg CC=<compiler>

The compiler can be one of: `gcc` or `clang`. Defaults to `gcc`.

## Use REPL in container shell

    docker run -it cpplox
    > print "Hello, Lox!";

## Run a script outside the container

In this example, I mount `$(pwd)/test/lox` into the container as `/project/host`, and I run a Lox script from that mounted folder.

    docker run -it -v $(pwd)/test/lox:/project/host:ro cpplox ./cpploxbc /project/host/hello.lox

## Development

Docker will cache stages such as the build stage, but a change to a single source file will re-run the entire build stage. To get incremental builds -- very handy during development -- we can mount our host files and run cmake from a container.

    docker run -it -v $(pwd):/project/src:ro cpplox bash
    # cmake --build .
    # ctest
