#pragma once

// Related header
// C standard headers
// C++ standard headers
#include <ostream>
// Third-party headers
#include <boost/optional.hpp>
// This project's headers
#include "literal.hpp"

namespace motts { namespace lox {
    /*
    We want to serialize an enum's name, but C++ doesn't generate that for us. This will get better when C++ gets
    reflection and injection (https://youtu.be/4AfRAVcThyA?t=21m33s). But for now settle for x-macro technique
    (https://en.wikipedia.org/wiki/X_Macro).

    The enum field names are intentionally lowercase rather than uppercase. Uppercase isn't meant to denote a constant;
    it's meant to denote a macro. Historically, constants were implemented as macros, which is why we became accustomed
    to uppercasing them, but if we're not writing macros then we shouldn't uppercase the name. This manifests as a real
    practical issue because some other header already defined a macro named EOF, so if we try to use EOF as one of our
    enum field names, bad things will happen.
    */
    #define MOTTS_LOX_TOKEN_TYPE_NAMES \
        /* Single char tokens */ \
        X(left_paren, "LEFT_PAREN") X(right_paren, "RIGHT_PAREN") \
        X(left_brace, "LEFT_BRACE") X(right_brace, "RIGHT_BRACE") \
        X(comma, "COMMA") X(dot, "DOT") X(minus, "MINUS") X(plus, "PLUS") \
        X(semicolon, "SEMICOLON") X(slash, "SLASH") X(star, "STAR") \
        \
        /* One or two char tokens */ \
        X(bang, "BANG") X(bang_equal, "BANG_EQUAL") X(equal, "EQUAL") X(equal_equal, "EQUAL_EQUAL") \
        X(greater, "GREATER") X(greater_equal, "GREATER_EQUAL") X(less, "LESS") X(less_equal, "LESS_EQUAL") \
        \
        /* Literals */ \
        X(identifier, "IDENTIFIER") X(string, "STRING") X(number, "NUMBER") \
        \
        /* Keywords */ \
        /* These are real reserved words in C++, so mangle their names in some way */ \
        X(and_, "AND") X(class_, "CLASS") X(else_, "ELSE") X(false_, "FALSE") X(fun_, "FUN") X(for_, "FOR") \
        X(if_, "IF") X(nil_, "NIL") X(or_, "OR") X(print_, "PRINT") X(return_, "RETURN") X(super_, "SUPER") \
        X(this_, "THIS") X(true_, "TRUE") X(var_, "VAR") X(while_, "WHILE") \
        \
        X(eof, "EOF")

    enum class Token_type {
        #define X(name, name_str) name,
        MOTTS_LOX_TOKEN_TYPE_NAMES
        #undef X
    };

    std::ostream& operator<<(std::ostream&, Token_type);

    struct Token {
        Token_type type;
        std::string lexeme;
        boost::optional<Literal> literal;
        int line;
    };

    std::ostream& operator<<(std::ostream&, const Token&);
}}
