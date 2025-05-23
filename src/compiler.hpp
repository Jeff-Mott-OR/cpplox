#pragma once

#include <string_view>

#include "chunk.hpp"
#include "interned_strings.hpp"
#include "memory.hpp"

namespace motts::lox
{
    // Turn text source code into bytecode, and use the provided GC heap to allocate function objects.
    GC_ptr<Function> compile(GC_heap&, Interned_strings&, std::string_view source);
}
