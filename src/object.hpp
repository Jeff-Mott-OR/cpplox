#pragma once

#include "object-fwd.hpp"

#include <string_view>
#include "chunk.hpp"

namespace motts { namespace lox {
    struct Function {
        std::string_view name;
        Chunk chunk;
    };
}}
