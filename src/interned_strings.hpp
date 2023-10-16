#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include "memory.hpp"

namespace motts::lox
{
    class Interned_strings
    {
        GC_heap& gc_heap_;
        std::unordered_map<std::string_view, GC_ptr<const std::string>> strings_by_chars_;
        std::unordered_map<const GC_control_block_base*, GC_ptr<const std::string>> strings_by_ptr_;

      public:
        Interned_strings(GC_heap&);
        ~Interned_strings();

        GC_ptr<const std::string> get(const char*);
        GC_ptr<const std::string> get(std::string_view);
        GC_ptr<const std::string> get(std::string&&);
    };
}
