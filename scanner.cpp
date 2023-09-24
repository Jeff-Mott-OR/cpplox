#include "scanner.hpp"

#include <stdexcept>
#include <string>

#include <boost/algorithm/string.hpp>

namespace motts { namespace lox {
    std::ostream& operator<<(std::ostream& os, const Token_type& token_type) {
        std::string name {([&] () {
            switch (token_type) {
                default:
                    throw std::logic_error{"Unexpected token type."};

                #define X(name) \
                    case Token_type::name: \
                        return #name;
                MOTTS_LOX_TOKEN_TYPE_NAMES
                #undef X
            }
        })()};

        // Names should print as uppercase without trailing underscores
        boost::trim_right_if(name, boost::is_any_of("_"));
        boost::to_upper(name);

        os << name;

        return os;
    }
}}
