#pragma once

#include <ostream>

namespace motts { namespace lox {
    #define MOTTS_LOX_OPCODE_NAMES \
        X(constant)

    enum class Opcode {
        #define X(name) name,
        MOTTS_LOX_OPCODE_NAMES
        #undef X
    };

    std::ostream& operator<<(std::ostream&, const Opcode&);
}}
