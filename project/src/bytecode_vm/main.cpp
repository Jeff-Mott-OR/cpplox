#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.hpp" // macros and constants
#include "chunk.hpp"  // Chunk and Op_code types
#include "debug.hpp"  // disassemble declarations
#include "vm.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {
    void run_prompt(VM& vm) {
        while (true) {
            std::cout << "> ";

            std::string source_line;
            std::getline(std::cin, source_line);

            interpret(vm, source_line);
        }
    }

    void run_file(VM& vm, const char* path) {
        const auto source = ([&] () {
            std::ifstream in {path};
            in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            return std::string{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
        })();

        InterpretResult result = interpret(vm, source);

        // TODO Use exceptions
        if (result == INTERPRET_COMPILE_ERROR) exit(65);
        if (result == INTERPRET_RUNTIME_ERROR) exit(70);
    }
}

int main(int argc, const char* argv[]) {
    Deferred_heap deferred_heap_main;
    Interned_strings interned_strings_main {deferred_heap_main};

    VM vm {deferred_heap_main, interned_strings_main}; // [one]

    try {
        if (argc == 1) {
            run_prompt(vm);
        } else if (argc == 2) {
            run_file(vm, argv[1]);
        } else {
            std::cerr << "Usage: clox [path]\n";
            exit(64);
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        exit(EXIT_FAILURE);
    }

    return 0;
}
