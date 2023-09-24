#pragma once

#include <string_view>

#include "chunk.hpp"
#include "memory.hpp"

namespace motts { namespace lox {
    Chunk compile(GC_heap&, std::string_view source);
}}
