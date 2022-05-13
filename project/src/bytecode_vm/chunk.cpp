#include "chunk.hpp"

#include <stdexcept>
#include <string>

#include <boost/algorithm/string.hpp>

namespace motts { namespace lox {
    std::ostream& operator<<(std::ostream& os, Opcode opcode) {
        // IIFE so I can use return rather than assign-and-break
        std::string name {([&] () {
            switch (opcode) {
                default:
                    throw std::logic_error{"Unreachable"};

                #define X(name) \
                    case Opcode::name: \
                        return #name;
                MOTTS_LOX_OPCODE_NAMES
                #undef X
            }
        })()};

        // Field names should print as uppercase without trailing underscores
        boost::trim_right_if(name, boost::is_any_of("_"));
        boost::to_upper(name);

        os << name;

        return os;
    }
}}
