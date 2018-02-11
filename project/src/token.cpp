#include "token.hpp"

#include <stdexcept>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using std::logic_error;
using std::ostream;
using std::string;

using boost::is_any_of;
using boost::lexical_cast;
using boost::to_upper;
using boost::trim_right_if;

namespace motts { namespace lox {
    ostream& operator<<(ostream& os, Token_type token_type) {
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

    // class Token

        Token::Token(Token_type type_, const string& lexeme_, int line_)
            : type {type_},
              lexeme {lexeme_},
              line {line_}
        {}

        Token::~Token() = default;

        string Token::to_string() const {
            return lexical_cast<string>(type) + " " + lexeme + " null";
        }

    ostream& operator<<(ostream& os, const Token& token) {
        os << token.to_string();

        return os;
    }
}}
