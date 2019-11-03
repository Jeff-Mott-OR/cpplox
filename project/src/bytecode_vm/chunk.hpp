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
        std::vector<unsigned char> code;
        std::vector<int> lines;
        std::vector<Value> constants;

        void bytecode_push_back(Op_code, int line);
        void bytecode_push_back(unsigned char byte, int line);
        std::vector<Value>::size_type constants_push_back(Value);
    };
}}
