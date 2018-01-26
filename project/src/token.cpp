#include "token.hpp"

#include <boost/lexical_cast.hpp>

using std::ostream;
using std::string;

using boost::lexical_cast;

namespace motts { namespace lox {
    ostream& operator<<(ostream& os, Token_type token_type) {
        switch (token_type) {
            #define X(name) \
                case Token_type::name: \
                    os << #name; \
                    break;
            MOTTS_LOX_TOKEN_TYPE_NAMES
            #undef X
        }

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
            return lexical_cast<string>(type) + " " + lexeme;
        }

    ostream& operator<<(ostream& os, const Token& token) {
        os << token.to_string();

        return os;
    }
}}
