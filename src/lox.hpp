#pragma once

#include <iostream>
#include <string>

#include "memory.hpp"
#include "vm.hpp"

namespace motts { namespace lox {
    // Lox serves as an application context and dependency injector.
    struct Lox {
        // Whether to dump debug information such as the stack and bytecode disassembly.
        const bool debug;

        // Which streams to read from and write to. This is useful during testing, for example,
        // to send artificial input and capture the output.
        std::ostream& cout;
        std::ostream& cerr;
        std::istream& cin;

        // Tracks allocations, and frees using mark-and-sweep.
        GC_heap gc_heap;

        // Keeps globals and a stack, and executes bytecode.
        VM vm {gc_heap, cout, debug};

        Lox(
            std::ostream& cout = std::cout,
            std::ostream& cerr = std::cerr,
            std::istream& cin = std::cin,
            bool debug = false
        );
    };

    void run_file(Lox&, const std::string& file_path);
    void run_prompt(Lox&);
}}
