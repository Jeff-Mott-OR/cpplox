#pragma once

#include <vector>
#include "value.hpp"

namespace motts { namespace lox {
    enum class Op_code {
        constant,
        nil,
        true_,
        false_,
        equal,
        greater,
        less,
        add,
        subtract,
        multiply,
        divide,
        not_,
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
