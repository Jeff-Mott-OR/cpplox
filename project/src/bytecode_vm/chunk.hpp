#pragma once

#include <cstdint>
#include <ostream>
#include <vector>

#include "scanner.hpp"
#include "value.hpp"

// X-macro
#define MOTTS_LOX_OPCODE_NAMES \
    X(constant) \
    X(nil) \
    X(true_) \
    X(false_) \
    X(pop) \
    X(get_local) \
    X(set_local) \
    X(get_global) \
    X(define_global) \
    X(set_global) \
    X(get_upvalue) \
    X(set_upvalue) \
    X(get_property) \
    X(set_property) \
    X(get_super) \
    X(equal) \
    X(greater) \
    X(less) \
    X(add) \
    X(subtract) \
    X(multiply) \
    X(divide) \
    X(not_) \
    X(negate) \
    X(print) \
    X(jump) \
    X(jump_if_false) \
    X(loop) \
    X(call) \
    X(invoke) \
    X(super_invoke) \
    X(closure) \
    X(close_upvalue) \
    X(return_) \
    X(class_) \
    X(inherit) \
    X(method)

namespace motts { namespace lox {
    enum class Opcode {
        #define X(name) name,
        MOTTS_LOX_OPCODE_NAMES
        #undef X
    };

    std::ostream& operator<<(std::ostream&, Opcode);

    struct Chunk {
        std::vector<std::uint8_t> opcodes;
        std::vector<Value> constants;
        std::vector<Token> tokens;
    };
}}
