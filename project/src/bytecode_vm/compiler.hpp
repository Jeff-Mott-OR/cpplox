#pragma once

#include <gsl/gsl_util>

#include "memory.hpp"
#include "object.hpp"

namespace motts { namespace lox {
    GC_ptr<Closure> compile(GC_heap& gc_heap, Interned_strings& interned_strings, const gsl::cstring_span<>& source);
}}
