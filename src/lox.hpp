#pragma once

#include <iostream>
#include <ostream>
#include <string>
#include <string_view>

#include "chunk.hpp"
#include "memory.hpp"
#include "vm.hpp"

namespace motts { namespace lox {
    struct Lox {
        GC_heap gc_heap;
        std::ostream& os;
        bool debug;
        VM vm {gc_heap, os, debug};

        Lox(std::ostream& os = std::cout, bool debug = false);

        Chunk compile(std::string_view source);

        void run_file(const std::string& input_file);
        void run_prompt();
    };
}}
