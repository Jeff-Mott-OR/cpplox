#pragma once

#include <ostream>

namespace motts { namespace lox {
    #define MOTTS_LOX_TOKEN_TYPE_NAMES \
        /* Single-character tokens */ \
        X(left_paren) X(right_paren) \
        X(left_brace) X(right_brace) \
        X(comma) X(dot) X(minus) X(plus) \
        X(semicolon) X(slash) X(star) \
        \
        X(identifier) X(number) \
        X(and_) X(or_) \
        X(eof)

    enum class Token_type {
        #define X(name) name,
        MOTTS_LOX_TOKEN_TYPE_NAMES
        #undef X
    };

    std::ostream& operator<<(std::ostream&, const Token_type&);
}}
