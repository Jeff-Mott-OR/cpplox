#pragma once

#include <cstdint>
#include <ostream>
#include <vector>

#include "scanner.hpp"

namespace motts { namespace lox {
    #define MOTTS_LOX_OPCODE_NAMES \
        X(constant) \
        X(nil)

    enum class Opcode {
        #define X(name) name,
        MOTTS_LOX_OPCODE_NAMES
        #undef X
    };

    std::ostream& operator<<(std::ostream&, const Opcode&);

    struct Chunk {
        std::vector<double> constants;
        std::vector<std::uint8_t> bytecode;
        std::vector<Token> source_map_tokens;
    };

    std::ostream& operator<<(std::ostream&, const Chunk&);
}}
