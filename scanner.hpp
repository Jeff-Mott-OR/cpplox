#pragma once

#include <ostream>

namespace motts { namespace lox {
    #define MOTTS_LOX_TOKEN_TYPE_NAMES \
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
