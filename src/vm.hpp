#pragma once

#include <ostream>
#include <unordered_map>
#include <vector>

#include "chunk.hpp"
#include "memory.hpp"
#include "value.hpp"

namespace motts { namespace lox {
    class VM {
        const bool debug_;
        std::ostream& os_;
        GC_heap& gc_heap_;

        std::vector<Dynamic_type_value> stack_;
        std::unordered_map<std::string, Dynamic_type_value> globals_;

        struct Call_frame {
            GC_ptr<Closure> closure;
            const Chunk& chunk;
            Chunk::Bytecode_vector::const_iterator bytecode_iter;
            const std::vector<Dynamic_type_value>::size_type stack_begin_index;
        };
        std::vector<Call_frame> call_frames_;

        public:
            VM(GC_heap&, std::ostream&, bool debug = false);
            ~VM();

            void run(const Chunk&);
    };
}}
