#pragma once

#include <vector>

#include "parser.hpp"
#include "scanner.hpp"

namespace motts { namespace lox {
    // X-macro technique
    #define MOTTS_LOX_OPCODE_NAMES \
        X(return_) X(constant) X(negate) \
        X(add) X(subtract) X(multiply) X(divide) \
        X(print)

    enum class Opcode {
        #define X(name) name,
        MOTTS_LOX_OPCODE_NAMES
        #undef X
    };

    struct Chunk {
        std::vector<int> code;
        std::vector<double> constants;
        std::vector<int> lines;
    };

    Chunk parse_bc(Token_iterator&&);
}}
