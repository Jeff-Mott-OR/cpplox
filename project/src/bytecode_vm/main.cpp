#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.hpp"
#include "chunk.hpp"
#include "debug.hpp"
#include "vm.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {
    void run_prompt(VM& vmr) {
        while (true) {
            std::cout << "> ";

            std::string source_line;
            std::getline(std::cin, source_line);

            interpret(vmr, source_line.c_str());
        }
    }

    void run_file(VM& vmr, const char* path) {
        const auto source = ([&] () {
            std::ifstream in {path};
            in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            return std::string{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
        })();

        InterpretResult result = interpret(vmr, source.c_str());

        // TODO Use exceptions
        if (result == INTERPRET_COMPILE_ERROR) exit(65);
        if (result == INTERPRET_RUNTIME_ERROR) exit(70);
    }
}

int main(int argc, const char* argv[]) {
    VM vm; // [one]

    if (argc == 1) {
        run_prompt(vm);
    } else if (argc == 2) {
        run_file(vm, argv[1]);
    } else {
        std::cerr << "Usage: clox [path]\n";
        exit(64);
    }

    return 0;
}
