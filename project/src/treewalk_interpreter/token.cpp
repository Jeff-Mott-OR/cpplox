#include "token.hpp"
#include <stdexcept>
#include <boost/algorithm/string.hpp>

using std::logic_error;
using std::ostream;
using std::string;

using boost::is_any_of;
using boost::to_upper;
using boost::trim_right_if;

namespace motts { namespace lox {
    ostream& operator<<(ostream& os, Token_type token_type) {
        // IIFE so I can use return rather than assign-and-break
        string name {([&] () {
            switch (token_type) {
                #define X(name) \
                    case Token_type::name: \
                        return #name;
                MOTTS_LOX_TOKEN_TYPE_NAMES
                #undef X

                default:
                    throw logic_error{"Unexpected token type."};
            }
        })()};

        // Field names should print as uppercase without trailing underscores
        trim_right_if(name, is_any_of("_"));
        to_upper(name);

        os << name;

        return os;
    }

    ostream& operator<<(ostream& os, const Token& token) {
        os << token.type << " " << token.lexeme << " ";
        if (token.literal) {
            os << *(token.literal);
        } else {
            os << "null";
        }

        return os;
    }
}}
