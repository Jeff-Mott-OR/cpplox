#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "value.hpp"

namespace motts { namespace lox {
    enum class Op_code {
        constant,
        nil,
        true_,
        false_,
        pop,
        get_local,
        set_local,
        get_global,
        define_global,
        set_global,
        equal,
        greater,
        less,
        add,
        subtract,
        multiply,
        divide,
        not_,
        negate,
        print,
        jump,
        jump_if_false,
        loop,
        return_
    };

    struct Chunk {
        std::vector<std::uint8_t> code;
        std::vector<int> lines;
        std::vector<Value> constants;

        void bytecode_push_back(Op_code, int line);
        void bytecode_push_back(std::uint8_t byte, int line);
        void bytecode_push_back(int byte, int line);
        void bytecode_push_back(std::size_t byte, int line);
        void bytecode_push_back(std::ptrdiff_t byte, int line);
        std::vector<Value>::size_type constants_push_back(Value);
    };
}}
