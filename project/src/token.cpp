// Related header
#include "token.hpp"
// C standard headers
// C++ standard headers
#include <stdexcept>
// Third-party headers
// This project's headers

using std::logic_error;
using std::ostream;

namespace motts { namespace lox {
    ostream& operator<<(ostream& os, const Token_type& token_type) {
        switch (token_type) {
            #define X(name, name_str) \
                case Token_type::name: \
                    os << name_str; \
                    break;
            MOTTS_LOX_TOKEN_TYPE_NAMES
            #undef X

            default:
                throw logic_error{"Unexpected token type."};
        }

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
