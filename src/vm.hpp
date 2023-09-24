#pragma once

#include <ostream>
#include <unordered_map>
#include <vector>

#include "compiler.hpp"
#include "memory.hpp"

namespace motts { namespace lox {
    class VM {
        GC_heap& gc_heap_;
        std::ostream& os_;
        bool debug_;

        std::vector<Dynamic_type_value> stack_;
        std::unordered_map<std::string, Dynamic_type_value> globals_;

        struct Call_frame {
            const Chunk& chunk;
            Chunk::Bytecode_vector::const_iterator bytecode_iter;
            const std::vector<Dynamic_type_value>::size_type stack_begin_index;
        };
        std::vector<Call_frame> call_frames_;

        public:
            VM(GC_heap&, std::ostream&, bool debug);
            ~VM();
            void run(const Chunk&);
    };
}}
