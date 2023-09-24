#pragma once

#include <iostream>
#include <ostream>
#include <unordered_map>
#include <vector>

#include "compiler.hpp"

namespace motts { namespace lox {
    void run(const Chunk&, std::ostream& = std::cout, bool debug = false);

    class VM {
        std::vector<Dynamic_type_value> stack_;
        std::unordered_map<std::string, Dynamic_type_value> globals_;

        struct Call_frame {
            const Chunk& chunk;
            Bytecode_vector::const_iterator bytecode_iter;
            const std::vector<Dynamic_type_value>::size_type stack_begin_index;
        };
        std::vector<Call_frame> call_frames_;

        public:
            void run(const Chunk&, std::ostream& = std::cout, bool debug = false);
    };
}}
