#pragma once

#include <cstdint>
#include <vector>

#include "memory.hpp"
#include "object.hpp"
#include "value.hpp"

namespace motts { namespace lox {
    class VM {
        struct Call_frame {
            GC_ptr<Closure> closure {};
            std::vector<std::uint8_t>::const_iterator opcode_iter;
            std::vector<Value>::size_type stack_begin_index;
        };

        struct Push_stack_frame_visitor;

        GC_heap& gc_heap_;
        std::size_t gc_heap_last_collect_size_ {};
        Interned_strings& interned_strings_;

        std::unordered_map<GC_ptr<std::string>, Value> globals_;
        std::vector<Value> stack_;
        std::vector<Call_frame> frames_;
        std::vector<GC_ptr<Upvalue>> open_upvalues_;

        public:
            VM(GC_heap&, Interned_strings&);
            void run(const GC_ptr<Closure>& closure, bool debug);
    };
}}
