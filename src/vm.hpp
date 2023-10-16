#pragma once

#include <ostream>
#include <unordered_map>
#include <vector>

#include "chunk.hpp"
#include "interned_strings.hpp"
#include "memory.hpp"
#include "value.hpp"

namespace motts::lox
{
    class VM
    {
        const bool debug_;
        std::ostream& os_;
        GC_heap& gc_heap_;
        Interned_strings& interned_strings_;
        std::size_t gc_heap_last_collect_size_{0};

        std::vector<Dynamic_type_value> stack_;
        std::vector<GC_ptr<Closure>> call_frames_;
        std::unordered_map<GC_ptr<const std::string>, Dynamic_type_value> globals_;

      public:
        VM(GC_heap&, Interned_strings&, std::ostream&, bool debug = false);
        ~VM();

        void run(const Chunk&);

      private:
        void run(GC_ptr<Closure>, std::size_t stack_begin_index);
    };
}
