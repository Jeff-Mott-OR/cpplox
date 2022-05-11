#pragma once

#include "compiler.hpp"
#include "memory.hpp"
#include "vm.hpp"

namespace motts { namespace lox {
    struct Lox {
        GC_heap gc_heap;
        Interned_strings interned_strings {gc_heap};
        VM vm {gc_heap, interned_strings};

        auto compile(const gsl::cstring_span<>& source) {
            return motts::lox::compile(gc_heap, interned_strings, source);
        }
    };
}}
