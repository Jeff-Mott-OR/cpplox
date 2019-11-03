#pragma once

#include <vector>
#include "value.hpp"

namespace motts { namespace lox {
    enum class Op_code {
        constant,
        return_
    };

    struct Chunk {
        std::vector<int> code;
        std::vector<int> lines;
        std::vector<Value> constants;
    };
}}
