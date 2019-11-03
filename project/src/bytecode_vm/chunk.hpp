#pragma once

#include <vector>
#include "value.hpp"

namespace motts { namespace lox {
    enum class Op_code {
        constant,
        add,
        subtract,
        multiply,
        divide,
        negate,
        return_
    };

    struct Chunk {
        std::vector<int> code;
        std::vector<int> lines;
        std::vector<Value> constants;
    };

    void write_chunk(Chunk&, Op_code, int line);
    void write_chunk(Chunk&, int constant_offset, int line);
}}
